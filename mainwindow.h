#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMap>
#include <QString>
#include <QMainWindow>
#include "qrcode/scanner/QrCodeScanWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    bool checkLogic();
    QString primaryAddress = nullptr;
    QString privateViewKey = nullptr;
    int restoreHeight = 0;

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

private slots:
    void syncDotIndicator(int index);
    void viewWalletScanFinished(bool successful);
    void removeQrCodeScanWidgetFromUi(QrCodeScanWidget *&widget);
    bool isViewOnlyWallet(const QString& qrCode);

private:
    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
