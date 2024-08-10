#include "mainwindow.h"
#include "qrcode/scanner/QrCodeScanWidget.h"
#include "./ui_mainwindow.h"
#include "networkchecker.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QUrlQuery>
#include <QTimer>
#include <QDir>
#include <QFile>
#include <QCoreApplication>
#include <QCloseEvent>
#include <QHostInfo>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QCryptographicHash>
#include <QByteArray>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &MainWindow::onShutdown);
    connect(this, &MainWindow::shutdownRequested, this, &MainWindow::onShutdown);
    ui->setupUi(this);
    setupNodeInputs();

    ui->prevButton->setDisabled(true);
    ui->stackedWidget->setCurrentIndex(0);
    this->syncDotIndicator(0);

    ui->dotIndicator->setTotalSteps(ui->stackedWidget->count());  // Set the total number of steps
    ui->dotIndicator->setCurrentStep(0);  // Set initial step
    ui->dotIndicator->setFixedSize(200, 20);  // Adjust size as needed
    ui->nextButton->setDisabled(true);

    connect(ui->nextButton, &QPushButton::clicked, this, [this]() {
        this->next();
    });

    connect(ui->prevButton, &QPushButton::clicked, this, [this]() {
        this->previous();
    });

    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &MainWindow::syncDotIndicator);
    connect(ui->scanViewWallet, &QrCodeScanWidget::finished, this, &MainWindow::onViewWalletScanFinished);
    connect(ui->scanKeyImages, &QrCodeScanWidget::finished, this, &MainWindow::onKeyImagesScanFinished);
    connect(ui->scanSignedTransaction, &QrCodeScanWidget::finished, this, &MainWindow::onSignedTransactionScanFinished);

    connect(this, &MainWindow::walletRpcConnected, this, &MainWindow::onWalletRpcReady);
    connect(this, &MainWindow::walletRpcConnectionFailed, this, &MainWindow::onWalletRpcFailed);
    connect(this, &MainWindow::walletLoaded, this, &MainWindow::onWalletLoaded);
    connect(this, &MainWindow::walletError, this, &MainWindow::onWalletError);

    connect(this, &MainWindow::validViewWalletQr, ui->scanViewWallet, &QrCodeScanWidget::onFrameStateValidated);
    connect(this, &MainWindow::invalidQr, ui->scanViewWallet, &QrCodeScanWidget::onFrameStateError);
    connect(this, &MainWindow::loadingWalletRpc, ui->scanViewWallet, &QrCodeScanWidget::onFrameStateProcessing);
    connect(this, &MainWindow::loadingWallet, ui->scanViewWallet, &QrCodeScanWidget::onFrameStateProcessing);
}

void MainWindow::next() {
    int currentIndex = ui->stackedWidget->currentIndex();
    int nextIndex = (currentIndex + 1) % ui->stackedWidget->count(); // Loop back to the first widget if at the last
    // Check logic before advancing to next step
    if (checkLogic(currentIndex, nextIndex)) {
        ui->stackedWidget->setCurrentIndex(nextIndex);
        // ui->dotIndicator->setCurrentStep(nextIndex); // Sync QDotProgressIndicator
    }
}

void MainWindow::previous() {
    int currentIndex = ui->stackedWidget->currentIndex();
    int prevIndex = (ui->stackedWidget->count() + currentIndex - 1) % ui->stackedWidget->count(); // Loop to last widget if at the first
    // Check logic before going back to previous step
    if (checkLogic(currentIndex, prevIndex)) {
        ui->stackedWidget->setCurrentIndex(prevIndex);
        ui->dotIndicator->setCurrentStep(prevIndex); // Sync QDotProgressIndicator
    }
}

