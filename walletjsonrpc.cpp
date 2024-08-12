#include "walletjsonrpc.h"
#include <QJsonDocument>
#include <QNetworkReply>
#include <QEventLoop>
#include <QByteArray>
#include <QString>
#include <QJsonArray>
#include <QJsonObject>

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
    connect(m_networkManager, &QNetworkAccessManager::authenticationRequired,
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
    qDebug() << "authenticate with: " << m_username << ":" << m_password;
    authenticator->setUser(m_username);
    authenticator->setPassword(m_password);
    //authenticator->setOption(QAuthenticator::AuthenticationOption::DigestAuthentication, true);
}


QJsonObject WalletJsonRpc::makeRequest(const QString &method, const QJsonObject &params)
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

bool WalletJsonRpc::setDaemon(
    const QString &address,
    bool trusted,
    const QString &ssl_support,
    const QString &ssl_private_key_path,
    const QString &ssl_certificate_path,
    const QString &ssl_ca_file,
    const QStringList &ssl_allowed_fingerprints,
    bool ssl_allow_any_cert,
    const QString &username,
    const QString &password
    )
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

    return makeRequest("set_daemon", params).contains("result");
}

BalanceResult WalletJsonRpc::getBalance(
    unsigned int account_index,
    const QList<unsigned int> &address_indices,
    bool all_accounts,
    bool strict
    )
{
    QJsonObject params;
    params["account_index"] = static_cast<qint64>(account_index);
    if (!address_indices.isEmpty()) {
        QJsonArray indices;
        for (unsigned int index : address_indices) {
            indices.append(static_cast<qint64>(index));
        }
        params["address_indices"] = indices;
    }
    if(all_accounts)
            params["all_accounts"] = all_accounts;
    if(strict)
            params["strict"] = strict;

    BalanceResult out;
    QJsonObject data = makeRequest("get_balance", params);
    if(data.contains("error")) {
        QJsonObject error = data["error"].toObject();
        return BalanceResult(true, error["code"].toInt(), error["message"].toString());
    }
    if(data.contains("result")) {
        QJsonObject result = data["result"].toObject();
        out = BalanceResult(
            result["balance"].toInteger(),
            result["unlocked_balance"].toInteger(),
            result["multisig_import_needed"].toBool(),
            result["time_to_unlock"].toInt(),
            result["blocks_to_unlock"].toInt());
            QJsonArray json_per_subaddress = result["per_subaddress"].toArray();
            for(int i = 0; i < json_per_subaddress.count(); i++) {
                QJsonObject jsab = json_per_subaddress[i].toObject();
                SubaddressBalance sab = SubaddressBalance(
                    jsab["account_index"].toInteger(),
                    jsab["address_index"].toInteger(),
                    jsab["address"].toString(),
                    jsab["balance"].toInteger(),
                    jsab["unlocked_balance"].toInteger(),
                    jsab["label"].toString(),
                    jsab["num_unspent_outputs "].toInteger(),
                    jsab["time_to_unlock"].toInt(),
                    jsab["blocks_to_unlock "].toInt());
                out.per_subaddress.append(sab);
            }
    }
    return out;
}

AddressResult WalletJsonRpc::getAddress(unsigned int account_index, const QList<unsigned int> &address_indices)
{
    QJsonObject params;
    params["account_index"] = static_cast<qint64>(account_index);
    if (!address_indices.isEmpty()) {
        QJsonArray indices;
        for (unsigned int index : address_indices) {
            indices.append(static_cast<qint64>(index));
        }
        params["address_index"] = indices;
    }

    QJsonObject data = makeRequest("get_address", params);
    AddressResult out = AddressResult();
    if(data.contains("error")) {
        QJsonObject error = data["error"].toObject();
        return AddressResult(true, error["code"].toInt(), error["message"].toString());
    }
    if(data.contains("result")) {
        QJsonObject result = data["result"].toObject();
        out = AddressResult(result["address"].toString());
        if(result.contains("addresses")) {
            QJsonArray jsonAddresses = result["addresses"].toArray();
            for(int i=0; i<jsonAddresses.count(); i++) {
                QJsonObject jsonAddress = jsonAddresses[i].toObject();
                SubAddress address = SubAddress(
                    jsonAddress["address"].toString(),
                    jsonAddress["label"].toString(),
                    jsonAddress["address_index"].toInteger(),
                    jsonAddress["used"].toBool());
                out.addresses.append(address);
            }
        }
    }
    return out;
}

