// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2024 The Monero Project

#include "URWidget.h"

#include "URSettingsDialog.h"
#include "ui_URWidget.h"

URWidget::URWidget(QWidget *parent)
        : QWidget(parent)
        , ui(new Ui::URWidget)
{
    ui->setupUi(this);

    connect(&m_timer, &QTimer::timeout, this, &URWidget::nextQR);
    connect(ui->btn_options, &QPushButton::clicked, this, &URWidget::setOptions);
}

void URWidget::setData(const QString &type, const std::string &data) {
    m_type = type;
    m_data = data;
    
    m_timer.stop();
    allParts.clear();
    
    if (m_data.empty()) {
        return;
    }
    
    std::string type_std = m_type.toStdString();

    ur::ByteVector a = ur::string_to_bytes(m_data);
    ur::ByteVector cbor;
    ur::CborLite::encodeBytes(cbor, a);
    ur::UR h = ur::UR(type_std, cbor);

    int bytesPerFragment = m_fragmentLength;

    delete m_urencoder;
    m_urencoder = new ur::UREncoder(h, bytesPerFragment);

    for (int i=0; i < m_urencoder->seq_len(); i++) {
        allParts.append(m_urencoder->next_part());
    }

    m_timer.setInterval(m_speed);
    m_timer.start();
}

void URWidget::nextQR() {
    currentIndex = currentIndex % m_urencoder->seq_len();

    std::string data;
    if (m_fountainCodeEnabled) {
        data = m_urencoder->next_part();
    } else {
        data = allParts[currentIndex];
    }
    
    ui->label_seq->setText(QString("%1/%2").arg(QString::number(currentIndex % m_urencoder->seq_len() + 1), QString::number(m_urencoder->seq_len())));

    m_code = new QrCode{QString::fromStdString(data), QrCode::Version::AUTO, QrCode::ErrorCorrectionLevel::MEDIUM};
    ui->qrWidget->setQrCode(m_code);
    
    currentIndex += 1;
}

void URWidget::setOptions() {
    URSettingsDialog dialog{this, m_fragmentLength, m_speed, m_fountainCodeEnabled};
    dialog.exec();
    this->setData(m_type, m_data);
}

void URWidget::onSettingsChanged(int fragmentLength, int speed, bool fountainCodeEnabled) {
    m_fragmentLength = fragmentLength;
    m_speed = speed;
    m_fountainCodeEnabled = fountainCodeEnabled;
}

URWidget::~URWidget() {
    delete m_urencoder;
}
