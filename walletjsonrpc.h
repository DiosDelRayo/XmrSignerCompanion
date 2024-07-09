#ifndef WALLETJSONRPC_H
#define WALLETJSONRPC_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QAuthenticator>

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
    QJsonObject setDaemon(const QString &address, bool trusted = false, const QString &ssl_support = "autodetect",
                          const QString &ssl_private_key_path = "", const QString &ssl_certificate_path = "",
                          const QString &ssl_ca_file = "", const QStringList &ssl_allowed_fingerprints = QStringList(),
                          bool ssl_allow_any_cert = false, const QString &username = "", const QString &password = "");

    QJsonObject getBalance(unsigned int account_index, const QList<unsigned int> &address_indices = QList<unsigned int>(),
                           bool all_accounts = false, bool strict = false);

    QJsonObject getAddress(unsigned int account_index, const QList<unsigned int> &address_indices = QList<unsigned int>());

    QJsonObject validateAddress(const QString &address, bool any_net_type = false, bool allow_openalias = false);

    int getHeight();

    QJsonObject transfer(const QJsonArray &destinations, unsigned int account_index = 0,
                         const QList<unsigned int> &subaddr_indices = QList<unsigned int>(),
                         unsigned int priority = 0, unsigned int ring_size = 7, unsigned int unlock_time = 0,
                         bool get_tx_key = false, bool do_not_relay = false, bool get_tx_hex = false,
                         bool get_tx_metadata = false);

    QJsonObject submitTransfer(const QString &tx_data_hex);
    QJsonObject stopWallet();
    QString exportOutputs(bool all = true);
    QJsonObject importKeyImages(unsigned int offset, const QJsonArray &signed_key_images);
    QJsonObject refresh(unsigned int start_height = 0);
    QJsonObject rescanSpent();
    QJsonObject loadWallet(
        unsigned int restore_height,
        const QString &filename,
        const QString &address,
        const QString &viewkey
        );
    QJsonObject closeWallet();
    QString getVersion();
    QJsonObject estimateTxSizeAndWeight(unsigned int n_inputs, unsigned int n_outputs, unsigned int ring_size, bool rct);
    QString getFingerprint();

private:
    QJsonObject makeRequest(const QString &method, const QJsonObject &params);
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