ValidateAddressResult WalletJsonRpc::validateAddress(const QString &address, bool any_net_type, bool allow_openalias)
{
    QJsonObject params;
    params["address"] = address;
    params["any_net_type"] = any_net_type;
    params["allow_openalias"] = allow_openalias;

    QJsonObject data = makeRequest("validate_address", params);
    ValidateAddressResult out = ValidateAddressResult();
    if(data.contains("error")) {
        QJsonObject error = data["error"].toObject();
        return ValidateAddressResult(true, error["code"].toInt(), error["message"].toString());
    }
    if(data.contains("result")) {
        QJsonObject result = data["result"].toObject();
        out = ValidateAddressResult(
            result["valid"].toBool(),
            result["integrated"].toBool(),
            result["subaddress"].toBool(),
            result["openalias_address"].toString(),
            result["nettype"].toString()
            );
    }
    return out;
}

HeightResult WalletJsonRpc::getHeight()
{
    QJsonObject data = makeRequest("get_height", QJsonObject());
    if(data.contains("error")) {
        QJsonObject error = data["error"].toObject();
        return HeightResult(true, error["code"].toInt(), error["message"].toString());
    }
    if(data.contains("result"))
        return HeightResult((data["result"].toObject()["height"].toInteger()));
    return HeightResult();
}

unsigned int WalletJsonRpc::getSimpleHeight()
{
    HeightResult hr = this->getHeight();
    if(hr.error)
        return HEIGHT_ERROR;
    return hr.height;
}

TransferResult WalletJsonRpc::transfer(
    const QList<Destination> &destinations,
    unsigned int account_index,
    const QList<unsigned int> &subaddr_indices,
    unsigned int priority,
    unsigned int ring_size,
    unsigned int unlock_time,
    bool get_tx_key,
    bool do_not_relay,
    bool get_tx_hex,
    bool get_tx_metadata
    )
{
    QJsonObject params;
    QJsonArray jsonDestinations = QJsonArray();
    for(int i=0; i<destinations.count(); i++) {
        Destination destination = destinations.at(i);
        QJsonObject jsonDestination = QJsonObject();
        jsonDestination["address"] = destination.address;
        jsonDestination["amount"] = static_cast<qint64>(destination.amount);
        jsonDestinations.append(jsonDestination);
    }
    params["destinations"] = jsonDestinations;
    params["account_index"] = static_cast<qint64>(account_index);
    if (!subaddr_indices.isEmpty()) {
        QJsonArray indices;
        for (unsigned int index : subaddr_indices) {
            indices.append(static_cast<qint64>(index));
        }
        params["subaddr_indices"] = indices;
    }
    params["priority"] = static_cast<int>(priority);
    params["ring_size"] = static_cast<int>(ring_size);
    params["unlock_time"] = static_cast<int>(unlock_time);
    if(get_tx_key)
            params["get_tx_key"] = get_tx_key;
    if(do_not_relay)
            params["do_not_relay"] = do_not_relay;
    if(get_tx_hex)
            params["get_tx_hex"] = get_tx_hex;
    if(get_tx_metadata)
            params["get_tx_metadata"] = get_tx_metadata;

    QJsonObject data = makeRequest("transfer", params);
    if(data.contains("error")) {
        QJsonObject error = data["error"].toObject();
        return TransferResult(true, error["code"].toInt(), error["message"].toString());
    }
    TransferResult out;
    if(data.contains("result")) {
        QJsonObject result = data["result"].toObject();
        out = TransferResult(
            result["amount"].toInteger(0),
            result["fee"].toInteger(0),
            result["multisig_txset"].toString(),
            result["tx_blob"].toString(),
            result["tx_hash"].toString(),
            result["tx_key"].toString(),
            result["unsigned_txset"].toString()
            );
    }
    return out;
}

