#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMap>
#include <QString>
#include <QMainWindow>
#include <QTemporaryDir>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include "daemonjsonrpc.h"
#include "walletrpcmanager.h"
#include "walletjsonrpc.h"
#include "qrcode/scanner/QrCodeScanWidget.h"

#define PAGE_SCAN_VIEW_WALLET 0
#define PAGE_CONNECT_DAEMON 1
#define PAGE_SYNC_WALLET 2
#define PAGE_QR_OUTPUTS 3
#define PAGE_SCAN_KEY_IMAGES 4
#define PAGE_SEND_XMR 5
#define PAGE_QR_UNSIGNED_TX 6
#define PAGE_SCAN_SIGNED_TX 7
#define PAGE_SUBMIT_TX 8
#define PAGE_CHANGE_BOCK_HEIGHT 9
#define PAGE_TRANSFER_FAILED 10


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
    bool checkLogic(int currentScreen, int nextScreen);
    QString primaryAddress = nullptr;
    QString privateViewKey = nullptr;
    QString unsignedTransaction = nullptr;
    bool lockedUnsignedTransaction = false;
    QList<QString> txIds;
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
    void next();
    void previous();
    void transactionsProgress(bool active, int periodInMilliseconds = 5000);
    void renderTxProgress();
    unsigned int estimateTotalTransfer();
    unsigned int getAmountValue();

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

    const QMap<QString, QRegularExpression> VALID_NETWORK_ADDRESS = {
        {"mainnet", QRegularExpression("^[48][1-9A-Za-z]{94}$")},
        {"testnet", QRegularExpression("^[ABab][1-9A-Za-z]{94}$")},
        {"stagenet", QRegularExpression("^[57][1-9A-Za-z]{94}$")}
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
    void onSignedTransactionScanFinished(bool successful);
    void onWalletRpcStarted();
    void onWalletRpcStopped();
    void onWalletRpcError();
    void onWalletRpcReady();
    void onWalletRpcFailed();
    void onWalletLoaded();
    void onWalletError();
    void onShutdown();
};

class PortValidator : public QValidator
{
public:
    PortValidator(QObject *parent = nullptr);

    State validate(QString &input, int &pos) const override;
};

#endif // MAINWINDOW_H