void MainWindow::setupNodeInputs()
{
    // Setup address input
    QRegularExpression rx(
        "^("
        // IPv4
        "((25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\\.){3}(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)"
        "|"
        // IPv6
        "(([0-9a-fA-F]{1,4}:){7,7}[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,7}:|([0-9a-fA-F]{1,4}:){1,6}:[0-9a-fA-F]{1,4}|([0-9a-fA-F]{1,4}:){1,5}(:[0-9a-fA-F]{1,4}){1,2}|([0-9a-fA-F]{1,4}:){1,4}(:[0-9a-fA-F]{1,4}){1,3}|([0-9a-fA-F]{1,4}:){1,3}(:[0-9a-fA-F]{1,4}){1,4}|([0-9a-fA-F]{1,4}:){1,2}(:[0-9a-fA-F]{1,4}){1,5}|[0-9a-fA-F]{1,4}:((:[0-9a-fA-F]{1,4}){1,6})|:((:[0-9a-fA-F]{1,4}){1,7}|:)|fe80:(:[0-9a-fA-F]{0,4}){0,4}%[0-9a-zA-Z]{1,}|::(ffff(:0{1,4}){0,1}:){0,1}((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])|([0-9a-fA-F]{1,4}:){1,4}:((25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9])\\.){3,3}(25[0-5]|(2[0-4]|1{0,1}[0-9]){0,1}[0-9]))"
        "|"
        // Domain name or local hostname
        "([a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?\\.)*[a-zA-Z0-9]([a-zA-Z0-9\\-]{0,61}[a-zA-Z0-9])?"
        ")$"
        );

    this->nodeAddressValidator = new QRegularExpressionValidator(rx, this);
    ui->nodeAddressEdit->setValidator(this->nodeAddressValidator);
    lookupTimer = new QTimer(this);
    lookupTimer->setSingleShot(true);
    lookupTimer->setInterval(500);

    connect(ui->nodeAddressEdit, &QLineEdit::textChanged, this, &MainWindow::checkNodeAddress);
    connect(lookupTimer, &QTimer::timeout, this, &MainWindow::checkNodeAddress);

    ui->nodePortEdit->setValidator(new PortValidator(ui->nodePortEdit));
    connect(ui->nodePortEdit, &QLineEdit::textChanged, this, &MainWindow::checkNodePort);
}

void MainWindow::checkNodeAddress()
{
    QString host = ui->nodeAddressEdit->text().trimmed();
    if (host.isEmpty()) {
        ui->nodeAddressEdit->setStyleSheet("QLineEdit { background-color: white; color: black; font-size: 36px; border-radius: 35px; padding: 5px; min-height: 45px; max-height: 45px; border: 10px solid transparent; }");
        return;
    }

    QHostInfo::lookupHost(host, this, [this](const QHostInfo &hostInfo) {
        bool ok = (hostInfo.error() == QHostInfo::NoError);
        ui->nodeAddressEdit->setStyleSheet(QString("QLineEdit { background-color: white; color: black; font-size: 36px; border-radius: 35px; padding: 5px; min-height: 45px; max-height: 45px; border: 10px solid %1; }").arg(ok?"green":"transparent"));
        if(!ok)
            ui->nextButton->setDisabled(true);
    });
}

void MainWindow::checkNodePort()
{
    qDebug() << "check node port (json connection";
    QString host = ui->nodeAddressEdit->text().trimmed();
    QString port = ui->nodePortEdit->text().trimmed();
    if (host.isEmpty() || port.isEmpty()) {
        ui->nodePortEdit->setStyleSheet("QLineEdit { background-color: white; color: black; font-size: 36px; border-radius: 35px; padding: 5px; min-height: 45px; max-height: 45px; border: 10px solid transparent; }");
        return;
    }

    // TODO: in tristate we should http and https...
    QString url = QString("http%1://%2:%3/json_rpc")
                        .arg(
                          ui->nodeTls->isChecked() ? "s" : "",
                          host,
                          port
                        );
    qDebug() << "check node url: " << url;

    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QJsonObject jsonObj;
    jsonObj["jsonrpc"] = "2.0";
    jsonObj["id"] = "0";
    jsonObj["method"] = "get_info";

    QJsonDocument doc(jsonObj);
    QByteArray data = doc.toJson();

    QNetworkReply *reply = manager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [this, reply, manager, url]() {
        bool ok = (reply->error() == QNetworkReply::NoError);

        qDebug() << "could connect to monerod node: " << (ok?"yes":"no");
        if(ok) {
            QByteArray response = reply->readAll();
            QJsonDocument jsonResponse = QJsonDocument::fromJson(response);
            QJsonObject jsonObject = jsonResponse.object();
            ok = ok        &&
                 jsonObject.contains("result") &&
                 jsonObject["result"].toObject().contains("status") &&
                 jsonObject["result"].toObject().contains("version");
        }
        qDebug() << "could connect to monerod response satisfying: " << (ok?"yes":"no");
        ui->nodePortEdit->setStyleSheet(QString("QLineEdit { background-color: white; color: black; font-size: 36px; border-radius: 35px; padding: 5px; min-height: 45px; max-height: 45px; border: 10px solid %1; }").arg(ok?"green":"transparent"));
        this->daemon_url = ok?url:nullptr;
        this->daemonRpc = new DaemonJsonRpc(this, ui->nodeAddressEdit->text(), ui->nodePortEdit->text().toInt(), ui->nodeTls->isChecked());
        ui->nextButton->setEnabled(ok);

        reply->deleteLater();
        manager->deleteLater();
    });
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    emit shutdownRequested();
    event->accept();
}