SubmitTransferResult WalletJsonRpc::submitTransfer(const QString &tx_data_hex)
{
    QJsonObject params;
    params["tx_data_hex"] = tx_data_hex;

    QJsonObject data = makeRequest("submit_transfer", params);
    if(data.contains("error")) {
        QJsonObject error = data["error"].toObject();
        return SubmitTransferResult(true, error["code"].toInt(), error["message"].toString());
    }
    SubmitTransferResult out;
    if(data.contains("result")) {
        QJsonObject result = data["result"].toObject();
        if(result.contains("tx_hash_list")) {
            QJsonArray list = result["tx_hash_list"].toArray();
            for(int i=0;i<list.count();i++)
                out.tx_hash_list.append(list[i].toString());
        }
    }
    return out;
}

TransferByTxIdResult WalletJsonRpc::getTransferByTxId(const QString &txid, unsigned int account_index) {
    qDebug() << "getTransferByTxId" << txid << ", " << account_index << ")";
    QJsonObject params;
    params["txid"] = txid;
    if(account_index != UINT_MAX)
        params["account_index"] = static_cast<qint64>(account_index);

    qDebug() << "getTransferByTxId: params: " << params;
    QJsonObject data = makeRequest("get_transfer_by_txid", params);
    qDebug() << "getTransferByTxId: data: " << data;

    if(data.contains("error")) {
        QJsonObject error = data["error"].toObject();
        return TransferByTxIdResult(true, error["code"].toInt(), error["message"].toString());
    }

    qDebug() << "getTransferByTxId: result: " << data;

    if(data.contains("result")) {
        TransferByTxIdResult out;
        QJsonObject result = data["result"].toObject();
        if(result.contains("transfer"))
            out.transfer = this->fromTransferObject(result["transfer"].toObject());
        if(result.contains("transfers")) {
            QJsonArray transfers = result["transfers"].toArray();
            for(int i=0; i<transfers.count(); i++)
                out.transfers.append(this->fromTransferObject(transfers[i].toObject()));
        }
	return out;
    }
    return TransferByTxIdResult(true, 0, "WTF just happend here in WalletJsonRpc?");
}

TransferType WalletJsonRpc::strToTransferType(const QString& str) {
    if (TRANSFER_TYPE.contains(str))
        return TRANSFER_TYPE.value(str);
    return TransferType::Unknown;
}

Transfer WalletJsonRpc::fromTransferObject(QJsonObject transfer) {

    QJsonObject subaddrIdx = transfer["subaddr_index"].toObject();

    Transfer out = Transfer(
        transfer["address"].toString(),
        transfer["amount"].toInteger(),
        transfer["confirmations"].toInteger(),
        transfer["double_spend_seen"].toBool(),
        transfer["fee"].toInteger(),
        transfer["height"].toInteger(),
        transfer["locked"].toBool(),
        transfer["payment_id"].toString(),
        SubAddressIndex(subaddrIdx["major"].toInteger(), subaddrIdx["major"].toInteger()),
        transfer["suggested_confirmations_threshold"].toInteger(),
        transfer["timestamp"].toInteger(),
        transfer["txid"].toString(),
        strToTransferType(transfer["type"].toString()),
        transfer["unlock_time"].toInteger()
        );
    if(transfer.contains("destinations")) {
        QJsonArray jdestinations = transfer["destinations"].toArray();
        for(int i=0;i<jdestinations.count(); i++) {
            QJsonObject jdest = jdestinations[i].toObject();
            out.destinations.append(Destination(jdest["amount"].toInteger(), jdest["address"].toString()));
        }
    }
    QJsonArray jamounts = transfer["amounts"].toArray();
    for(int i=0; i<jamounts.count();i++)
        out.amounts.append(jamounts[i].toInteger());
    return out;
};

bool WalletJsonRpc::stopWallet()
{
    return makeRequest("stop_wallet", QJsonObject()).contains("result");
}

ExportOutputsResult WalletJsonRpc::exportOutputs(bool all)
{
    QJsonObject params;
    params["all"] = all;

    QJsonObject data = makeRequest("export_outputs", params);

    if(data.contains("error")) {
        QJsonObject error = data["error"].toObject();
        return ExportOutputsResult(true, error["code"].toInt(), error["message"].toString());
    }
    if(data.contains("result")) {
        QJsonObject result = data["result"].toObject();
        return ExportOutputsResult(result["outputs_data_hex"].toString());
    }
    return ExportOutputsResult(true, 0, "WTF just happend in WalletJsonRpc?"); // shouldn't reah here, bt who knows...
}

