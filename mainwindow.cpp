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
        this->walletRpc->setDaemon(this->daemon_url);
        this->walletSyncProgress(true);
        break;
    case 3:
        ui->titleLabel->setText(QString("Export Outputs"));
        qDebug() << "outputs: " << this->walletRpc->exportOutputs();
        this->walletSyncProgress(false);
        ui->nextButton->setEnabled(false);
        break;
    case 4:
        ui->titleLabel->setText(QString("Import Key Images"));
        ui->scanKeyImages->startCapture(true);
        //ui->nextButton->setDisabled(true);
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
    QJsonObject response = this->walletRpc->loadWallet(
        this->restoreHeight,
        this->getWalletFile(),
        this->primaryAddress,
        this->privateViewKey
        );
	qDebug() <<  response;
    
    if(response.contains("error")) {
        // Handle error case
        qDebug() << "Error found in response: " << response["error"].toString();
        emit this->walletError();
    } else if(response.contains("result")) {
        // Handle result case
        qDebug() << "Result found in response: " << response["result"].toString();
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

void MainWindow::updateWalletSyncProgress() {
    qDebug() << "update wallet sync progress";
    int networkHeight = this->daemonRpc->getHeight();
    int walletHeight = this->walletRpc->getHeight();
    int percentage = walletHeight * 100 / networkHeight;
    int missing = networkHeight - walletHeight;
    qDebug() << "Height: " << walletHeight << "/" << networkHeight << " (" << percentage << "%) Missing: " << missing;
    ui->syncProgressBar->setValue(percentage);
    if(missing == 0 && networkHeight != 0) {
        this->walletSyncProgress(false);
        int currentIndex = ui->stackedWidget->currentIndex();
        int nextIndex = (currentIndex + 1) % ui->stackedWidget->count();
        ui->stackedWidget->setCurrentIndex(nextIndex);
        ui->dotIndicator->setCurrentStep(nextIndex);
        qDebug() << "syncing wallet done";
    }
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
