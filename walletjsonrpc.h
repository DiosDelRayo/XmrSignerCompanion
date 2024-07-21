#include <climits>
#ifndef WALLETJSONRPC_H
#define WALLETJSONRPC_H

#define HEIGHT_ERROR UINT_MAX
#define INVALID_FEE UINT_MAX

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QAuthenticator>
#include <QList>

struct Error {
    bool error = false;
    int code = 0;
    QString error_message;

    Error() {};
    Error(QString _msg, int _code = 0, bool _error = true): error(_error), code(_code), error_message(_msg) {};
};

struct ExportOutputsResult: public Error {
    QString outputs_data_hex;

    ExportOutputsResult() {};
    ExportOutputsResult(QString _outputs): outputs_data_hex(_outputs) {};
    ExportOutputsResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {}
};

struct HeightResult: public Error {
    unsigned int height;

    HeightResult() {};
    HeightResult(unsigned int _height): height(_height) {};
    HeightResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {}
};

struct EstimateSizeWeightResult: public Error {
    int size = 0;
    int weight = 0;

    EstimateSizeWeightResult() {};
    EstimateSizeWeightResult(int _size, int _weight):
        size(_size),
        weight(_weight) {};
    EstimateSizeWeightResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {};
};

struct LoadWalletResult: public Error {
    QString address;
    QString info;

    LoadWalletResult() {}
    LoadWalletResult(QString _address, QString _info): address(_address), info(_info) {}
    LoadWalletResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {}
};

struct RefreshResult: public Error {
    unsigned int blocks_fetched = 0;
    bool received_money = false;

    RefreshResult() {};
    RefreshResult(unsigned int _blocks_fetched, bool _received_money): blocks_fetched(_blocks_fetched), received_money(_received_money) {};
    RefreshResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {}
};

struct KeyImageImportResult: public Error {
    unsigned int height = 0;
    unsigned int spent = 0;
    unsigned int unspent = 0;

    KeyImageImportResult() {};
    KeyImageImportResult(unsigned int _height, unsigned int _spent, unsigned int _unspent): height(_height), spent(_spent), unspent(_unspent) {};
    KeyImageImportResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {}
};

struct TransferResult: public Error {
    unsigned int amount = 0;
    unsigned int fee = 0;
    QString multisig_txset;
    QString tx_blob;
    QString tx_hash;
    QString tx_key;
    QString unsigned_txset;
    QList<QString> tx_metadata;

    TransferResult() {};
    TransferResult(
        unsigned int _amount,
        unsigned int _fee,
        QString _multisig_txset,
        QString _tx_blob,
        QString _tx_hash,
        QString _tx_key,
        QString _unsigned_txset
        ):
        amount(_amount),
        fee(_fee),
        multisig_txset(_multisig_txset),
        tx_blob(_tx_blob),
        tx_hash(_tx_hash),
        tx_key(_tx_key),
        unsigned_txset(_unsigned_txset)
        {};
    TransferResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {}
};

struct SubmitTransferResult: public Error {
    QList<QString> tx_hash_list;

    SubmitTransferResult() {}
    SubmitTransferResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {}
};

struct SubaddressBalance {
    unsigned int account_index = 0;
    unsigned int address_index = 0;
    QString address;
    unsigned int balance = 0;
    unsigned int unlocked_balance = 0;
    QString label;
    unsigned int num_unspent_outputs = 0;
    int time_to_unlock = 0;
    int blocks_to_unlock = 0;

    SubaddressBalance() {};
    SubaddressBalance(
        unsigned int _account_index,
        unsigned int _address_index,
        QString _address,
        unsigned int _balance,
        unsigned int _unlocked_balance,
        QString _label,
        unsigned int _num_unspent_outputs,
        int _time_to_unlock,
        int _blocks_to_unlock
        ):
        account_index(_account_index),
        address_index(_address_index),
        address(_address),
        balance(_balance),
        unlocked_balance(_unlocked_balance),
        label(_label),
        num_unspent_outputs(_num_unspent_outputs),
        time_to_unlock(_time_to_unlock),
        blocks_to_unlock(_blocks_to_unlock)
        {};
};

