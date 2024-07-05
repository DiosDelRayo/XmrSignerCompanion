#include "qaddresslineedit.h"
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

QAddressLineEdit::QAddressLineEdit(QWidget *parent): QLineEdit(parent) {
    setPlaceholderText("Insert reception address");
    setStyleSheet("QLineEdit { background-color: white; color: black; font-size: 18px; border-radius: 25px; padding: 5px 15px; min-height: 50px; }");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 0, 10, 0);
    layout->setSpacing(10);
    layout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    layout->addStretch();

    QFrame *frame = new QFrame(this);
    QHBoxLayout *frameLayout = new QHBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 0, 0);
    frameLayout->setSpacing(2);
    frameLayout->setAlignment(Qt::AlignCenter | Qt::AlignVCenter);
    frame->setStyleSheet("QFrame { background: rgba(0, 0, 0, 0.8); border: none; border-radius: 28px; padding: 2px; }");
    frame->setFixedSize(160, 56);

    scanButton = new QPushButton(frame);
    pasteButton = new QPushButton(frame);
    addressBookButton = new QPushButton(frame);

    for (QPushButton *button : {scanButton, pasteButton, addressBookButton}) {
        button->setFixedSize(48, 48);
        button->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 24px; }");
        button->setIconSize(QSize(48, 48));
        frameLayout->addWidget(button);
    }

    scanButton->setIcon(QIcon(":/icons/icons/Camera.svg"));
    pasteButton->setIcon(QIcon(":/icons/icons/Past.svg"));
    addressBookButton->setIcon(QIcon(":/icons/icons/Bookmarks.svg"));

    layout->addWidget(frame);

    setTextMargins(10, 0, 172, 0);
}
