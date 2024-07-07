#include "walletrpcmanager.h"
#include <QJsonDocument>
#include <QNetworkRequest>
#include <QEventLoop>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

QString WalletRpcManager::getExecutableDir() {
    // Get the absolute path of the current executable
    QString executablePath = QCoreApplication::applicationFilePath();

    // Extract the directory part of the executable path
    QDir executableDir(executablePath);
    return executableDir.absolutePath();
}

QString WalletRpcManager::findShippedWalletRpc(const QString &executableName) {
    QString currentDir = getExecutableDir();

    QDir dir(currentDir);
    QStringList files = dir.entryList(QStringList(executableName), QDir::Files);

    if (!files.isEmpty()) {
        QString executablePath = QDir::toNativeSeparators(dir.absoluteFilePath(files.first()));
        return executablePath;
    }

    return QString();
}

WalletRpcManager::WalletRpcManager(
    QObject *parent,
    const QString& walletRpcPath,
    const QString& network,
    int rpcPort)
    : QObject(parent)
    , m_walletRPCProcess(new QProcess(this))
    , m_walletRPCPath(walletRpcPath)  // Set this to the actual path
    , m_network(network.toLower())
    , m_rpcPort(rpcPort)  // Default Monero RPC port, adjust if needed
    , m_networkManager(new QNetworkAccessManager(this))
{
    setupProcess();
}

WalletRpcManager::~WalletRpcManager()
{
    stopWalletRPC();
}

void WalletRpcManager::setupProcess()
{
    connect(m_walletRPCProcess, &QProcess::errorOccurred, this, &WalletRpcManager::handleProcessError);
    connect(m_walletRPCProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &WalletRpcManager::handleProcessFinished);
}

bool WalletRpcManager::startWalletRPC()
{
    if (isRunning()) {
        return true;
    }

    QStringList arguments;
    arguments << "--rpc-bind-port" << QString::number(m_rpcPort);
    arguments << "--rpc-login" << "wallet:wallet";
    arguments << "--wallet-dir" << "/tmp";
    if (m_network == "testnet") {
        arguments << "--testnet";
    } else if (m_network == "stagenet") {
        arguments << "--stagenet";
    }
    qDebug() << "start " << m_walletRPCPath << arguments.join(" ");
    m_walletRPCProcess->start(m_walletRPCPath, arguments);

    if (!m_walletRPCProcess->waitForStarted()) {
        qDebug() << "Failed to start wallet-rpc process";
        emit error("Failed to start wallet-rpc process");
        return false;
    }

    qDebug() << "wallet rpc started";
    emit walletRPCStarted();
    return true;
}

void WalletRpcManager::stopWalletRPC()
{
    if (isRunning()) {
        m_walletRPCProcess->terminate();
        if (!m_walletRPCProcess->waitForFinished(5000)) {  // 5 second timeout
            m_walletRPCProcess->kill();
        }
        emit walletRPCStopped();
    }
}

bool WalletRpcManager::isRunning() const
{
    return m_walletRPCProcess->state() == QProcess::Running;
}

QJsonObject WalletRpcManager::makeRPCCall(const QString &method, const QJsonObject &params)
{
    QJsonObject request;
    request["jsonrpc"] = "2.0";
    request["id"] = "0";
    request["method"] = method;
    request["params"] = params;

    QNetworkRequest networkRequest(QUrl(QString("http://localhost:%1/json_rpc").arg(m_rpcPort)));
    networkRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QNetworkReply *reply = m_networkManager->post(networkRequest, QJsonDocument(request).toJson());

    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();

    if (reply->error() == QNetworkReply::NoError) {
        QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
        reply->deleteLater();
        return response;
    } else {
        emit error(QString("Network error: %1").arg(reply->errorString()));
        reply->deleteLater();
        return QJsonObject();
    }
}

void WalletRpcManager::handleProcessError(QProcess::ProcessError error)
{
    QString errorMessage;
    switch (error) {
        case QProcess::FailedToStart:
            errorMessage = "The wallet-rpc process failed to start.";
            break;
        case QProcess::Crashed:
            errorMessage = "The wallet-rpc process crashed.";
            break;
        default:
            errorMessage = "An error occurred with the wallet-rpc process.";
    }
        qDebug() << errorMessage;
    emit this->error(errorMessage);
}

void WalletRpcManager::handleProcessFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
    if (exitStatus == QProcess::CrashExit) {
        qDebug() << "The wallet-rpc process crashed.";
        emit error("The wallet-rpc process crashed.");
    } else if (exitCode != 0) {
        qDebug() << QString("The wallet-rpc process exited with code %1.").arg(exitCode);
        emit error(QString("The wallet-rpc process exited with code %1.").arg(exitCode));
    }
    emit walletRPCStopped();
}
