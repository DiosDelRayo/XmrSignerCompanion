#include "qamountlineedit.h"
#include <QHBoxLayout>

QAmountLineEdit::QAmountLineEdit(QWidget *parent) : QLineEdit(parent){
    setStyleSheet("QLineEdit { background-color: white; color: black; font-size: 18px; border-radius: 25px; padding: 5px 15px; min-height: 50px; }");

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(15, 0, 15, 0);
    layout->setSpacing(0);

    xmrLabel = new QLabel("XMR:", this);
    xmrLabel->setStyleSheet("QLabel { color: black; font-size: 36px; font-weight: bold; background: transparent; }");
    layout->addWidget(xmrLabel);

    layout->addSpacing(5);  // Add some space between label and text

    setTextMargins(m_labelWidth + 55, 0, 10, 0);
}

void QAmountLineEdit::setLabelWidth(int width) {
    if (m_labelWidth != width) {
        m_labelWidth = width;
        updateStyle();
    }
}

void QAmountLineEdit::updateStyle() {
    xmrLabel->setFixedWidth(m_labelWidth);
    setTextMargins(m_labelWidth + 20, 0, 10, 0);
}
