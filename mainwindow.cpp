#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QVBoxLayout>
#include <QDebug>

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

    qDebug() << "Dot Indicator size:" << ui->dotIndicator->size();
    qDebug() << "Dot Indicator is visible:" << ui->dotIndicator->isVisible();
}

void MainWindow::syncDotIndicator(int index) {
    ui->dotIndicator->setCurrentStep(index);
    switch (index) {
    case 0:
        ui->titleLabel->setText(QString("Import View Only Wallet"));
        break;
    case 1:
        ui->titleLabel->setText(QString("Connect"));
        break;
    case 2:
        ui->titleLabel->setText(QString("Synchronize Wallet"));
        break;
    case 3:
        ui->titleLabel->setText(QString("Export Outputs"));
        break;
    case 4:
        ui->titleLabel->setText(QString("Import Key Images"));
        break;
    case 5:
        ui->titleLabel->setText(QString("Send XMR"));
        break;
    case 6:
        ui->titleLabel->setText(QString("Export Unsigned Transaction"));
        break;
    case 7:
        ui->titleLabel->setText(QString("Import Signed Transaction"));
        break;
    default:
        ui->titleLabel->setText(QString(""));
    }
}

bool MainWindow::checkLogic() {
    return true; // Return true if logic is OK to proceed, otherwise return false
}

MainWindow::~MainWindow()
{
    delete ui;
}
