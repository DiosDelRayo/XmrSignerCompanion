#ifndef QAMOUNTLINEEDIT_H
#define QAMOUNTLINEEDIT_H

#include <QLineEdit>
#include <QLabel>

class QAmountLineEdit : public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(int labelWidth READ labelWidth WRITE setLabelWidth)

public:
    explicit QAmountLineEdit(QWidget *parent = nullptr);
    int labelWidth() const { return m_labelWidth; }
    void setLabelWidth(int width);

private:
    void updateStyle();
    QLabel *xmrLabel;
    int m_labelWidth = 50;
};

#endif // QAMOUNTLINEEDIT_H