struct BalanceResult: public Error {
    unsigned int balance = 0;
    unsigned int unlocked_balance = 0;
    bool multisig_import_needed = false;
    int time_to_unlock = 0;
    int blocks_to_unlock = 0;
    QList<SubaddressBalance> per_subaddress;

    BalanceResult() {};
    BalanceResult(
        unsigned int _balance,
        unsigned int _unlocked_balance,
        bool _multisig_import_needed,
        int _time_to_unlock,
        int _blocks_to_unlock
        ):
        balance(_balance),
        unlocked_balance(_unlocked_balance),
        multisig_import_needed(_multisig_import_needed),
        time_to_unlock(_time_to_unlock),
        blocks_to_unlock(_blocks_to_unlock)
        {};
    BalanceResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {}
};

struct ValidateAddressResult: public Error {
    bool valid = false;
    bool integrated = false;
    bool subaddress = false;
    QString nettype;
    QString openalias_address;

    ValidateAddressResult() {};
    ValidateAddressResult(bool _valid, bool _integrated, bool _subaddress, QString _nettype, QString _openalias_address):
        valid(_valid),
        integrated(_integrated),
        subaddress(_subaddress),
        nettype(_nettype),
        openalias_address(_openalias_address)
        {};
    ValidateAddressResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {}
};

struct SubAddress {
    QString address;
    QString label;
    unsigned int address_index = 0;
    bool used = false;

    SubAddress() {};
    SubAddress(QString _address, QString _label, unsigned int _address_index, bool _used):
        address(_address),
        label(_label),
        address_index(_address_index),
        used(_used)
        {};
};

struct AddressResult: Error {
    QString address;
    QList<SubAddress> addresses;

    AddressResult() {};
    AddressResult(QString _address): address(_address) {};
    AddressResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {}
};

struct Destination {
    unsigned int amount = 0;
    QString address;

    Destination() {};
    Destination(unsigned int _amount, QString _address): amount(_amount), address(_address) {};
};

struct SubAddressIndex {
    unsigned int major;
    unsigned int minor;

    SubAddressIndex() {};
    SubAddressIndex(unsigned int _major, unsigned int _minor): major(_major), minor(_minor) {};
};

enum class TransferType {
    In,
    Out,
    Pending,
    Failed,
    Pool,
    Unknown
};

const QMap<QString, TransferType> TRANSFER_TYPE {
	{ "in", TransferType::In },
		{ "out", TransferType::Out },
		{ "pending", TransferType::Pending },
		{ "failed", TransferType::Failed },
		{ "pool", TransferType::Pool }
};

struct Transfer {
    QString address;
    unsigned int amount;
    QList<unsigned int> amounts;
    unsigned int confirmations;
    QList<Destination> destinations;
    bool double_spend_seen;
    unsigned int fee;
    unsigned int height;
    bool locked;
    QString payment_id;
    SubAddressIndex subaddr_index;
    unsigned int suggested_confirmations_threshold;
    unsigned int timestamp;
    QString txid;
    TransferType type;
    unsigned int unlock_time;

    Transfer() {};
    Transfer(
        QString _address,
        unsigned int _amount,
        unsigned int _confirmations,
        bool _double_spend_seen,
        unsigned int _fee,
        unsigned int _height,
        bool _locked,
        QString _payment_id,
        SubAddressIndex _subaddr_index,
        unsigned int _suggested_confirmations_threshold,
        unsigned int _timestamp,
        QString _txid,
        TransferType _type,
        unsigned int _unlock_time
        ):
        address(_address),
        amount(_amount),
        confirmations(_confirmations),
        double_spend_seen(_double_spend_seen),
        fee(_fee),
        height(_height),
        locked(_locked),
        payment_id(_payment_id),
        subaddr_index(_subaddr_index),
        suggested_confirmations_threshold(_suggested_confirmations_threshold),
        timestamp(_timestamp),
        txid(_txid),
        type(_type),
        unlock_time(_unlock_time)
        {};
};

