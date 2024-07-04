#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "qdotprogressindicator.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    bool checkLogic();

private slots:
    void syncDotIndicator(int index);

private:
    Ui::MainWindow *ui;
    QDotProgressIndicator* m_dotIndicator;
};
#endif // MAINWINDOW_H
