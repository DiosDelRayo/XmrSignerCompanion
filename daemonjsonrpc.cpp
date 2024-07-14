#include "daemonjsonrpc.h"
#include <QJsonDocument>
#include <QNetworkReply>
#include <QEventLoop>
#include <QDateTime>

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

InfoResult DaemonJsonRpc::getInfo(bool cached, int acceptable_cache_seconds) {
    static InfoResult cachedResult;
    static qint64 lastUpdateTime = 0;

    qint64 currentTime = QDateTime::currentSecsSinceEpoch();

    if(cached && (currentTime - lastUpdateTime < acceptable_cache_seconds))
        return cachedResult;

    QJsonObject data = makeRequest("get_info", QJsonObject());
    InfoResult out;

    if (data.contains("result")) {
        QJsonObject result = data["result"].toObject();

        out.adjusted_time = result["adjusted_time"].toInt();
        out.alt_blocks_count = result["alt_blocks_count"].toInt();
        out.block_size_limit = result["block_size_limit"].toInt();
        out.block_size_median = result["block_size_median"].toInt();
        out.block_weight_limit = result["block_weight_limit"].toInt();
        out.block_weight_median = result["block_weight_median"].toInt();
        out.bootstrap_daemon_address = result["bootstrap_daemon_address"].toString();
        out.busy_syncing = result["busy_syncing"].toBool();
        out.credits = result["credits"].toInt();
        out.cumulative_difficulty = result["cumulative_difficulty"].toVariant().toULongLong();
        out.cumulative_difficulty_top64 = result["cumulative_difficulty_top64"].toVariant().toULongLong();
        out.database_size = result["database_size"].toVariant().toULongLong();
        out.difficulty = result["difficulty"].toVariant().toULongLong();
        out.difficulty_top64 = result["difficulty_top64"].toVariant().toULongLong();
        out.free_space = result["free_space"].toVariant().toULongLong();
        out.grey_peerlist_size = result["grey_peerlist_size"].toInt();
        out.height = result["height"].toInt();
        out.height_without_bootstrap = result["height_without_bootstrap"].toInt();
        out.incoming_connections_count = result["incoming_connections_count"].toInt();
        out.mainnet = result["mainnet"].toBool();
        out.nettype = result["nettype"].toString();
        out.offline = result["offline"].toBool();
        out.outgoing_connections_count = result["outgoing_connections_count"].toInt();
        out.rpc_connections_count = result["rpc_connections_count"].toInt();
        out.stagenet = result["stagenet"].toBool();
        out.start_time = result["start_time"].toInt();
        out.status = result["status"].toString();
        out.synchronized = result["synchronized"].toBool();
        out.target = result["target"].toInt();
        out.target_height = result["target_height"].toInt();
        out.testnet = result["testnet"].toBool();
        out.top_block_hash = result["top_block_hash"].toString();
        out.top_hash = result["top_hash"].toString();
        out.tx_count = result["tx_count"].toInt();
        out.tx_pool_size = result["tx_pool_size"].toInt();
        out.untrusted = result["untrusted"].toBool();
        out.update_available = result["update_available"].toBool();
        out.version = result["version"].toString();
        out.was_bootstrap_ever_used = result["was_bootstrap_ever_used"].toBool();
        out.white_peerlist_size = result["white_peerlist_size"].toInt();
        out.wide_cumulative_difficulty = result["wide_cumulative_difficulty"].toString();
        out.wide_difficulty = result["wide_difficulty"].toString();

        // Update cache
        cachedResult = out;
        lastUpdateTime = currentTime;
    } else {
        // Handle error case
        qWarning() << "Error in getInfo(): No 'result' field in response";
    }

    return out;
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
    return this->getInfo(true, 15).height;
}

int DaemonJsonRpc::getLocalHeight()
{
    return this->getInfo().height_without_bootstrap;
}

int DaemonJsonRpc::getSynced()
{
    InfoResult info = this->getInfo();
    return info.height_without_bootstrap * 100 / info.height;
}
