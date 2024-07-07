#include "mainwindow.h"
#include "qrcode/scanner/QrCodeScanWidget.h"
#include "./ui_mainwindow.h"
#include <QVBoxLayout>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QRegularExpression>
#include <QUrlQuery>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) //, Qt::FramelessWindowHint)
    , ui(new Ui::MainWindow)
{
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
    connect(ui->scanViewWallet, &QrCodeScanWidget::finished, this, &MainWindow::viewWalletScanFinished);

    qDebug() << "Dot Indicator size:" << ui->dotIndicator->size();
    qDebug() << "Dot Indicator is visible:" << ui->dotIndicator->isVisible();
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

void MainWindow::viewWalletScanFinished(bool successful) {
    int currentIndex = ui->stackedWidget->currentIndex();
    int nextIndex = (currentIndex + 1) % ui->stackedWidget->count();
    bool next = this->isViewOnlyWallet(ui->scanViewWallet->decodedString);
    if(next) {
        this->removeQrCodeScanWidgetFromUi(ui->scanViewWallet);
        ui->stackedWidget->setCurrentIndex(nextIndex);
        ui->dotIndicator->setCurrentStep(nextIndex);
    } else {
        ui->scanViewWallet->startCapture(false);
    }
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
