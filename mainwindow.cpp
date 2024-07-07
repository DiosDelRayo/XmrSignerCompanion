#include "mainwindow.h"
#include "qrcode/scanner/QrCodeScanWidget.h"
#include "./ui_mainwindow.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) //, Qt::FramelessWindowHint)
    , ui(new Ui::MainWindow)
{
    connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit, this, &MainWindow::onShutdown);
    connect(this, &MainWindow::shutdownRequested, this, &MainWindow::onShutdown);
    //setAttribute(Qt::WA_TranslucentBackground);
    ui->setupUi(this);
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

    qDebug() << "Dot Indicator size:" << ui->dotIndicator->size();
    qDebug() << "Dot Indicator is visible:" << ui->dotIndicator->isVisible();
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
        ui->scanViewWallet->startCapture(false);
        ui->nextButton->setEnabled(true);
        break;
    case 2:
        ui->titleLabel->setText(QString("Synchronize Wallet"));
        break;
    case 3:
        ui->titleLabel->setText(QString("Export Outputs"));
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

void MainWindow::awaitWalletRpcForMax(int microseconds) {
    qDebug() << "Wait for wallet rpc for more: " << microseconds << "microseconds";
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
    if (qrCode.isEmpty())
        return false;

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
    }
    return false;
}

void MainWindow::removeQrCodeScanWidgetFromUi(QrCodeScanWidget *&widget) {
    widget->stop();
    QLayout *layout = widget->parentWidget()->layout();
    layout->removeWidget(widget);
    widget->deleteLater();
}
