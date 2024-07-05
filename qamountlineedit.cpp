#include "qamountlineedit.h"
#include <QHBoxLayout>

QAmountLineEdit::QAmountLineEdit(QWidget *parent) : QLineEdit(parent) {
    setStyleSheet("QLineEdit { background-color: white; color: black; font-size: 18px; border-radius: 25px; padding: 5px 15px; min-height: 50px; }");
    setPlaceholderText("0.00");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 0, 0, 0);
    layout->setSpacing(5);
    layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    xmrLabel = new QLabel("XMR:", this);
    xmrLabel->setStyleSheet("QLabel { color: black; font-weight: bold; }");
    layout->addWidget(xmrLabel);

    setTextMargins(50, 0, 10, 0);
}
