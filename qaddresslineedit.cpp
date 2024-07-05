#include "qaddresslineedit.h"
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

QAddressLineEdit::QAddressLineEdit(QWidget *parent): QLineEdit(parent) {
    setPlaceholderText("Insert reception address");
    setStyleSheet("QLineEdit { background-color: white; color: black; font-size: 18px; border-radius: 25px; padding: 5px 15px; min-height: 50px; }");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 10, 0);
    layout->setSpacing(10);
    layout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    QFrame *frame = new QFrame(this);
    QHBoxLayout *frameLayout = new QHBoxLayout(frame);
    frameLayout->setContentsMargins(0, 0, 10, 0);
    frameLayout->setSpacing(0);
    frameLayout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    frame->setStyleSheet("QFrame { background: green; border: none; border-radius: 32px; }");
    frame->setFixedHeight(70);
    scanButton = new QPushButton(frame);
    pasteButton = new QPushButton(frame);
    addressBookButton = new QPushButton(frame);

    for (QPushButton *button : {scanButton, pasteButton, addressBookButton}) {
        button->setFixedSize(64, 64);
        button->setStyleSheet("QPushButton { background: transparent; border: none; border-radius: 32px; }");
        button->setIconSize(QSize(48, 48));
        layout->addWidget(button);
    }

    scanButton->setIcon(QIcon(":/icons/icons/Camera.svg"));
    pasteButton->setIcon(QIcon(":/icons/icons/Past.svg"));
    addressBookButton->setIcon(QIcon(":/icons/icons/Bookmarks.svg"));

    setTextMargins(10, 0, 135, 0);
}
