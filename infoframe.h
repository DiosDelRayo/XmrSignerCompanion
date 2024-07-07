#ifndef INFOFRAME_H
#define INFOFRAME_H
#include <QPushButton>
#include <QFrame>
#include <QLabel>
#include <QDialog>

class InfoFrame : public QFrame {
    Q_OBJECT

public:
    explicit InfoFrame(QWidget *parent);
    void setInfo(const QIcon &icon, const QString &text);
    void setText(const QString &text);

private:
    QPushButton *m_icon;
    QLabel *m_infoLabel;
};


#endif // INFOFRAME_H
