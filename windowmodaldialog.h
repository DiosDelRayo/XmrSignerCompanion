#ifndef WINDOWMODALDIALOG_H
#define WINDOWMODALDIALOG_H
#include <QDialog>

class WindowModalDialog : public QDialog {
    Q_OBJECT

public:
    explicit WindowModalDialog(QWidget *parent);
};

#endif // WINDOWMODALDIALOG_H
