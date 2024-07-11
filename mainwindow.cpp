#include "mainwindow.h"
#include "qrcode/scanner/QrCodeScanWidget.h"
#include "./ui_mainwindow.h"
#include "networkchecker.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
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
    : QMainWindow(parent) //, Qt::FramelessWindowHint)
    , ui(new Ui::MainWindow)
{
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &MainWindow::onShutdown);
    connect(this, &MainWindow::shutdownRequested, this, &MainWindow::onShutdown);
    //setAttribute(Qt::WA_TranslucentBackground);
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
        // Check logic before advancing to next step
        if (checkLogic()) {
            int currentIndex = ui->stackedWidget->currentIndex();
            int nextIndex = (currentIndex + 1) % ui->stackedWidget->count(); // Loop back to the first widget if at the last
            ui->stackedWidget->setCurrentIndex(nextIndex);
            ui->dotIndicator->setCurrentStep(nextIndex); // Sync QDotProgressIndicator
        }
    });

    connect(ui->prevButton, &QPushButton::clicked, this, [this]() {
        // Check logic before going back to previous step
        if (checkLogic()) {
            int currentIndex = ui->stackedWidget->currentIndex();
            int prevIndex = (ui->stackedWidget->count() + currentIndex - 1) % ui->stackedWidget->count(); // Loop to last widget if at the first
            ui->stackedWidget->setCurrentIndex(prevIndex);
            ui->dotIndicator->setCurrentStep(prevIndex); // Sync QDotProgressIndicator
        }
    });

    connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &MainWindow::syncDotIndicator);
    connect(ui->scanViewWallet, &QrCodeScanWidget::finished, this, &MainWindow::onViewWalletScanFinished);
    connect(ui->scanKeyImages, &QrCodeScanWidget::finished, this, &MainWindow::onKeyImagesScanFinished);

    connect(this, &MainWindow::walletRpcConnected, this, &MainWindow::onWalletRpcReady);
    connect(this, &MainWindow::walletRpcConnectionFailed, this, &MainWindow::onWalletRpcFailed);
    connect(this, &MainWindow::walletLoaded, this, &MainWindow::onWalletLoaded);
    connect(this, &MainWindow::walletError, this, &MainWindow::onWalletError);

    connect(this, &MainWindow::validViewWalletQr, ui->scanViewWallet, &QrCodeScanWidget::onFrameStateValidated);
    connect(this, &MainWindow::invalidQr, ui->scanViewWallet, &QrCodeScanWidget::onFrameStateError);
    connect(this, &MainWindow::loadingWalletRpc, ui->scanViewWallet, &QrCodeScanWidget::onFrameStateProcessing);
    connect(this, &MainWindow::loadingWallet, ui->scanViewWallet, &QrCodeScanWidget::onFrameStateProcessing);

    qDebug() << "Dot Indicator size:" << ui->dotIndicator->size();
    qDebug() << "Dot Indicator is visible:" << ui->dotIndicator->isVisible();
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
    lookupTimer->setInterval(500); // 500ms delay

    connect(ui->nodeAddressEdit, &QLineEdit::textChanged, this, &MainWindow::checkNodeAddress);
    connect(lookupTimer, &QTimer::timeout, this, &MainWindow::checkNodeAddress);

    // Setup port input
    QRegularExpression portRx("^([1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])$");
    nodePortValidator = new QRegularExpressionValidator(portRx, this);
    ui->nodePortEdit->setValidator(nodePortValidator);

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
                      .arg(ui->nodeTls->isChecked() ? "s" : "")
                      .arg(host)
                      .arg(port);
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
        // ui->outputsUR->setData("xmr-outputs", outputs.toStdString());
        qDebug() << "outputs:binary:(local 8bit) " << QByteArray::fromHex(outputs.toLocal8Bit()).constData();
        qDebug() << "outputs:binary:(latin 1) " << QByteArray::fromHex(outputs.toLatin1()).constData();
        qDebug() << "outputs:binary:(local 8bit -> std::string) " << QByteArray::fromHex(outputs.toLocal8Bit()).toStdString();
        //ui->outputsUR->setData("xmr-outputs", QByteArray::fromHex(outputs.toLocal8Bit()).toStdString());
        //ui->outputsUR->setData("xmr-outputs", QByteArray::fromHex(outputs.toLocal8Bit()).toStdString());
        ui->outputsUR->setData("xmr-outputs", QByteArray::fromHex("4d6f6e65726f206f7574707574206578706f72740457aab7d6f180e30a7ddd74b5019d2cc0a5ca23b3568b246bb76fed76ca0f8e6f14bf163b60743d807777312f797276d4bca1705b59170264b53065ad306a259f32f1595a6040cb493a5da1066141f558fd0e03126ab9b112873b65b026e03f70e7bd18454dd1364435a6b485dcb99d8a1274b189d69f7ac0d5a87af050e39bc18f907288cf5faff6303a51506c68703d332aac2b42a47debf9a94c55754f54aaeaa8fe3aebc3ca017da0a44c10235aab17647764413235483163566844e971f4f78ed55ec4de5f8433297008f13dad33a9e9c2e627648e8d1e981665b1e719d288ef62bdb0ec1ef68b25b9e4e06bf53bfcb510753f2d7623abf497fc515eb9f1749253ddaf7e7b3f25b5591fd18dc45423db59bebc29462ab924845f1223fda0c842f26fcf2bad1f7f4832568af24d799d0620f273c17ac0f538fcc0782eb19cdcabf541e5b2e54cd130fe6d6b583c0a6a0926175d3d8fea00c454ffe06c727fcee5c44e9466940f1a532c836b627fc0f29d415888e825e108").toStdString());
        this->walletSyncProgress(false);
        ui->nextButton->setEnabled(true);
        break;
    case 4:
        ui->titleLabel->setText(QString("Import Key Images"));
        ui->scanKeyImages->startCapture(true);
        ui->nextButton->setDisabled(true);
        break;
    case 5:
        ui->titleLabel->setText(QString("Send XMR"));
        ui->nextButton->setEnabled(true);
        break;
    case 6:
        ui->titleLabel->setText(QString("Export Unsigned Transaction"));
        break;
    case 7:
        ui->titleLabel->setText(QString("Import Signed Transaction"));
        break;
    default:
        ui->titleLabel->setText(QString(""));
        ui->nextButton->setDisabled(true);
    }
}

