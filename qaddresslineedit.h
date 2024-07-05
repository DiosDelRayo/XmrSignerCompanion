#ifndef QADDRESSLINEEDIT_H
#define QADDRESSLINEEDIT_H

#include "qlineedit.h"
#include "qpushbutton.h"
class QAddressLineEdit: public QLineEdit
{
    Q_OBJECT
    Q_PROPERTY(int buttonSize READ buttonSize WRITE setButtonSize)
    Q_PROPERTY(int borderSize READ borderSize WRITE setBorderSize)

public:
    explicit QAddressLineEdit(QWidget *parent = nullptr);
    int buttonSize() const { return m_buttonSize; }
    void setButtonSize(int size);
    int borderSize() const { return m_buttonSize; }
    void setBorderSize(int size);

signals:
    void scanClicked();
    void pasteClicked();
    void addressBookClicked();

private:
    void updateStyle();
    QFrame *frame;
    QLayout *frameLayout;
    QPushButton *scanButton;
    QPushButton *pasteButton;
    QPushButton *addressBookButton;
    int m_buttonSize = 48;
    int m_borderSize = 5;
};

#endif // QADDRESSLINEEDIT_H
