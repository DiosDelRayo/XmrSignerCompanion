#include "walletjsonrpc.h"
#include <QJsonDocument>
#include <QNetworkReply>
#include <QEventLoop>

WalletJsonRpc::WalletJsonRpc(
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
    connect(&m_networkManager, &QNetworkAccessManager::authenticationRequired,
            this, &WalletJsonRpc::handleAuthenticationRequired);
}

void WalletJsonRpc::setAuthentication(const QString& username, const QString& password)
{
    m_username = username;
    m_password = password;
}

void WalletJsonRpc::handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_UNUSED(reply)
    authenticator->setUser(m_username);
    authenticator->setPassword(m_password);
}

QJsonObject WalletJsonRpc::makeRequest(const QString &method, const QJsonObject &params)
{
    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = "0";
    request["method"] = method;
    request["params"] = params;

    QNetworkRequest networkRequest(m_url);
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_networkManager.post(networkRequest, QJsonDocument(request).toJson());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
        reply->deleteLater();
        return response["result"].toObject();
    } else {
        qWarning() << "Network error:" << reply->errorString();
        reply->deleteLater();
        return QJsonObject();
    }
}
/*
QJsonObject WalletJsonRpc::makeRequest(const QString &method, const QJsonObject &params)
{
    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = "0";
    request["method"] = method;
    request["params"] = params;

    QNetworkAccessManager manager;
    QNetworkRequest networkRequest(QUrl(QString("http%1://%2:%3/json_rpc").arg(m_tls?"s":"").arg(m_host).arg(m_port)));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = manager.post(networkRequest, QJsonDocument(request).toJson());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
        reply->deleteLater();
        return response["result"].toObject();
    } else {
        reply->deleteLater();
        return QJsonObject();
    }
}
*/

QJsonObject WalletJsonRpc::setDaemon(const QString &address, bool trusted, const QString &ssl_support,
                                     const QString &ssl_private_key_path, const QString &ssl_certificate_path,
                                     const QString &ssl_ca_file, const QStringList &ssl_allowed_fingerprints,
                                     bool ssl_allow_any_cert, const QString &username, const QString &password)
{
    QJsonObject params;
    params["address"] = address;
    params["trusted"] = trusted;
    params["ssl_support"] = ssl_support;
    if (!ssl_private_key_path.isEmpty()) params["ssl_private_key_path"] = ssl_private_key_path;
    if (!ssl_certificate_path.isEmpty()) params["ssl_certificate_path"] = ssl_certificate_path;
    if (!ssl_ca_file.isEmpty()) params["ssl_ca_file"] = ssl_ca_file;
    if (!ssl_allowed_fingerprints.isEmpty()) params["ssl_allowed_fingerprints"] = QJsonArray::fromStringList(ssl_allowed_fingerprints);
    params["ssl_allow_any_cert"] = ssl_allow_any_cert;
    if (!username.isEmpty()) params["username"] = username;
    if (!password.isEmpty()) params["password"] = password;

    return makeRequest("set_daemon", params);
}

QJsonObject WalletJsonRpc::getBalance(unsigned int account_index, const QList<unsigned int> &address_indices,
                                      bool all_accounts, bool strict)
{
    QJsonObject params;
    params["account_index"] = static_cast<int>(account_index);
    if (!address_indices.isEmpty()) {
        QJsonArray indices;
        for (unsigned int index : address_indices) {
            indices.append(static_cast<int>(index));
        }
        params["address_indices"] = indices;
    }
    params["all_accounts"] = all_accounts;
    params["strict"] = strict;

    return makeRequest("get_balance", params);
}

QJsonObject WalletJsonRpc::getAddress(unsigned int account_index, const QList<unsigned int> &address_indices)
{
    QJsonObject params;
    params["account_index"] = static_cast<int>(account_index);
    if (!address_indices.isEmpty()) {
        QJsonArray indices;
        for (unsigned int index : address_indices) {
            indices.append(static_cast<int>(index));
        }
        params["address_index"] = indices;
    }

    return makeRequest("get_address", params);
}