void MainWindow::checkNodeUrl() {
    QUrl url = QUrl(QString("%1:%2").arg(ui->nodeAddressEdit->text()).arg(ui->nodePortEdit->text()));
    if(!url.isValid()) {
        // mark invalid
        ui->nextButton->setDisabled(true);
        return;
    }
    ui->nextButton->setEnabled(true);
    //NetworkChecker::checkMonerodRpc(url.toString());
    //this->walletRpc->setDaemon(url.toString(), NetworkChecker::isDomainOrIpLocal(url.host()));
}

bool MainWindow::checkLogic() {
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
    KeyImageImportResult result = this->walletRpc->importKeyImagesFromByteString(data, this->restoreHeight);
    qDebug() << "imported keyimages, height: " << result.height << ", spent: " << result.spent << ", unspent: " << result.unspent;
    if(result.height >= 0) {
        qDebug("go next");
        this->walletRpc->rescanSpent();
        this->walletRpc->refresh();
        qDebug() << " balance: " << this->walletRpc->getBalance().balance;
        //go next
        if(result.unspent <= SPENDABLE_TRESHOLD)
            this->sendXmrPage();
        else
            this->insufficientFunds();
    }
    qDebug("exit: onKeyImagesScanFinished()");
}

void MainWindow::insufficientFunds() {
    // TODO: implement address browser, for now go anyway to sendXmr
    this->sendXmrPage();
}

void MainWindow::sendXmrPage() {
    ui->availableAmount->setText(QString("Available: %1").arg(this->relativeXmr(this->walletRpc->getBalance().unlocked_balance)));
    int currentIndex = ui->stackedWidget->currentIndex();
    int nextIndex = (currentIndex + 1) % ui->stackedWidget->count();
    this->removeQrCodeScanWidgetFromUi(ui->scanKeyImages);
    ui->stackedWidget->setCurrentIndex(nextIndex);
    ui->dotIndicator->setCurrentStep(nextIndex);
    ui->nextButton->setDisabled(true);
    this->setupSendXmrInputs();
    this->syncAvailableXmr(true);
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

    return QString("%1 %2").arg(formattedValue).arg(units[unitIndex]);
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
    /* seems we don't need it and can relay on auto remove
    auto files = {this->getWalletPath(), QString(this->getWalletPath()) + ".key"};
    for(const auto &f: files) {
        qDebug() << "f: " << f;
        QFile file = QFile(f);
        if(file.exists()) {
            file.remove();
            qDebug() << "file " << file.filesystemFileName() << " removed.";
        }
    } */
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
    this->checkWalletRpcConnection(10);
}

