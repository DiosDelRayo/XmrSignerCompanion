// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2024 The Monero Project

#ifndef FEATHER_URWIDGET_H
#define FEATHER_URWIDGET_H

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
};

#endif //FEATHER_URWIDGET_H
