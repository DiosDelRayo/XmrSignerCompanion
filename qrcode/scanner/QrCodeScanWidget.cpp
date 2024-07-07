// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2024 The Monero Project

#include "QrCodeScanWidget.h"
#include "ui_QrCodeScanWidget.h"

#include <QPermission>
#include <QMediaDevices>
#include <QComboBox>
#include <QPainter>
#include <QDebug>

QrCodeScanWidget::QrCodeScanWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::QrCodeScanWidget)
    , m_sink(new QVideoSink(this))
    , m_thread(new QrScanThread(this))
    , m_frameState(FrameState::Idle)
    , m_animationProgress(0)
{
    ui->setupUi(this);
    
    int framePadding = 5; // Adjust this value as needed
    ui->verticalLayout->setContentsMargins(framePadding, framePadding, framePadding, framePadding);

    this->setWindowTitle("Scan QR code");
    
    ui->frame_error->hide();
    ui->frame_error->setInfo(QIcon(":/icons/icons/warning.png"), "Lost connection to camera");

    connect(&m_animationTimer, &QTimer::timeout, this, &QrCodeScanWidget::animateProcessing);

    this->refreshCameraList();
    
    connect(ui->combo_camera, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &QrCodeScanWidget::onCameraSwitched);
    connect(ui->viewfinder->videoSink(), &QVideoSink::videoFrameChanged, this, &QrCodeScanWidget::handleFrameCaptured);
    connect(ui->btn_refresh, &QPushButton::clicked, [this]{
        this->refreshCameraList();
        this->onCameraSwitched(0);
    });
    connect(m_thread, &QrScanThread::decoded, this, &QrCodeScanWidget::onDecoded);

    connect(ui->check_manualExposure, &QCheckBox::toggled, [this](bool enabled) {
        if (!m_camera) {
            return;
        }

        ui->slider_exposure->setVisible(enabled);
        if (enabled) {
            m_camera->setExposureMode(QCamera::ExposureManual);
        } else {
            // Qt-bug: this does not work for cameras that only support V4L2_EXPOSURE_APERTURE_PRIORITY
            // Check with v4l2-ctl -L
            m_camera->setExposureMode(QCamera::ExposureAuto);
        }
        // conf()->set(Config::cameraManualExposure, enabled);
    });

    connect(ui->slider_exposure, &QSlider::valueChanged, [this](int value) {
        if (!m_camera) {
            return;
        }

        float exposure = 0.00033 * value;
        m_camera->setExposureMode(QCamera::ExposureManual);
        m_camera->setManualExposureTime(exposure);
        // conf()->set(Config::cameraExposureTime, value);
    });

    ui->check_manualExposure->setVisible(false);
    ui->slider_exposure->setVisible(false);
}

void QrCodeScanWidget::drawProcessingAnimation(QPainter &painter, const QRect &rect)
{
    int totalLength = (rect.width() + rect.height()) * 2;
    int currentLength = totalLength - (m_animationProgress * totalLength / 100);

    if (currentLength == 0) {
        return;  // Animation completed, nothing to draw
    }

    int startSide = (m_animationProgress * 4) / 100;  // 0: top, 1: right, 2: bottom, 3: left
    QPoint start;

    switch (startSide) {
        case 0: start = QPoint(rect.right(), rect.top()); break;
        case 1: start = QPoint(rect.right(), rect.bottom()); break;
        case 2: start = QPoint(rect.left(), rect.bottom()); break;
        case 3: start = QPoint(rect.left(), rect.top()); break;
    }

    QPoint end = start;
    int remainingLength = currentLength;

    for (int side = 0; side < 4; ++side) {
        int currentSide = (startSide + side) % 4;
        int sideLength;

        switch (currentSide) {
            case 0: // top
            case 2: // bottom
                sideLength = rect.width();
                break;
            case 1: // right
            case 3: // left
                sideLength = rect.height();
                break;
        }

        if (remainingLength > sideLength) {
            switch (currentSide) {
                case 0: end.setX(rect.left()); break;
                case 1: end.setY(rect.top()); break;
                case 2: end.setX(rect.right()); break;
                case 3: end.setY(rect.bottom()); break;
            }
            painter.drawLine(start, end);
            start = end;
            remainingLength -= sideLength;
        } else {
            switch (currentSide) {
                case 0: end.setX(start.x() - remainingLength); break;
                case 1: end.setY(start.y() - remainingLength); break;
                case 2: end.setX(start.x() + remainingLength); break;
                case 3: end.setY(start.y() + remainingLength); break;
            }
            painter.drawLine(start, end);
            break;
        }
    }
}

