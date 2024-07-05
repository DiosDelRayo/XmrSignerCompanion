#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include "qaddresslineedit.h"
#include "qamountlineedit.h"
#include "qdotprogressindicator.h"
#include "qrcode/scanner/QrCodeScanWidget.h"

Q_DECLARE_METATYPE(QAddressLineEdit*)
Q_DECLARE_METATYPE(QAmountLineEdit*)
Q_DECLARE_METATYPE(QDotProgressIndicator*)
Q_DECLARE_METATYPE(QrCodeScanWidget*)

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "XmrSigner_" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
    MainWindow w;
    w.show();
    return a.exec();
}
