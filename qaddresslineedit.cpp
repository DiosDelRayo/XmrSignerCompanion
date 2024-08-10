#include "qaddresslineedit.h"
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>
#include <QApplication>
#include <QClipboard>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QDebug>


QAddressLineEdit::QAddressLineEdit(QWidget *parent): QLineEdit(parent) {
    QMap<QString, QStringList> moneroRegex = {
                                              {"mainnet", {"^4[0-9AB][0-9A-HJ-NP-Z]{93}$", "^8[0-9A-HJ-NP-Z]{95}$"}},
        {"testnet", {"^5[0-9A-HJ-NP-Z]{94}$", "^9[0-9A-HJ-NP-Z]{95}$"}},
        {"stagenet", {"^5[0-9A-HJ-NP-Z]{94}$", "^A[0-9A-HJ-NP-Z]{95}$"}}
    };
    setPlaceholderText("Insert reception address");
    setStyleSheet("QLineEdit { background-color: white; color: black; font-size: 18px; border-radius: 25px; padding: 5px 15px; min-height: 50px; }");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(10);
    layout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addStretch();

    frame = new QFrame(this);
    frameLayout = new QHBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(2);
    frameLayout->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    frame->setStyleSheet("QFrame { background: rgba(0, 0, 0, 0.8); border: none; border-radius: 28px; padding: 2px; }");
    frame->setFixedSize(160, 56);
    frame->setVisible(false); // deactivate for now

    scanButton = new QPushButton(frame);
    pasteButton = new QPushButton(frame);
    addressBookButton = new QPushButton(frame);

    for (QPushButton *button : {scanButton, pasteButton, addressBookButton}) {
        button->setCursor(Qt::ArrowCursor);
        button->setFixedSize(m_buttonSize, m_buttonSize);
        button->setFixedSize(48, 48);
        button->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 24px; }");
        button->setIconSize(QSize(48, 48));
        frameLayout->addWidget(button);
    }

    connect(scanButton, &QPushButton::clicked, this, &QAddressLineEdit::scanClicked);
    connect(pasteButton, &QPushButton::clicked, this, &QAddressLineEdit::pasteClicked);
    connect(addressBookButton, &QPushButton::clicked, this, &QAddressLineEdit::addressBookClicked);
    connect(pasteButton, &QPushButton::clicked, [this, moneroRegex](){

        QClipboard *clipboard = QGuiApplication::clipboard();
        QString clipboardText = clipboard->text();

        for(const QString& pattern : moneroRegex["testnet"]) {
            QRegularExpression regex(pattern);
            if (regex.match(clipboardText).hasMatch()) {
                setText(clipboardText);
                qDebug() << "Clipboard content is a valid monero address.";
                return;
            }
        }
        qDebug() << "Clipboard content is not a valid monero address.";
    });

    scanButton->setIcon(QIcon(":/icons/icons/Camera.svg"));
    pasteButton->setIcon(QIcon(":/icons/icons/Past.svg"));
    addressBookButton->setIcon(QIcon(":/icons/icons/Bookmarks.svg"));

    layout->addWidget(frame);

    setTextMargins(10, 0, 172, 0);
    updateStyle();
}

void QAddressLineEdit::setButtonSize(int size) {
    if (m_buttonSize != size) {
        m_buttonSize = size;
        updateStyle();
    }
}

void QAddressLineEdit::setBorderSize(int size) {
    if (m_borderSize != size) {
        m_borderSize = size;
        updateStyle();
    }
}

void QAddressLineEdit::updateStyle() {
    QString style = QString("QLineEdit { background-color: white; color: black; font-size: 18px; "
                            "border-radius: %1px; padding: 5px 15px; min-height: %2px; }")
                        .arg(m_buttonSize / 2)
                        .arg(m_buttonSize + 10);
    setStyleSheet(style);

    for (QPushButton *button : {scanButton, pasteButton, addressBookButton}) {
        button->setFixedSize(m_buttonSize, m_buttonSize);
        button->setStyleSheet(QString("QPushButton { background: transparent; border: none; "
                                      "border-radius: %1px; }").arg(m_buttonSize / 2));
        button->setIconSize(QSize(m_buttonSize, m_buttonSize));
    }

    frameLayout->setSpacing(m_borderSize);
    frame->setFixedSize(m_buttonSize * 3 +  4 * m_borderSize, m_buttonSize + 2 * m_borderSize);
    frame->setStyleSheet(QString("QFrame { background: #333333; border: none; "
                                 "border-radius: %1px; padding: %2px; }").arg((m_buttonSize + 2 * m_borderSize) / 2).arg(m_borderSize));
    setTextMargins(10, 0, frame->width() + 2 * m_borderSize, 0);
}