void QrCodeScanWidget::animateProcessing()
{
    m_animationProgress += 1;
    if (m_animationProgress >= 100) {
        m_animationProgress = 0;
    }
    update();
}

void QrCodeScanWidget::onFrameStateIdle() {
    updateFrameState(FrameState::Idle);
}

void QrCodeScanWidget::onFrameStateRecognized() {
    updateFrameState(FrameState::Recognized);
}

void QrCodeScanWidget::onFrameStateValidated() {
    updateFrameState(FrameState::Validated);
}

void QrCodeScanWidget::onFrameStateProgress() {
    updateFrameState(FrameState::Progress);
}

void QrCodeScanWidget::onProgressUpdate(int percent) {
    m_progress = percent;
}

void QrCodeScanWidget::onFrameStateProcessing(){
    updateFrameState(FrameState::Processing);
    if(m_estimatedMicroSeconds < 5000)
        m_estimatedMicroSeconds = 5000;
}

void QrCodeScanWidget::onProcessingTimeEstimate(int estimatedMicroSeconds) {
    m_estimatedMicroSeconds = estimatedMicroSeconds;
}

void QrCodeScanWidget::onFrameStateProcessing(int estimatedMicroSeconds) {
    if(m_frameState != FrameState::Processing) {
        updateFrameState(FrameState::Processing);
        m_estimatedMicroSeconds = estimatedMicroSeconds;
    } else {
        m_estimatedMicroSeconds += estimatedMicroSeconds;
    }
}

void QrCodeScanWidget::onFrameStateProgress(int percent) {
    if(m_frameState != FrameState::Progress)
        updateFrameState(FrameState::Progress);
    m_progress = percent;
}

void QrCodeScanWidget::onFrameStateError() {
    updateFrameState(FrameState::Error);
}

void QrCodeScanWidget::updateFrameState(FrameState state)
{
    m_frameState = state;
    if (state == FrameState::Processing) {
        m_animationProgress = 0;
        m_animationTimer.start(50);
    } else {
        m_animationTimer.stop();
    }
    update();
}

void QrCodeScanWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int penWidth = 4;
    QRect frameRect = rect().adjusted(penWidth/2, penWidth/2, -penWidth/2, -penWidth/2);

    QPen pen(Qt::black, penWidth);
    painter.setPen(pen);

    switch (m_frameState) {
    case FrameState::Idle:
        // Don't draw anything in idle state
        break;
    case FrameState::Recognized:
        painter.setBrush(QColor(255, 255, 0, 100)); // Semi-transparent yellow
        painter.drawRect(frameRect);
        break;
    case FrameState::Validated:
        painter.setBrush(QColor(0, 255, 0, 100)); // Semi-transparent green
        painter.drawRect(frameRect);
        break;
    case FrameState::Processing:
        // Draw the processing animation
        pen.setColor(Qt::blue);
        painter.setPen(pen);
        drawProcessingAnimation(painter, frameRect);
        break;
    }
}

void QrCodeScanWidget::startCapture(bool scan_ur) {
    m_scan_ur = scan_ur;
    ui->progressBar_UR->setVisible(m_scan_ur);
    ui->progressBar_UR->setFormat("Progress: %v%");

    updateFrameState(FrameState::Idle);

    QCameraPermission cameraPermission;
    switch (qApp->checkPermission(cameraPermission)) {
        case Qt::PermissionStatus::Undetermined:
            qDebug() << "Camera permission undetermined";
            qApp->requestPermission(cameraPermission, [this] {
                startCapture(m_scan_ur);
            });
            return;
        case Qt::PermissionStatus::Denied:
            ui->frame_error->setText("No permission to start camera.");
            ui->frame_error->show();
            qDebug() << "No permission to start camera.";
            return;
        case Qt::PermissionStatus::Granted:
            qDebug() << "Camera permission granted";
            break;
    }

    if (ui->combo_camera->count() < 1) {
        ui->frame_error->setText("No cameras found. Attach a camera and press 'Refresh'.");
        ui->frame_error->show();
        qDebug() << "No cameras found. Attach a camera and press 'Refresh'.";
        return;
    }
    
    this->onCameraSwitched(0);
    
    if (!m_thread->isRunning()) {
        m_thread->start();
    }
}

void QrCodeScanWidget::reset() {
    this->decodedString = "";
    m_done = false;
    ui->progressBar_UR->setValue(0);
    m_decoder = ur::URDecoder();
    m_thread->start();
    m_handleFrames = true;
}

void QrCodeScanWidget::stop() {
    m_camera->stop();
    m_thread->stop();
}

