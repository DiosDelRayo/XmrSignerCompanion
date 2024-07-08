#include "daemonjsonrpc.h"
#include <QJsonDocument>
#include <QNetworkReply>
#include <QEventLoop>

DaemonJsonRpc::DaemonJsonRpc(
    QObject *parent,
    const QString& host,
    int port,
    bool tls
    ) : QObject(parent)
    , m_host(host)
    , m_port(port)
    ,m_tls(tls)
{
    m_networkManager = new QNetworkAccessManager(this);
    connect(m_networkManager, &QNetworkAccessManager::authenticationRequired,
            this, &DaemonJsonRpc::handleAuthenticationRequired);
}

void DaemonJsonRpc::setAuthentication(const QString& username, const QString& password)
{
    m_username = username;
    m_password = password;
}

void DaemonJsonRpc::handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply)
    qDebug() << "authenticate with: " << m_username << ":" << m_password;
    authenticator->setUser(m_username);
    authenticator->setPassword(m_password);
    //authenticator->setOption(QAuthenticator::AuthenticationOption::DigestAuthentication, true);
}


QJsonObject DaemonJsonRpc::makeRequest(const QString &method, const QJsonObject &params)
{
    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = "0";
    request["method"] = method;
    request["params"] = params;

    QUrl url = QUrl(QString("http%1://%2:%3/json_rpc").arg(m_tls?"s":"").arg(m_host).arg(m_port));
    qDebug() << "connect to: " << url;
    QNetworkRequest networkRequest(url);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_networkManager->post(networkRequest, QJsonDocument(request).toJson());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    QJsonObject response;
    if (reply->error() == QNetworkReply::NoError) {
        response = QJsonDocument::fromJson(reply->readAll()).object();
    } else {
        qWarning() << "Network error:" << reply->errorString();
    }

    reply->deleteLater();
    return response;
}

// TODO: caching???
QJsonObject DaemonJsonRpc::getInfo() {
    return makeRequest("get_info", QJsonObject());
}

QString DaemonJsonRpc::getVersion()
{
    QJsonObject data = makeRequest("get_version", QJsonObject());

    QJsonObject resultObject = data["result"].toObject();
    int version = resultObject["version"].toInt();
    int major = (version >> 16) & 0xFF;
    int minor = version & 0xFF;
    qDebug() << "version: " << version << " major: " << major << " minor: " << minor << " data: " << data;
    return QString("%1.%2").arg(major).arg(minor);
}

int DaemonJsonRpc::getHeight()
{
    QJsonObject data = this->getInfo();
    QJsonObject resultObject = data["result"].toObject();
    return resultObject["height"].toInt();
}

int DaemonJsonRpc::getLocalHeight()
{
    QJsonObject data = this->getInfo();
    QJsonObject resultObject = data["result"].toObject();
    return resultObject["height_without_bootstrap"].toInt();
}

int DaemonJsonRpc::getSynced()
{
    QJsonObject data = this->getInfo();
    QJsonObject resultObject = data["result"].toObject();
    int local = resultObject["height_without_bootstrap"].toInt();
    int remote = resultObject["height"].toInt();
    return local * 100 / remote;
}
