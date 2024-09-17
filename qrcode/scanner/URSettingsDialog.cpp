// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2024 The Monero Project

#include "URSettingsDialog.h"
#include "ui_URSettingsDialog.h"

URSettingsDialog::URSettingsDialog(QWidget *parent, int fragmentLength, int speed, bool fountainCodeEnabled)
        : QDialog(parent)
        , ui(new Ui::URSettingsDialog)
{
#ifndef Q_OS_MACOS
    this->setWindowModality(Qt::WindowModal);
#endif

    ui->setupUi(this);

    this->updateSettings(fragmentLength, speed, fountainCodeEnabled);

    connect(ui->spin_fragmentLength, &QSpinBox::valueChanged, [this](int value){
	emit this->fragmentLengthChanged(value);
	this->onUpdate();
    });
    connect(ui->spin_speed, &QSpinBox::valueChanged, [this](int value){
	emit this->fragmentLengthChanged(value);
	this->onUpdate();
    });
    connect(ui->check_fountainCode, &QCheckBox::toggled, [this](bool toggled){
	emit this->fountainCodeEnabledChanged(toggled);
	this->onUpdate();
    });
    
    connect(ui->btn_reset, &QPushButton::clicked, [this]{
	this->updateSettings(this->defaultFragmentLength, this->defaultSpeed, this->defaultFountainCodeEnabled);
	this->onUpdate();
    });
   
    this->adjustSize();
}

void URSettingsDialog::setDefault(int fragmentLength, int speed, bool fountainCodeEnabled) {
	this->defaultFragmentLength = fragmentLength;
	this->defaultSpeed = speed;
	this->defaultFountainCodeEnabled = fountainCodeEnabled;
}

int URSettingsDialog::getFragmentLength() const {
    return ui->spin_fragmentLength->value();
}

int URSettingsDialog::getSpeed() const {
        return ui->spin_fragmentLength->value();
}

bool URSettingsDialog::getFountainCodeEnabled() const {
        return ui->check_fountainCode->isChecked();
}

void URSettingsDialog::onUpdate() {
	emit this->settingsChanged(this->getFragmentLength(), this->getSpeed(), this->getFountainCodeEnabled());
}

void URSettingsDialog::updateSettings(int fragmentLength, int speed, bool fountainCodeEnabled) {
	this->setFragmentLength(fragmentLength);
	this->setSpeed(speed);
	this->setFountainCodeEnabled(fountainCodeEnabled);
}

void URSettingsDialog::setFragmentLength(int value) {
    ui->spin_fragmentLength->setValue(value);
}

void URSettingsDialog::setSpeed(int value) {
    ui->spin_speed->setValue(value);
}

void URSettingsDialog::setFountainCodeEnabled(bool enabled) {
    ui->check_fountainCode->setChecked(enabled);
}

URSettingsDialog::~URSettingsDialog() = default;