void QrCodeScanWidget::pause() {
    m_handleFrames = false;
}

void QrCodeScanWidget::refreshCameraList() {
    ui->combo_camera->clear();
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();
    for (const auto &camera : cameras) {
        ui->combo_camera->addItem(camera.description());
    }
}

void QrCodeScanWidget::handleFrameCaptured(const QVideoFrame &frame) {
    if (!m_handleFrames) {
        return;
    }
    
    if (!m_thread->isRunning()) {
        return;
    }

    QImage img = this->videoFrameToImage(frame);
    if (img.format() == QImage::Format_ARGB32) {
        m_thread->addImage(img);
    }
}

QImage QrCodeScanWidget::videoFrameToImage(const QVideoFrame &videoFrame)
{
    QImage image = videoFrame.toImage();

    if (image.isNull()) {
        return {};
    }

    if (image.format() != QImage::Format_ARGB32) {
        image = image.convertToFormat(QImage::Format_ARGB32);
    }

    return image.copy();
}


void QrCodeScanWidget::onCameraSwitched(int index) {
    const QList<QCameraDevice> cameras = QMediaDevices::videoInputs();

    if (index < 0) {
        return;
    }
    
    if (index >= cameras.size()) {
        return;
    }

    if (m_camera) {
        m_camera->stop();
    }

    ui->frame_error->setVisible(false);

    m_camera.reset(new QCamera(cameras.at(index), this));
    m_captureSession.setCamera(m_camera.data());
    m_captureSession.setVideoOutput(ui->viewfinder);

    bool manualExposureSupported = m_camera->isExposureModeSupported(QCamera::ExposureManual);
    ui->check_manualExposure->setVisible(manualExposureSupported);

    qDebug() << "Supported camera features: " << m_camera->supportedFeatures();
    qDebug() << "Current focus mode: " << m_camera->focusMode();
    if (m_camera->isExposureModeSupported(QCamera::ExposureBarcode)) {
        qDebug() << "Barcode exposure mode is supported";
    }

    connect(m_camera.data(), &QCamera::activeChanged, [this](bool active){
        ui->frame_error->setText("Lost connection to camera");
        ui->frame_error->setVisible(!active);
        if (!active)
                qDebug() << "Lost connection to camera";
    });

    connect(m_camera.data(), &QCamera::errorOccurred, [this](QCamera::Error error, const QString &errorString) {
        if (error == QCamera::Error::CameraError) {
            ui->frame_error->setText(QString("Error: %1").arg(errorString));
            ui->frame_error->setVisible(true);
            qDebug() << QString("Error: %1").arg(errorString);
        }
    });

    m_camera->start();

    // bool useManualExposure = conf()->get(Config::cameraManualExposure).toBool() && manualExposureSupported;
    // ui->check_manualExposure->setChecked(useManualExposure);
    // if (useManualExposure) {
    //    ui->slider_exposure->setValue(conf()->get(Config::cameraExposureTime).toInt());
    //}
    ui->slider_exposure->setValue(1);
}

void QrCodeScanWidget::onDecoded(const QString &data) {
    if (m_done) {
        return;
    }
    
    if (m_scan_ur) {
        bool success = m_decoder.receive_part(data.toStdString());
        if (!success) {
          return;
        }

    updateFrameState(FrameState::Recognized);

        ui->progressBar_UR->setValue(m_decoder.estimated_percent_complete() * 100);
        ui->progressBar_UR->setMaximum(100);

        if (m_decoder.is_complete()) {
            m_done = true;
            m_thread->stop();
            emit finished(m_decoder.is_success());
        }

        return;
    }

    decodedString = data;
    m_done = true;
    m_thread->stop();
    emit finished(true);
}

std::string QrCodeScanWidget::getURData() {
    if (!m_decoder.is_success()) {
        return "";
    }

    ur::ByteVector cbor = m_decoder.result_ur().cbor();
    std::string data;
    auto i = cbor.begin();
    auto end = cbor.end();
    ur::CborLite::decodeBytes(i, end, data);
    return data;
}

std::string QrCodeScanWidget::getURType() {
    if (!m_decoder.is_success()) {
        return "";
    }

    return m_decoder.expected_type().value_or("");
}

QString QrCodeScanWidget::getURError() {
    if (!m_decoder.is_failure()) {
        return {};
    }
    return QString::fromStdString(m_decoder.result_error().what());
}

QrCodeScanWidget::~QrCodeScanWidget()
{
    m_thread->stop();
    m_thread->quit();
    if (!m_thread->wait(5000))
    {
        m_thread->terminate();
        m_thread->wait();
    }
}