QJsonObject WalletJsonRpc::validateAddress(const QString &address, bool any_net_type, bool allow_openalias)
{
    QJsonObject params;
    params["address"] = address;
    params["any_net_type"] = any_net_type;
    params["allow_openalias"] = allow_openalias;

    return makeRequest("validate_address", params);
}

QJsonObject WalletJsonRpc::getHeight()
{
    return makeRequest("get_height", QJsonObject());
}

QJsonObject WalletJsonRpc::transfer(const QJsonArray &destinations, unsigned int account_index,
                                    const QList<unsigned int> &subaddr_indices, unsigned int priority,
                                    unsigned int ring_size, unsigned int unlock_time, bool get_tx_key,
                                    bool do_not_relay, bool get_tx_hex, bool get_tx_metadata)
{
    QJsonObject params;
    params["destinations"] = destinations;
    params["account_index"] = static_cast<int>(account_index);
    if (!subaddr_indices.isEmpty()) {
        QJsonArray indices;
        for (unsigned int index : subaddr_indices) {
            indices.append(static_cast<int>(index));
        }
        params["subaddr_indices"] = indices;
    }
    params["priority"] = static_cast<int>(priority);
    params["ring_size"] = static_cast<int>(ring_size);
    params["unlock_time"] = static_cast<int>(unlock_time);
    params["get_tx_key"] = get_tx_key;
    params["do_not_relay"] = do_not_relay;
    params["get_tx_hex"] = get_tx_hex;
    params["get_tx_metadata"] = get_tx_metadata;

    return makeRequest("transfer", params);
}

QJsonObject WalletJsonRpc::submitTransfer(const QString &tx_data_hex)
{
    QJsonObject params;
    params["tx_data_hex"] = tx_data_hex;

    return makeRequest("submit_transfer", params);
}

QJsonObject WalletJsonRpc::stopWallet()
{
    return makeRequest("stop_wallet", QJsonObject());
}

QJsonObject WalletJsonRpc::exportOutputs(bool all)
{
    QJsonObject params;
    params["all"] = all;

    return makeRequest("export_outputs", params);
}

QJsonObject WalletJsonRpc::importKeyImages(unsigned int offset, const QJsonArray &signed_key_images)
{
    QJsonObject params;
    params["offset"] = static_cast<int>(offset);
    params["signed_key_images"] = signed_key_images;

    return makeRequest("import_key_images", params);
}

QJsonObject WalletJsonRpc::refresh(unsigned int start_height)
{
    QJsonObject params;
    params["start_height"] = static_cast<int>(start_height);

    return makeRequest("refresh", params);
}

QJsonObject WalletJsonRpc::rescanSpent()
{
    return makeRequest("rescan_spent", QJsonObject());
}

QJsonObject WalletJsonRpc::generateViewOnlyWallet(unsigned int restore_height, const QString &filename,
                                                  const QString &address, const QString &spendkey,
                                                  const QString &viewkey, const QString &password,
                                                  bool autosave_current)
{
    QJsonObject params;
    params["restore_height"] = static_cast<int>(restore_height);
    params["filename"] = filename;
    params["address"] = address;
    params["spendkey"] = spendkey;
    params["viewkey"] = viewkey;
    params["password"] = password;
    params["autosave_current"] = autosave_current;

    return makeRequest("generate_from_keys", params);
}

QJsonObject WalletJsonRpc::closeWallet()
{
    return makeRequest("close_wallet", QJsonObject());
}

QString WalletJsonRpc::getVersion()
{
    QJsonObject data = makeRequest("get_version", QJsonObject());
    int version = data["version"].toInt();
    int major = version & 0xFF00 >> 16;
    int minor = version & 0xFF;
    return QString("%1.%2").arg(major).arg(minor);
}

QJsonObject WalletJsonRpc::estimateTxSizeAndWeight(unsigned int n_inputs, unsigned int n_outputs,
                                                   unsigned int ring_size, bool rct)
{
    QJsonObject params;
    params["n_inputs"] = static_cast<int>(n_inputs);
    params["n_outputs"] = static_cast<int>(n_outputs);
    params["ring_size"] = static_cast<int>(ring_size);
    params["rct"] = rct;

    return makeRequest("estimate_tx_size_and_weight", params);
}