void MainWindow::syncDotIndicator(int index) {
    QString outputs;

    qDebug() << "set current step: " << index;
    ui->dotIndicator->setCurrentStep(index);

    switch (index) {
    case 0:
        ui->titleLabel->setText(QString("Import View Only Wallet"));
        ui->scanViewWallet->startCapture(false);
        break;
    case 1:
        ui->titleLabel->setText(QString("Connect"));
        ui->daemonNotFoundMessage->hide();
        ui->nextButton->setEnabled(false);
        checkNodeAddress();
        checkNodePort();
        break;
    case 2:
        ui->titleLabel->setText(QString("Synchronize Wallet"));
        if(this->daemon_url == nullptr)
            QApplication::exit(1);
        ui->fingerprintLabel->setText(QString("Fingerprint: %1").arg(this->getFingerprint()));
        ui->nextButton->setEnabled(false);
        this->walletRpc->setDaemon(this->daemon_url, true);
        this->walletSyncProgress(true);
        break;
    case 3:
        ui->titleLabel->setText(QString("Export Outputs"));
        outputs = this->walletRpc->exportSimpleOutputs();
        qDebug() << "outputs: " << outputs;
        ui->outputsUR->setData("xmr-output", QByteArray::fromHex(outputs.toLocal8Bit()).toStdString());
        this->walletSyncProgress(false);
        ui->nextButton->setEnabled(true);
        break;
    case 4:
        ui->titleLabel->setText(QString("Import Key Images"));
        ui->scanKeyImages->startCapture(true);
        ui->nextButton->setDisabled(true);
        break;
    case 5:
        ui->availableAmount->setText(QString("Available: %1").arg(this->relativeXmr(this->walletRpc->getBalance().unlocked_balance)));
        this->removeQrCodeScanWidgetFromUi(ui->scanKeyImages);
        this->setupSendXmrInputs();
        this->syncAvailableXmr(true);
        ui->titleLabel->setText(QString("Send XMR"));
        ui->nextButton->setDisabled(true);
        this->lockedUnsignedTransaction = false;
        this->unsignedTransaction = nullptr;
        break;
    case 6:
        if(!this->lockedUnsignedTransaction || this->unsignedTransaction== nullptr || this->unsignedTransaction.isEmpty())
            return;
        this->syncAvailableXmr(false);
        qDebug() << "send unsigned tx: " << this->unsignedTransaction;
        ui->unsignedTransactionUR->setData("xmr-txunsigned",QByteArray::fromHex(this->unsignedTransaction.toLocal8Bit()).toStdString());
        ui->titleLabel->setText(QString("Export Unsigned Transaction"));
        ui->nextButton->setEnabled(true);
        break;
    case 7:
        ui->titleLabel->setText("Import Signed Transaction");
        ui->scanSignedTransaction->startCapture(true);
        ui->nextButton->setDisabled(true);
        break;
    case 8:
        ui->titleLabel->setText("");
        this->renderTxProgress();
        ui->txids->setText(QString("Tx Id's: %1").arg(this->txIds.join(", ")));
        ui->blocks->setVisible(false);
        ui->nextButton->setDisabled(true);
        break;
    default:
        ui->titleLabel->setText("");
        ui->nextButton->setDisabled(true);
    }
}

void MainWindow::transactionsProgress(bool active, int periodInMilliseconds) {
    static QTimer* syncTimer = nullptr;

    if (active) {
        if (!syncTimer) {
            syncTimer = new QTimer(this);
            connect(syncTimer, &QTimer::timeout, this, &MainWindow::renderTxProgress);
        }

        syncTimer->start(periodInMilliseconds);
    } else {
        if (syncTimer) {
            syncTimer->stop();
            delete syncTimer;
            syncTimer = nullptr;
        }
    }
};

