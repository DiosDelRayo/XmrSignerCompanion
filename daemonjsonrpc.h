#ifndef DAEMONJSONRPC_H
#define DAEMONJSONRPC_H

#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkAccessManager>
#include <QAuthenticator>

struct InfoResult {
    unsigned int adjusted_time;
    unsigned int alt_blocks_count;
    unsigned int block_size_limit;
    unsigned int block_size_median;
    unsigned int block_weight_limit;
    unsigned int block_weight_median;
    QString bootstrap_daemon_address;
    bool busy_syncing;
    unsigned int credits;
    unsigned int cumulative_difficulty;
    unsigned int cumulative_difficulty_top64;
    unsigned int database_size;
    unsigned int difficulty;
    unsigned int difficulty_top64;
    unsigned int free_space;
    unsigned int grey_peerlist_size;
    unsigned int height;
    unsigned int height_without_bootstrap;
    unsigned int incoming_connections_count;
    bool mainnet;
    QString nettype;
    bool offline;
    unsigned int outgoing_connections_count;
    unsigned int rpc_connections_count;
    bool stagenet;
    unsigned int start_time;
    QString status;
    bool synchronized;
    unsigned int target;
    unsigned int target_height;
    bool testnet;
    QString top_block_hash;
    QString top_hash;
    unsigned int tx_count;
    unsigned int tx_pool_size;
    bool untrusted;
    bool update_available;
    QString version;
    bool was_bootstrap_ever_used;
    unsigned int white_peerlist_size;
    QString wide_cumulative_difficulty;
    QString wide_difficulty;
};

class DaemonJsonRpc : public QObject
{
    Q_OBJECT

public:
    explicit DaemonJsonRpc(
        QObject *parent = nullptr,
        const QString& host = "localhost",
        int port = 18081,
        bool tls = false
        );
    void setAuthentication(const QString& username, const QString& password);
    int getHeight();
    int getLocalHeight();
    int getSynced(); // percentage 0...100 of the sync status
    InfoResult getInfo(bool cached = true, int acceptable_cache_seconds = 30);
    QString getVersion();

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

#endif // DAEMONJSONRPC_H
