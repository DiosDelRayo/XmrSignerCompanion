#include "qaddresslineedit.h"
#include <QLineEdit>
#include <QPushButton>
#include <QHBoxLayout>

QAddressLineEdit::QAddressLineEdit(QWidget *parent): QLineEdit(parent) {
    setPlaceholderText("Insert reception address");
    setStyleSheet("QLineEdit { background-color: white; color: black; font-size: 18px; border-radius: 25px; padding: 5px 15px; min-height: 50px; }");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(0, 0, 10, 0);
    layout->setSpacing(5);
    layout->setAlignment(Qt::AlignRight | Qt::AlignVCenter);

    scanButton = new QPushButton(this);
    pasteButton = new QPushButton(this);
    addressBookButton = new QPushButton(this);

    for (QPushButton *button : {scanButton, pasteButton, addressBookButton}) {
        button->setFixedSize(40, 40);
        button->setStyleSheet("QPushButton { background: transparent; border: none; }");
        layout->addWidget(button);
    }

    scanButton->setIcon(QIcon(":/icons/icons/Camera.svg"));
    pasteButton->setIcon(QIcon(":/icons/icons/Past.svg"));
    addressBookButton->setIcon(QIcon(":/icons/icons/Répertoire.svg"));

    setTextMargins(10, 0, 135, 0);
}
