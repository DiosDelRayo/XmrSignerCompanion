#ifndef QADDRESSLINEEDIT_H
#define QADDRESSLINEEDIT_H

#include "qlineedit.h"
#include "qpushbutton.h"
class QAddressLineEdit: public QLineEdit
{
    Q_OBJECT

public:
    QAddressLineEdit(QWidget *parent = nullptr);

private:
    QPushButton *scanButton;
    QPushButton *pasteButton;
    QPushButton *addressBookButton;
};

#endif // QADDRESSLINEEDIT_H
