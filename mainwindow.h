#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMap>
#include <QString>
#include <QMainWindow>
#include <QTemporaryDir>
#include <QRegularExpressionValidator>
#include "daemonjsonrpc.h"
#include "walletrpcmanager.h"
#include "walletjsonrpc.h"
#include "qrcode/scanner/QrCodeScanWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

#define SPENDABLE_TRESHOLD 0

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    bool checkLogic();
    QString primaryAddress = nullptr;
    QString privateViewKey = nullptr;
    QString daemon_url = nullptr;
    QTemporaryDir *tempDir = nullptr;
    QRegularExpressionValidator *nodeAddressValidator;
    QRegularExpressionValidator *nodePortValidator;
    QRegularExpressionValidator *addressValidator;
    QRegularExpressionValidator *amountRegexValidator;
    QTimer *lookupTimer;
    void setupNodeInputs();
    void checkNodeAddress();
    void checkNodePort();
    void walletSyncProgress(bool active, int periodInMilliseconds = 5000);
    void setupSendXmrInputs();
    void checkAddress();
    void checkAmount();
    void updateSendButtonState();
    unsigned int estimateTotalTransfer();
    double getAmountValue();

    int restoreHeight = 0;
    WalletRpcManager *walletRpcManager = nullptr;
    WalletJsonRpc *walletRpc = nullptr;
    DaemonJsonRpc *daemonRpc = nullptr;
    void checkWalletRpcConnection(int attempts = 10, int delayBetweenAttempts = 500);
    void loadWallet();
    bool isViewOnlyWallet(const QString& qrCode);
    void removeQrCodeScanWidgetFromUi(QrCodeScanWidget *&widget);
    QString getWalletFile();
    QString getWalletDirectory();
    QString getWalletPath();
    void removeWalletFiles();
    void checkNodeUrl();
    void updateWalletSyncProgress();
    void sendXmrPage();
    void updateAvailableXmr();
    void syncAvailableXmr(bool active, int periodInMilliseconds = 5000);
    void insufficientFunds(); // should give access to the addresses...
    QString relativeXmr(unsigned int amount);
    QString relativeTimeFromMilliseconds(int milliseconds);
    QString relativeBlocksPerSecond(int blocks);
    QString getFingerprint();
    Ui::MainWindow *ui;

    const QMap<QChar, QString> NETWORK = {
        {'4', "mainnet"},
        {'9', "testnet"},
        {'5', "stagenet"}
    };

    const QMap<QString, int> NETWORK_PORT = {
        {"mainnet", 18081},
        {"testnet", 28081},
        {"stagenet", 38081}
    };

    QString network = NETWORK['4'];
    int monerodPort = 0;
    unsigned int availableBalance = 0;

signals:
    void loadingWalletRpc(int estimatedMilliseconds);
    void walletRpcConnected();
    void walletRpcConnectionFailed();
    void waitForWalletRpcConnection(int millisecondsLeft);
    void waitForWalletRpc();
    void waitForWallet();
    void loadingWallet(int estimatedMilliseconds);
    void walletLoaded();
    void walletError();
    void shutdownRequested();
    void validViewWalletQr();
    void invalidQr();

private slots:
    void syncDotIndicator(int index);
    void onViewWalletScanFinished(bool successful);
    void onKeyImagesScanFinished(bool successful);
    void onWalletRpcStarted();
    void onWalletRpcStopped();
    void onWalletRpcError();
    void onWalletRpcReady();
    void awaitWalletRpcForMax(int milliseconds);
    void onWalletRpcFailed();
    void onWalletLoaded();
    void onWalletError();
    void onShutdown();
};
#endif // MAINWINDOW_H
