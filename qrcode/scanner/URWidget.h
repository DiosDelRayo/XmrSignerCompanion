// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2024 The Monero Project

#ifndef URWIDGET_H
#define URWIDGET_H

#include <QWidget>
#include <QTimer>

#include "qrcode/QrCode.h"
#include <bcur/bc-ur.hpp>

namespace Ui {
    class URWidget;
}

class URWidget : public QWidget
{
    Q_OBJECT

public:
    explicit URWidget(QWidget *parent = nullptr);
    ~URWidget();
    
    void setData(const QString &type, const std::string &data);

public slots:
    void onSettingsChanged(int fragmentLength, int speed, bool fountainCodeEnabled);

private slots:
    void nextQR();
    void setOptions();

private:
    QScopedPointer<Ui::URWidget> ui;
    QTimer m_timer;
    ur::UREncoder *m_urencoder = nullptr;
    QrCode *m_code = nullptr;
    QList<std::string> allParts;
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    qsizetype currentIndex = 0;
#else
    int currentIndex = 0;
#endif
    
    std::string m_data;
    QString m_type;
    int m_fragmentLength = 150;
    int m_speed = 80;
    bool m_fountainCodeEnabled = false;
};

#endif //URWIDGET_H
