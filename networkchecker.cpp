#include "networkchecker.h"
#include <QHostInfo>
#include <QJsonObject>
#include <QJsonDocument>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QEventLoop>

bool NetworkChecker::isDomainOrIpLocal(const QString& host)
{
    // First, try to parse the host as an IP address
    QHostAddress address(host);
    if (!address.isNull()) {
        return isIpLocal(address);
    }

    // If it's not a valid IP, try to resolve it as a domain
    QHostInfo hostInfo = QHostInfo::fromName(host);
    if (hostInfo.error() != QHostInfo::NoError) {
        return false;  // Unable to resolve
    }

    // Check all resolved IP addresses
    for (const QHostAddress& addr : hostInfo.addresses()) {
        if (isIpLocal(addr)) {
            return true;
        }
    }

    return false;
}

bool NetworkChecker::isIpLocal(const QHostAddress& address)
{
    if (address.isLoopback()) {
        return true;
    }

    if (address.protocol() == QAbstractSocket::IPv4Protocol) {
        quint32 ipv4 = address.toIPv4Address();
        return ((ipv4 & 0xFF000000) == 0x0A000000 ||  // 10.0.0.0/8
                (ipv4 & 0xFFF00000) == 0xAC100000 ||  // 172.16.0.0/12
                (ipv4 & 0xFFFF0000) == 0xC0A80000 ||  // 192.168.0.0/16
                (ipv4 & 0xFFFF0000) == 0xA9FE0000);   // 169.254.0.0/16
    }
    else if (address.protocol() == QAbstractSocket::IPv6Protocol) {
        Q_IPV6ADDR ipv6 = address.toIPv6Address();
        return (ipv6.c[0] == 0xFE && (ipv6.c[1] & 0xC0) == 0x80) || // fe80::/10
               (ipv6.c[0] == 0xFD || ipv6.c[0] == 0xFC);            // fc00::/7
    }

    return false;
}

void NetworkChecker::checkMonerodRpc(const QString& address, std::function<void(bool)> callback)
{
    QUrl url(address);
    if (!url.isValid()) {
        callback(false);
        return;
    }

    QNetworkAccessManager *manager = new QNetworkAccessManager();
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonObj;
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["id"] = "0";
    jsonObj["method"] = "get_info";

    QJsonDocument doc(jsonObj);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = manager->post(request, data);

    QObject::connect(reply, &QNetworkReply::finished, [reply, callback, manager]() {
        onRpcReplyFinished(reply, callback);
        manager->deleteLater();
    });
}

void NetworkChecker::onRpcReplyFinished(QNetworkReply *reply, std::function<void(bool)> callback)
{
    bool isValid = false;

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray response = reply->readAll();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
        QJsonObject jsonObject = jsonResponse.object();

        // Check if the response contains expected fields
        isValid = jsonObject.contains("result") &&
                  jsonObject["result"].toObject().contains("status") &&
                  jsonObject["result"].toObject().contains("version");
    }

    callback(isValid);
    reply->deleteLater();
}