struct TransferByTxIdResult: Error {
    Transfer transfer;
    QList<Transfer> transfers;

    TransferByTxIdResult() {}
    TransferByTxIdResult(bool _error, int _code, QString _error_message): Error(_error_message, _code, _error) {}
};

class WalletJsonRpc : public QObject
{
    Q_OBJECT

public:
    explicit WalletJsonRpc(
        QObject *parent = nullptr,
        const QString& host = "localhost",
        int port = 18082,
        bool tls = false
        );
    void setAuthentication(const QString& username, const QString& password);
    bool setDaemon(
        const QString &address,
        bool trusted = false,
        const QString &ssl_support = "autodetect",
        const QString &ssl_private_key_path = "",
        const QString &ssl_certificate_path = "",
        const QString &ssl_ca_file = "",
        const QStringList &ssl_allowed_fingerprints = QStringList(),
        bool ssl_allow_any_cert = false,
        const QString &username = "",
        const QString &password = ""
        );

    BalanceResult getBalance(
        unsigned int account_index = 0,
        const QList<unsigned int> &address_indices = QList<unsigned int>(),
        bool all_accounts = true,
        bool strict = false
        );

    AddressResult getAddress(
        unsigned int account_index = 0,
        const QList<unsigned int> &address_indices = QList<unsigned int>()
        );

    ValidateAddressResult validateAddress(
        const QString &address,
        bool any_net_type = false,
        bool allow_openalias = false
        );

    HeightResult getHeight();
    unsigned int getSimpleHeight();

    TransferResult transfer(
        const QList<Destination> &destinations,
        unsigned int account_index = 0,
        const QList<unsigned int> &subaddr_indices = QList<unsigned int>(),
        unsigned int priority = 0,
        unsigned int ring_size = 7,
        unsigned int unlock_time = 0,
        bool get_tx_key = false,
        bool do_not_relay = false,
        bool get_tx_hex = false,
        bool get_tx_metadata = false
        );

    SubmitTransferResult submitTransfer(const QString &tx_data_hex);
    bool stopWallet();
    ExportOutputsResult exportOutputs(bool all = true);
    QString exportSimpleOutputs(bool all = true);
    KeyImageImportResult importKeyImages(
        const QJsonArray &encrypted_key_images_blob,
        unsigned int offset = -1
        );

    TransferByTxIdResult getTransferByTxId(QString txid, unsigned int account_index = UINT_MAX);

    KeyImageImportResult importKeyImagesFromByteString(
        const std::string& data,
        unsigned int offset = -1
        );
    RefreshResult refresh(unsigned int start_height = 0);
    bool rescanSpent();
    LoadWalletResult loadWallet(
        unsigned int restore_height,
        const QString &filename,
        const QString &address,
        const QString &viewkey
        );
    bool closeWallet();
    QString getVersion();
    EstimateSizeWeightResult estimateTxSizeAndWeight(
        unsigned int n_inputs,
        unsigned int n_outputs,
        unsigned int ring_size,
        bool rct
        );
    QString getFingerprint();
    TransferType strToTransferType(const QString& str);

private:
    QJsonObject makeRequest(const QString &method, const QJsonObject &params);
    QJsonArray parseKeyImages(const std::string& binaryData);
    Transfer fromTransferObject(QJsonObject transfer);
    QString m_host;
    int m_port;
    bool m_tls;
    QNetworkAccessManager* m_networkManager;
    QUrl m_url;
    QString m_username;
    QString m_password;

private slots:
    void handleAuthenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
};

#endif // WALLETJSONRPC_H