void MainWindow::onWalletRpcStopped() {
    // Todo: is there something to do? Shutdown or? Maybe check if it was intentional, if not inform user!
    // TODO: delete wallet files
    delete this->walletRpcManager;
    this->walletRpcManager = nullptr;
    // QApplication::exit(0); // find better solution because this will end in a race condition
}

void MainWindow::onWalletRpcError() {
    // Todo: inform user before exit
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
    // load wallet
    this->loadWallet();
}

void MainWindow::awaitWalletRpcForMax(int milliseconds) {
    qDebug() << "Wait for wallet rpc for more: " << milliseconds << "milliseconds";
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
        // TODO: Add any additional validation you need and set wallet
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

    if (active) {
        if (!syncTimer) {
            syncTimer = new QTimer(this);
            connect(syncTimer, &QTimer::timeout, this, &MainWindow::updateWalletSyncProgress);
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
    ui->syncProgressBar->setValue(percentage);
    if(missing == 0 && networkHeight != 0  && networkHeight == walletHeight) {
        this->walletSyncProgress(false);
        int currentIndex = ui->stackedWidget->currentIndex();
        int nextIndex = (currentIndex + 1) % ui->stackedWidget->count();
        ui->stackedWidget->setCurrentIndex(nextIndex);
        ui->dotIndicator->setCurrentStep(nextIndex);
        qDebug() << "syncing wallet done";
    }
    //this->walletRpc->refresh(this->restoreHeight);
    BalanceResult br = this->walletRpc->getBalance();
    qDebug() << "balance: " << br.balance << " unlocked: " << br.unlocked_balance;
}

QString MainWindow::getFingerprint() {
    // Convert QString to std::string
    std::string addressStr = this->primaryAddress.toStdString();

    // Compute SHA256 hash
    QByteArray hash = QCryptographicHash::hash(QByteArray::fromStdString(addressStr), QCryptographicHash::Sha256);

    // Convert hash to hexadecimal string
    QString hashHex = hash.toHex();

    // Return the last 6 characters
    return hashHex.right(6).toUpper();
}

void MainWindow::setupSendXmrInputs()
{
    // Setup address input
    QRegularExpression addressRx("^[1-9A-Za-z]{95}$");
    addressValidator = new QRegularExpressionValidator(addressRx, this);
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
    bool ok = ui->address->hasAcceptableInput() && ui->address->text()[0] == QChar(NETWORK.key(network));
    ui->address->setStyleSheet(QString("QLineEdit { background-color: white; color: black; font-size: 36px; border-radius: 35px; padding: 5px; min-height: 45px; max-height: 45px; border: 10px solid %1; }").arg(ok?"green":"transparent"));
    updateSendButtonState();
}

void MainWindow::checkAmount()
{
    QString amountText = ui->amount->text();

    // Replace comma with dot
    if (ui->amount->text().contains(',')) {
        int cursorPosition = ui->amount->cursorPosition();
        ui->amount->setText(ui->amount->text().replace(',', '.'));
        ui->amount->setCursorPosition(cursorPosition);
    }

    bool ok;
    double amount = ui->amount->text().toDouble(&ok);
    ok = ok && amount > 0 && amount <= static_cast<double>(availableBalance) / 1e12;
    ui->amount->setStyleSheet(QString("QLineEdit { background-color: white; color: black; font-size: 36px; border-radius: 35px; padding: 5px; min-height: 45px; max-height: 45px; border: 10px solid %1; }").arg(ok?"green":"transparent"));
    updateSendButtonState();
}

void MainWindow::updateSendButtonState()
{
    ui->nextButton->setEnabled(
        !ui->amount->text().isEmpty() && ui->address->hasAcceptableInput()
        && !ui->amount->text().isEmpty() && ui->amount->hasAcceptableInput()
        );
}

double MainWindow::getAmountValue()
{
    return ui->amount->text().toDouble();
}
