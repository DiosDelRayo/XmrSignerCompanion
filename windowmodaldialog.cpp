#include "windowmodaldialog.h"

WindowModalDialog::WindowModalDialog(QWidget *parent)
    : QDialog(parent)
{
#ifndef Q_OS_MACOS
    this->setWindowModality(Qt::WindowModal);
#endif
}