QString WalletJsonRpc::exportSimpleOutputs(bool all)
{
    ExportOutputsResult eor = this->exportOutputs(all);
    if(eor.error)
        return "";
    return eor.outputs_data_hex;
}

KeyImageImportResult WalletJsonRpc::importKeyImages(const QString &encrypted_key_images_blob, unsigned int offset)
{
    qDebug() << "rpc:import_key_images: " << encrypted_key_images_blob << " offset: " << offset;
    QJsonObject params;
    if(offset != -1)
            params["offset"] = static_cast<int>(offset);
    params["encrypted_key_images_blob"] = encrypted_key_images_blob;

    QJsonObject result;
    QJsonObject data = makeRequest("import_encrypted_key_images", params);
    qDebug() << "rpc:import_key_images: result: " << data;
    if(data.contains("error")) {
        QJsonObject error = data["error"].toObject();
        return KeyImageImportResult(true, error["code"].toInt(), error["message"].toString());
    }

    if (data.contains("result")) {
        result = data["result"].toObject();
        return KeyImageImportResult(
            result["height"].toInt(),
            result["spent"].toVariant().toULongLong(),
            result["unspent"].toVariant().toULongLong()
            );
    }
    return KeyImageImportResult(true, 0, "WTF happenend in WalletJsonRpc?"); // shouldn't reach here stupid!
}

RefreshResult WalletJsonRpc::refresh(unsigned int start_height)
{
    QJsonObject params;
    params["start_height"] = static_cast<int>(start_height);

    QJsonObject data = makeRequest("refresh", params);
    RefreshResult out = RefreshResult();
    if(data.contains("result")) {
        QJsonObject result = data["result"].toObject();
        out.blocks_fetched = data["blocks_fetched "].toInt();
        out.received_money = data["received_money "].toBool();
    }
    return out;
}

bool WalletJsonRpc::rescanSpent()
{
    return makeRequest("rescan_spent", QJsonObject()).contains("result");
}

LoadWalletResult WalletJsonRpc::loadWallet(
    unsigned int restore_height,
    const QString &filename,
    const QString &address,
    const QString &viewkey
    )
{
    QJsonObject params;
    params["restore_height"] = static_cast<int>(restore_height);
    params["filename"] = filename;
    params["address"] = address;
    params["viewkey"] = viewkey;
    params["password"] = "";
    params["autosave_current"] = false;

    QJsonObject data = makeRequest("generate_from_keys", params);
    LoadWalletResult out = LoadWalletResult();
    if(data.contains("result")) {
        QJsonObject result = data["result"].toObject();
        out.address = result["address"].toString();
        out.info = result["info"].toString();
    }
    if(data.contains("error")) {
        out.error = true;
        out.error_message = data["error"].toString();
    }
    return out;
}

bool WalletJsonRpc::closeWallet()
{
    return makeRequest("close_wallet", QJsonObject()).contains("result");
}

QString WalletJsonRpc::getVersion()
{
    QJsonObject data = makeRequest("get_version", QJsonObject());

    QJsonObject resultObject = data["result"].toObject();
    int version = resultObject["version"].toInt();
    int major = (version >> 16) & 0xFF;
    int minor = version & 0xFF;
    return QString("%1.%2").arg(major).arg(minor);
}

EstimateSizeWeightResult WalletJsonRpc::estimateTxSizeAndWeight(
    unsigned int n_inputs,
    unsigned int n_outputs,
    unsigned int ring_size,
    bool rct
    )
{
    QJsonObject params;
    params["n_inputs"] = static_cast<int>(n_inputs);
    params["n_outputs"] = static_cast<int>(n_outputs);
    params["ring_size"] = static_cast<int>(ring_size);
    params["rct"] = rct;

    QJsonObject data = makeRequest("estimate_tx_size_and_weight", params);
    EstimateSizeWeightResult out = EstimateSizeWeightResult();
    if(data.contains("result")) {
        QJsonObject result = data["result"].toObject();
        out.size = result["size"].toInt();
        out.weight = result["weight"].toInt();
    }
    return out;
}


QString WalletJsonRpc::getFingerprint()
{
    std::string addressStr = this->getAddress().address.toStdString();
    QByteArray hash = QCryptographicHash::hash(QByteArray::fromStdString(addressStr), QCryptographicHash::Sha256);
    QString hashHex = hash.toHex();
    return hashHex.right(6).toUpper();
}
