#ifndef QAMOUNTLINEEDIT_H
#define QAMOUNTLINEEDIT_H

#include <QLineEdit>
#include <QLabel>

class QAmountLineEdit : public QLineEdit
{
    Q_OBJECT

public:
    explicit QAmountLineEdit(QWidget *parent = nullptr);

private:
    QLabel *xmrLabel;
};

#endif // QAMOUNTLINEEDIT_H