void MainWindow::renderTxProgress() {
    TransferByTxIdResult result = this->walletRpc->getTransferByTxId(this->txIds.first());
    if(result.error) {
        qDebug() << "error: " << result.error_message;
        return;
    }
    switch (result.transfer.type) {
    case TransferType::Pending:
    ui->sendingStatus->setText("Sending...");
        break;
    case TransferType::Pool:
    ui->sendingStatus->setText("Waiting for first confirmation...");
        break;
    case TransferType::Out:
        if(result.transfer.confirmations >= 10) {
            ui->sendingStatus->setText(QString("Done! The receiver can now spend the %1.").arg(this->relativeXmr(result.transfer.amount)));
            break;
        }
    ui->sendingStatus->setText("Waiting for first confirmation...");
        break;
    default:
        break;
    }
    ui->done->setVisible(result.transfer.confirmations > 0); // set visible on first confirmation
    ui->done->setEnabled(result.transfer.confirmations >= 10); // on 10th confirmation
    const static QList<TransferType> ACCEPTABLE_TRANSFER_TYPES = {
        TransferType::Out,
        TransferType::Pending,
        TransferType::Pool
    };
    ui->blocks->setVisible(ACCEPTABLE_TRANSFER_TYPES.contains(result.transfer.type)); // set visible with tx in mempool)
    ui->blocks->setText(QString("%1/%2").arg(result.transfer.confirmations).arg(10));
    ui->nextButton->setEnabled(result.transfer.confirmations > 0); // set enabled on first confirmation;
}

void MainWindow::checkNodeUrl() {
    QUrl url = QUrl(QString("%1:%2").arg(ui->nodeAddressEdit->text(), ui->nodePortEdit->text()));
    if(!url.isValid()) {
        // mark invalid
        ui->nextButton->setDisabled(true);
        return;
    }
    ui->nextButton->setEnabled(true);
    //NetworkChecker::checkMonerodRpc(url.toString());
    //this->walletRpc->setDaemon(url.toString(), NetworkChecker::isDomainOrIpLocal(url.host()));
}

