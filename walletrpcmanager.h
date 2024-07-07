#ifndef WALLETRPCMANAGER_H
#define WALLETRPCMANAGER_H

#include <QObject>
#include <QProcess>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class WalletRpcManager : public QObject
{
    Q_OBJECT

public:
    explicit WalletRpcManager(
        QObject *parent = nullptr,
        const QString& walletRpcPath = nullptr,
        const QString& walletDirPath = nullptr,
        const QString& network = "mainnet",
        int rpcPort = 18082
        );
    ~WalletRpcManager();

    bool startWalletRPC();
    void stopWalletRPC();
    bool isRunning() const;

    QJsonObject makeRPCCall(const QString &method, const QJsonObject &params);
    static QString findShippedWalletRpc(const QString &executableName);
    static QString getExecutableDir();

signals:
    void walletRPCStarted();
    void walletRPCStopped();
    void error(const QString &errorMessage);

private slots:
    void handleProcessError(QProcess::ProcessError error);
    void handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);

private:
    void setupProcess();

    QProcess *m_walletRPCProcess;
    QString m_walletRPCPath;
    QString m_walletDirPath;
    QString m_network;
    int m_rpcPort;
    QNetworkAccessManager *m_networkManager;
};

#endif // WALLETRPCMANAGER_H
