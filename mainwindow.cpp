#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QVBoxLayout>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) //, Qt::FramelessWindowHint)
	, ui(new Ui::MainWindow)
	  , m_dotIndicator(new QDotProgressIndicator(this))
{
    //setAttribute(Qt::WA_TranslucentBackground);
	ui->setupUi(this);
    ui->prevButton->setDisabled(true);
    ui->stackedWidget->setCurrentIndex(0);
    this->syncDotIndicator(0);

    m_dotIndicator->setTotalSteps(ui->stackedWidget->count());  // Set the total number of steps
    m_dotIndicator->setCurrentStep(0);  // Set initial step
	m_dotIndicator->setFixedSize(200, 20);  // Adjust size as needed

	// Assuming the widget in your UI is named 'dotProgressIndicatorFrame'
	// Replace 'dotProgressIndicatorFrame' with the actual name from your UI file
	if (ui->dotProgressIndicatorFrame) {
		QWidget* parent = ui->dotProgressIndicatorFrame->parentWidget();
		QLayout* parentLayout = parent->layout();

		if (parentLayout) {
			// Remove the existing widget from the layout
			parentLayout->removeWidget(ui->dotProgressIndicatorFrame);
			parentLayout->removeWidget(ui->nextButton);

			// Add the new dot indicator to the layout
			parentLayout->addWidget(m_dotIndicator);
			parentLayout->addWidget(ui->nextButton);

			// Set the same size policy as the original widget
			m_dotIndicator->setSizePolicy(ui->dotProgressIndicatorFrame->sizePolicy());

			// Hide and schedule for deletion the original widget
			ui->dotProgressIndicatorFrame->hide();
			ui->dotProgressIndicatorFrame->deleteLater();
		} else {
			// If there's no layout, simply add the new widget to the parent
			m_dotIndicator->setParent(parent);
			m_dotIndicator->move(ui->dotProgressIndicatorFrame->pos());
			m_dotIndicator->show();

			ui->dotProgressIndicatorFrame->hide();
			ui->dotProgressIndicatorFrame->deleteLater();
		}
	}

	connect(ui->nextButton, &QPushButton::clicked, this, [this]() {
			// Check logic before advancing to next step
			if (checkLogic()) {
			int currentIndex = ui->stackedWidget->currentIndex();
			int nextIndex = (currentIndex + 1) % ui->stackedWidget->count(); // Loop back to the first widget if at the last
			ui->stackedWidget->setCurrentIndex(nextIndex);
            m_dotIndicator->setCurrentStep(nextIndex); // Sync QDotProgressIndicator
			}
			});

	connect(ui->prevButton, &QPushButton::clicked, this, [this]() {
			// Check logic before going back to previous step
			if (checkLogic()) {
			int currentIndex = ui->stackedWidget->currentIndex();
			int prevIndex = (ui->stackedWidget->count() + currentIndex - 1) % ui->stackedWidget->count(); // Loop to last widget if at the first
			ui->stackedWidget->setCurrentIndex(prevIndex);
            m_dotIndicator->setCurrentStep(prevIndex); // Sync QDotProgressIndicator
			}
			});

	connect(ui->stackedWidget, &QStackedWidget::currentChanged, this, &MainWindow::syncDotIndicator);

	// For debugging
	qDebug() << "Dot Indicator size:" << m_dotIndicator->size();
	qDebug() << "Dot Indicator is visible:" << m_dotIndicator->isVisible();
}
    // Slot in MainWindow class to sync QDotProgressIndicator with QStackedWidget
    void MainWindow::syncDotIndicator(int index) {
        m_dotIndicator->setCurrentStep(index);
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
        case 8:
            ui->titleLabel->setText(QString("Sending..."));
            break;
        default:
            ui->titleLabel->setText(QString(""));
        }
    }

    bool MainWindow::checkLogic() {
        // Add your logic checking here
        return true; // Return true if logic is OK to proceed, otherwise return false
    }

MainWindow::~MainWindow()
{
	delete ui;
}