bool MainWindow::checkLogic(int currentIndex, int nextIndex) {
    if(nextIndex == 6) {
        this->lockedUnsignedTransaction = true;
        if(this->unsignedTransaction != nullptr && !this->unsignedTransaction.isEmpty())
            return true;
        this->lockedUnsignedTransaction = false;
        return false;
    }
    return true; // Return true if logic is OK to proceed, otherwise return false
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onViewWalletScanFinished(bool successful) {
    if(!successful || !this->isViewOnlyWallet(ui->scanViewWallet->decodedString)) {
        ui->scanViewWallet->startCapture(false);
        return;
    }
    // this->walletRpcManager = new WalletRpcManager(this, QString("/home/thor/monero-gui-v0.18.3.3/extras/monero-wallet-rpc"), this->getWalletDirectory(), this->network, 18666);
    emit this->loadingWalletRpc(15000);
    this->walletRpcManager = new WalletRpcManager(this, WalletRpcManager::findShippedWalletRpc("monero-wallet-rpc"), this->getWalletDirectory(), this->network, 18666);
    connect(this->walletRpcManager, &WalletRpcManager::walletRPCStarted, this, &MainWindow::onWalletRpcStarted);
    connect(this->walletRpcManager, &WalletRpcManager::walletRPCStopped, this, &MainWindow::onWalletRpcStopped);
    connect(this->walletRpcManager, &WalletRpcManager::error, this, &MainWindow::onWalletRpcError);
    this->walletRpcManager->startWalletRPC();
}

void MainWindow::onKeyImagesScanFinished(bool successful) {
    qDebug() << "onKeyImagesScanFinished";
    qDebug() << "successful: " << successful << " ur type: " << ui->scanKeyImages->getURType();
    if(!successful || ui->scanKeyImages->getURType() != "xmr-keyimage") {
        qDebug() << "scan again...";
        ui->scanKeyImages->startCapture(true);
        return;
    }
    qDebug() << "keyimages: ur:type:" << ui->scanKeyImages->getURType();
    std::string data = ui->scanKeyImages->getURData();
    qDebug() << "keyimages: " << data;
    KeyImageImportResult result = this->walletRpc->importKeyImagesFromByteString(QByteArray::fromStdString(data).toHex().toStdString(), this->restoreHeight);
    qDebug() << "imported keyimages, height: " << result.height << ", spent: " << result.spent << ", unspent: " << result.unspent;
    if(result.height >= 0) {
        qDebug("go next");
        this->walletRpc->rescanSpent();
        this->walletRpc->refresh();
        qDebug() << " balance: " << this->walletRpc->getBalance().balance;
        //go next
        if(result.unspent <= SPENDABLE_TRESHOLD)
            this->next();
        else
            this->insufficientFunds();
    }
    qDebug("exit: onKeyImagesScanFinished()");
}

void MainWindow::onSignedTransactionScanFinished(bool successful) {
    qDebug() << "onSignedTransactionScanFinished";
    qDebug() << "successful: " << successful << " ur type: " << ui->scanSignedTransaction->getURType();

    if(!successful || ui->scanSignedTransaction->getURType() != "xmr-txsigned") {
        qDebug() << "scan again...";
        ui->scanSignedTransaction->startCapture(true);
        return;
    }
    qDebug() << "signed tx: ur:type:" << ui->scanSignedTransaction->getURType();
    std::string data = ui->scanSignedTransaction->getURData();
    qDebug() << "signed tx: " << data;
    SubmitTransferResult result = this->walletRpc->submitTransfer(QByteArray::fromStdString(data).toHex());
    qDebug() << "submit error: " << result.error << (result.error?(QString("(%1)").arg(result.error_message)):"") << " tx ids: " << result.tx_hash_list;
    if(result.error || result.tx_hash_list.isEmpty()) {
        QMessageBox::critical(this, "Transaction failed", QString("Wallet error: %1.").arg(result.error_message));
    }
    qDebug("success: onSignedTransactionScanFinished();");
    this->txIds.fromList(result.tx_hash_list);
    this->walletRpc->rescanSpent();
    this->walletRpc->refresh();
    qDebug() << " balance: " << this->walletRpc->getBalance().balance;
    this->next();
}

void MainWindow::insufficientFunds() {
    // TODO: implement address browser, for now go anyway to sendXmr
    this->next();
}

void MainWindow::updateAvailableXmr() {
    this->availableBalance = this->walletRpc->getBalance().unlocked_balance;
        ui->availableAmount->setText(QString("Available: %1").arg(this->relativeXmr(this->availableBalance)));
    // if this->walletRpc->getBalance().unlocked_balance and this->walletRpc->getBalance().balance differ it should be shown also in the UI that x amount is still locked...
    checkAmount();  // Recheck amount validity with new balance
}

void MainWindow::syncAvailableXmr(bool active, int periodInMilliseconds) {
    static QTimer* syncTimer = nullptr;

    if (active) {
        if (!syncTimer) {
            syncTimer = new QTimer(this);
            connect(syncTimer, &QTimer::timeout, this, &MainWindow::updateAvailableXmr);
        }

        syncTimer->start(periodInMilliseconds);
    } else {
        if (syncTimer) {
            syncTimer->stop();
            delete syncTimer;
            syncTimer = nullptr;
        }
    }
}

QString MainWindow::relativeXmr(unsigned int atomic_units) {
    const QVector<QString> units = {"pXMR", "nXMR", "µXMR", "mXMR", "XMR"};
    const QVector<quint64> scales = {1ULL, 1000ULL, 1000000ULL, 1000000000ULL, 1000000000000ULL};

    int unitIndex = 0;
    double value = static_cast<double>(atomic_units);
    while (unitIndex < units.size() - 1 && value >= scales[unitIndex + 1]) {
        unitIndex++;
    }
    value /= scales[unitIndex];
    int maxDecimals = 12 - (3 * unitIndex);
    maxDecimals = qMax(0, qMin(maxDecimals, 12)); // Ensure maxDecimals is between 0 and 12
    QString formattedValue = QString::number(value, 'f', maxDecimals);
    formattedValue = formattedValue.remove(QRegularExpression("0+$")).remove(QRegularExpression("\\.$"));

    return QString("%1 %2").arg(formattedValue, units[unitIndex]);
}

QString MainWindow::getWalletFile() {
    if(this->primaryAddress == nullptr)
        return nullptr;
    return QString("tmp_wallet_%1").arg((this->primaryAddress));
}

QString MainWindow::getWalletDirectory() {
    if(this->tempDir == nullptr) {
        this->tempDir = new QTemporaryDir();
        if (this->tempDir->isValid()) {
            this->tempDir->setAutoRemove(true);
        }
    }
    if(this->tempDir == nullptr)
            return QDir::tempPath();
    return this->tempDir->path();
}

QString MainWindow::getWalletPath() {
    if(this->primaryAddress == nullptr)
        return nullptr;
    return QDir(this->getWalletDirectory()).filePath(this->getWalletFile());
}

void MainWindow::removeWalletFiles() {
    qDebug() << "removeWalletFiles";
    if(this->tempDir != nullptr) {
	    delete this->tempDir;
	    this->tempDir = nullptr;
    }
}

void MainWindow::loadWallet() {
    emit this->loadingWallet(100000);
    LoadWalletResult response = this->walletRpc->loadWallet(
        this->restoreHeight,
        this->getWalletFile(),
        this->primaryAddress,
        this->privateViewKey
        );
    qDebug() <<  response.address << ":" << response.info;
    qDebug() << "correct wallet: " << (response.address == this->primaryAddress);
    
    if(response.error) {
        // Handle error case
        qDebug() << "Error found in response: " << response.error_message;
        emit this->walletError();
    } else {
        // Handle result case
        qDebug() << "Result found in response: " << response.info;
        emit this->walletLoaded();
    }
}

void MainWindow::onWalletRpcStarted() {
    qDebug() << "wallet rpc started";
    this->walletRpc = new WalletJsonRpc(this, "localhost", 18666, false);
    this->walletRpc->setAuthentication("wallet", "wallet");
    this->checkWalletRpcConnection(50);
}

void MainWindow::onWalletRpcStopped() {
    // Todo: is there something to do? Shutdown or? Maybe check if it was intentional, if not inform user!
    delete this->walletRpcManager;
    this->walletRpcManager = nullptr;
    // QApplication::exit(0); // find better solution because this will end in a race condition
}

void MainWindow::onWalletRpcError() {
    QMessageBox::critical(this, "Error", "An error occurred related to wallet rpc. Application will exit.");
    QApplication::exit(1);
}

void MainWindow::onShutdown() {
    qDebug() << "onShutdown";
    if(this->walletRpcManager != nullptr) {
        this->walletRpcManager->stopWalletRPC();
    }
    this->removeWalletFiles();
}

void MainWindow::checkWalletRpcConnection(int attempts, int delayBetweenAttempts) {
    emit this->waitForWalletRpcConnection(attempts * delayBetweenAttempts);
    if (attempts <= 0) {
        qDebug() << "Failed to connect after multiple attempts";
        emit this->walletRpcConnectionFailed();
        return;
    }

    QString version = this->walletRpc->getVersion();
    if (version != "0.0") {
        qDebug() << "Connected successfully. Version:" << version;
        emit this->walletRpcConnected();
        return;
    }
    QTimer::singleShot(delayBetweenAttempts, this, [this, attempts]() {
        checkWalletRpcConnection(attempts - 1);
    });
}

void MainWindow::onWalletRpcReady() {
    this->loadWallet();
}

void MainWindow::onWalletRpcFailed() {
    // we can do nothing on this point, tell user about and exit!
    // TODO: some dialog
    QApplication::exit(1);
}

void MainWindow::onWalletLoaded() {
    int currentIndex = ui->stackedWidget->currentIndex();
    int nextIndex = (currentIndex + 1) % ui->stackedWidget->count();
    this->removeQrCodeScanWidgetFromUi(ui->scanViewWallet);
    ui->stackedWidget->setCurrentIndex(nextIndex);
    ui->dotIndicator->setCurrentStep(nextIndex);
}

void MainWindow::onWalletError() {
    // dialog and scan again
    qDebug() << "view wallet is faulty, restart scanning!";
    this->onViewWalletScanFinished(false);
}

bool MainWindow::isViewOnlyWallet(const QString& qrCode) {
    if (qrCode.isEmpty()) {
        emit this->invalidQr();
        return false;
    }

    if (qrCode.startsWith("monero_wallet:")) {
        // Use regex to extract the address part
        QRegularExpression re("monero_wallet:([^?]+)(?:\\?(.*))?");
        QRegularExpressionMatch match = re.match(qrCode);

        if (match.hasMatch()) {
            this->primaryAddress = match.captured(1);
            this->network = NETWORK.value(this->primaryAddress[0], "");
            this->monerodPort = NETWORK_PORT.value(this->network, -1);

            // Use QUrlQuery for the query part
            if (match.lastCapturedIndex() >= 2) {
                QUrlQuery query(match.captured(2));
                this->privateViewKey = query.queryItemValue("view_key");
                this->restoreHeight = query.queryItemValue("height").toInt();
            }
        }
    } else {
        // Try parsing as JSON
        QJsonParseError error;
        auto doc = QJsonDocument::fromJson(qrCode.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
            return false;
        this->primaryAddress = doc["primaryAddress"].toString();
        this->privateViewKey = doc["privateViewKey"].toString();
        this->restoreHeight = doc["restoreHeight"].toInt();
        this->network = NETWORK.value(this->primaryAddress[0], "");
        this->monerodPort = NETWORK_PORT.value(this->network, -1);
    }

    // Validate the parsed data
    if (!this->primaryAddress.isEmpty() && !this->privateViewKey.isEmpty()) {
        ui->networkLabel->setText(this->network.toUpper());
        ui->networkLabel->setVisible(this->network != NETWORK['4']);
        return true;
        emit this->validViewWalletQr();
    }
    emit this->invalidQr();
    return false;
}

void MainWindow::removeQrCodeScanWidgetFromUi(QrCodeScanWidget *&widget) {
    widget->stop();
    QLayout *layout = widget->parentWidget()->layout();
    layout->removeWidget(widget);
    widget->deleteLater();
}

void MainWindow::walletSyncProgress(bool active, int periodInMilliseconds) {
    static QTimer* syncTimer = nullptr;

    if (!active) {
        if (syncTimer) {
            syncTimer->stop();
            delete syncTimer;
            syncTimer = nullptr;
        }
        return;
    }
    if (!syncTimer) {
        syncTimer = new QTimer(this);
        connect(syncTimer, &QTimer::timeout, this, &MainWindow::updateWalletSyncProgress);
    }
    syncTimer->start(periodInMilliseconds);
}

QString MainWindow::relativeTimeFromMilliseconds(int milliseconds) {
    if(milliseconds > 172800000) // 2 day
        return QString("> %1 days").arg(milliseconds / 86400000);
    if(milliseconds > 86400000) // 1 day
        return QString("> 1 day");
    if(milliseconds > 7140000) // 119min
        return QString("%1h").arg(milliseconds / 3600000);
    if(milliseconds > 60000)
        return QString("%1min").arg(milliseconds / 60000);
    if(milliseconds > 1000)
        return QString("%1s").arg(milliseconds / 1000);
    return QString("1s");
}

QString MainWindow::relativeBlocksPerSecond(int blocks) {
    const QVector<QString> units = {"Blocks", "kBlocks", "MBlocks", "GBlocks"};
    double value = blocks;
    int unitIndex = 0;

    while (value >= 1000 && unitIndex < units.size() - 1) {
        value /= 1000;
        unitIndex++;
    }

    if(unitIndex == 0)
            return QString("%1 %2/s").arg(value).arg(units[unitIndex]);
    return QString("%1 %2/s").arg(value, 0, 'f', 2).arg(units[unitIndex]);
}

void MainWindow::updateWalletSyncProgress() {
    qDebug() << "update wallet sync progress";
    static qint64 start = 0;
    qint64 current_time = QDateTime::currentSecsSinceEpoch();
    if(start == 0)
        start = current_time;
    int networkHeight = this->daemonRpc->getHeight();
    int walletHeight = this->walletRpc->getSimpleHeight();
    int percentage = (walletHeight - this->restoreHeight) * 100 / (networkHeight - this->restoreHeight);
    int missing = networkHeight - walletHeight;
    int avgBlocksPerSecond = (start==current_time)?0:((walletHeight - this->restoreHeight) / (current_time - start));
    int eta = (start==current_time)?-1:(missing / avgBlocksPerSecond) * 1000;
    qDebug() << "Height: " << walletHeight << "/" << networkHeight << " (" << percentage << "%) Missing: " << missing << " eta: " << eta;
    ui->missingBlocks->setText(QString("Blocks missing: %1").arg(missing));
    ui->syncETA->setText((eta!=-1)?(QString("ETA: %1").arg(this->relativeTimeFromMilliseconds(eta))):QString("ETA: calculating..."));
    ui->avgBlockPerSecond->setText((avgBlocksPerSecond!=-1)?this->relativeBlocksPerSecond(avgBlocksPerSecond):QString(""));
    ui->syncProgressBar->setValue((0 < percentage && percentage < 15)?14:percentage); // fix for css glitch
    ui->syncProgressBar->setTextVisible(percentage > 14); // fix for css glitch
    if(missing == 0 && networkHeight != 0  && networkHeight == walletHeight) {
        this->walletSyncProgress(false);
        int currentIndex = ui->stackedWidget->currentIndex();
        int nextIndex = (currentIndex + 1) % ui->stackedWidget->count();
        ui->stackedWidget->setCurrentIndex(nextIndex);
        ui->dotIndicator->setCurrentStep(nextIndex);
        qDebug() << "syncing wallet done";
    }
    BalanceResult br = this->walletRpc->getBalance();
    qDebug() << "balance: " << br.balance << " unlocked: " << br.unlocked_balance;
}

QString MainWindow::getFingerprint() {
    return QCryptographicHash::hash(QByteArray::fromStdString(this->primaryAddress.toStdString()), QCryptographicHash::Sha256).toHex().right(6).toUpper();
    /**
    std::string addressStr = this->primaryAddress.toStdString();
    QByteArray hash = QCryptographicHash::hash(QByteArray::fromStdString(addressStr), QCryptographicHash::Sha256);
    QString hashHex = hash.toHex();
    return hashHex.right(6).toUpper();
        */
}

void MainWindow::setupSendXmrInputs()
{
    // Setup address input
    QRegularExpression addressRx("^[1-9A-Za-z]{95}$");
    addressValidator = new QRegularExpressionValidator(VALID_NETWORK_ADDRESS[this->network], this);
    ui->address->setValidator(addressValidator);
    connect(ui->address, &QLineEdit::textChanged, this, &MainWindow::checkAddress);

    // Setup amount input
    QRegularExpression amountRx("^\\d{1,12}([.,]\\d{1,12})?$");
    amountRegexValidator = new QRegularExpressionValidator(amountRx, this);
    ui->amount->setValidator(amountRegexValidator);
    connect(ui->amount, &QLineEdit::textChanged, this, &MainWindow::checkAmount);
    // Initial state
    updateSendButtonState();
}

void MainWindow::checkAddress()
{
    ui->address->setStyleSheet(QString("QLineEdit { background-color: white; color: black; font-size: 36px; border-radius: 35px; padding: 5px; min-height: 45px; max-height: 45px; border: 10px solid %1; }").arg(ui->address->hasAcceptableInput()?"green":"transparent"));
    updateSendButtonState();
}

void MainWindow::checkAmount()
{
    // Replace comma with dot
    if (ui->amount->text().contains(',')) {
        int cursorPosition = ui->amount->cursorPosition();
        ui->amount->setText(ui->amount->text().replace(',', '.'));
        ui->amount->setCursorPosition(cursorPosition);
    }

    bool ok;
    double amount = ui->amount->text().toDouble(&ok);
    ok = ok && amount > 0 && amount <= static_cast<double>(this->availableBalance) / 1e12;
    ui->amount->setStyleSheet(QString("QLineEdit { background-color: white; color: black; font-size: 36px; border-radius: 35px; padding: 5px; min-height: 45px; max-height: 45px; border: 10px solid %1; }").arg(ok?"green":"transparent"));
    updateSendButtonState();
}

void MainWindow::updateSendButtonState()
{
    unsigned int estimatedFee = this->estimateTotalTransfer();
    ui->nextButton->setEnabled(
        this->getAmountValue() != 0
        &&!ui->amount->text().isEmpty()
        && ui->address->hasAcceptableInput()
        && !ui->amount->text().isEmpty()
        && ui->amount->hasAcceptableInput()
        && this->getAmountValue() <= this->availableBalance
        && (estimatedFee != INVALID_FEE)
        && estimatedFee <= this->availableBalance
    );
    ui->estimatedFee->setEnabled(estimatedFee != INVALID_FEE);
    ui->estimatedFee->setText((estimatedFee == INVALID_FEE)?"":(QString("Estimated fee: %1").arg(this->relativeXmr(estimatedFee))));
}

unsigned int MainWindow::estimateTotalTransfer() {
    if(this->getAmountValue() <= 0)
        return INVALID_FEE;
    if(this->lockedUnsignedTransaction)
        return INVALID_FEE;
    QList<Destination> destinations;
    destinations.append(Destination(this->getAmountValue(), (!ui->address->text().isEmpty()&&ui->address->hasAcceptableInput())?ui->address->text():this->primaryAddress));
    TransferResult tr = this->walletRpc->transfer(destinations);
    if(tr.error) {
        if(!this->lockedUnsignedTransaction)
            this->unsignedTransaction = nullptr;
        return INVALID_FEE;
    }
    if(!this->lockedUnsignedTransaction)
        this->unsignedTransaction = tr.unsigned_txset;
    return tr.fee;
}

unsigned int MainWindow::getAmountValue()
{
    bool ok;
    double amount = ui->amount->text().toDouble(&ok);
    return ok ? static_cast<unsigned int>(amount * 1e12) : 0;
}

PortValidator::PortValidator(QObject *parent) : QValidator(parent) {}

QValidator::State PortValidator::validate(QString &input, int &pos) const {
    Q_UNUSED(pos);

    int port = input.toInt();
    return (input.isEmpty() || (port >= 1 && port <= 65535)) ? QValidator::Acceptable : QValidator::Invalid;
}
