// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2024 The Monero Project

#include "QrCodeScanWidget.h"
#include "ui_QrCodeScanWidget.h"

#include <QPermission>
#include <QMediaDevices>
#include <QComboBox>
#include <QPainter>
#include <QDebug>
#include <algorithm>

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
    int totalLength = ((rect.width() + rect.height()) * 2) / m_borderSize - 4;
    m_animationProgress = m_animationProgress % totalLength; // make sure m_animationProgress < totalLength
    int currentLength = std::min((totalLength - 1) * 100, m_estimatedMicroSeconds / 10);
    if (currentLength <= 0)
        return;  // Animation completed, nothing to draw
    int currentStart = totalLength - m_animationProgress;
    int currentEnd = (currentStart + currentLength) % totalLength;
    int sides[] = {
        (rect.width() - m_borderSize) / m_borderSize,
        (rect.height() - m_borderSize) / m_borderSize,
        (rect.width() - m_borderSize) / m_borderSize,
        (rect.height() - m_borderSize) / m_borderSize
    };
    int starts[] = { 0, sides[1], sides[2], sides[3] };
    int ends[] = { starts[1] - 1, starts[2] - 1, starts[3] - 1, totalLength -1};
    for(int i = 0; i < 4; i++) {
        int start = starts[i];
        int end = ends[i];
        int size = sides[i];
        if(!(start <= currentStart <= end) && !(start <= currentEnd <= end))
            continue; // nothing to draw on that side
        bool reverse = i > 1;
        QPoint s;
        QPoint e;
        if(i % 2 == 0) {// horizontal
            s = QPoint(
                !reverse?(rect.left() + (currentStart - start) * m_borderSize):(rect.right() - (currentStart - start) * m_borderSize),
                !reverse?rect.top():(rect.bottom() - m_borderSize)
            );
            e = QPoint(
                !reverse?(rect.left() + m_borderSize + ((currentEnd - start) * m_borderSize)):(rect.right() - m_borderSize - ((currentEnd - start) * m_borderSize)),
                !reverse?(rect.top() + m_borderSize):rect.bottom()
                );
        } else { // vertical
            s = QPoint(
                !reverse?(rect.right() - m_borderSize):rect.left(),
                !reverse?(rect.top() + (currentStart - start) * m_borderSize):(rect.bottom() - (currentStart - start) * m_borderSize)
                );
            e = QPoint(
                !reverse?rect.right():(rect.left() + m_borderSize),
                !reverse?(rect.top() + (currentStart - start) * m_borderSize):(rect.bottom() - (currentStart - start) * m_borderSize)
                );
        }
        painter.drawRect(QRect(s, e));
    }
}

void QrCodeScanWidget::animateProcessing()
{
    m_animationProgress++;
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

void QrCodeScanWidget::onProgressUpdate(int percent) {
    m_progress = percent;
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
    if(state == m_frameState)
        return; //nothing to do.
    m_frameState = state;
    if (state == FrameState::Processing) {
        m_animationProgress = 0;
        m_animationTimer.start(50);
    } else {
        m_animationTimer.stop();
    }
    update();
}

void QrCodeScanWidget::setProcessColor(const QColor &color) { m_processColor = color; }
void QrCodeScanWidget::setProgressColor(const QColor &color) { m_progressColor = color; }
void QrCodeScanWidget::setUnscannedUrColor(const QColor &color) { m_unscannedUrColor = color; }
void QrCodeScanWidget::setScannedUrColor(const QColor &color) { m_scannedUrColor = color; }

void QrCodeScanWidget::onUrFrame(int currentFrame) {
    m_currentUrFrame = currentFrame;
    update();
}

void QrCodeScanWidget::onTotalUrFrames(int totalFrames) {
    m_totalUrFrames = totalFrames;
    update();
}

int QrCodeScanWidget::calculateTotalPixels(const QRect &rect) const {
    return (rect.height() * 2 + rect.width() * 2) / m_borderSize - 4;
}

void QrCodeScanWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect frameRect = rect().adjusted(m_borderSize/2, m_borderSize/2, -m_borderSize/2, -m_borderSize/2);

    QPen pen(Qt::black, m_borderSize);
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
        pen.setColor(m_processColor);
        painter.setPen(pen);
        drawProcessingAnimation(painter, frameRect);
        break;
    case FrameState::Progress:
        pen.setColor(m_progressColor);
        painter.setPen(pen);
        drawProgressAnimation(painter, frameRect);
        break;
    case FrameState::Error:
        painter.setBrush(QColor(255, 0, 0, 100)); // Semi-transparent red
        painter.drawRect(frameRect);
        break;
    }

    if (m_totalUrFrames > 0) {
        drawUrFramesProgress(painter, frameRect);
    }
}

void QrCodeScanWidget::drawProgressAnimation(QPainter &painter, const QRect &rect)
{
    int totalPixels = calculateTotalPixels(rect);
    int progressPixels = totalPixels * m_progress / 100;

    QPoint start = rect.topLeft();
    QPoint end = start;

    for (int i = 0; i < progressPixels; ++i) {
        QPoint nextPoint = getPointFromPixel(rect, i + 1);
        painter.drawLine(end, nextPoint);
        end = nextPoint;
    }
}

void QrCodeScanWidget::drawUrFramesProgress(QPainter &painter, const QRect &rect)
{
    int frameSize = qMin(rect.width(), rect.height()) / 5; // Adjust as needed
    int spacing = m_borderSize;
    int columns = rect.width() / (frameSize + spacing);
    int rows = (m_totalUrFrames + columns - 1) / columns;

    for (int i = 0; i < m_totalUrFrames; ++i) {
        int row = i / columns;
        int col = i % columns;
        QRect frameRect(rect.left() + col * (frameSize + spacing),
                        rect.top() + row * (frameSize + spacing),
                        frameSize, frameSize);

        if (i < m_currentUrFrame) {
            painter.fillRect(frameRect, m_scannedUrColor);
        } else {
            painter.fillRect(frameRect, m_unscannedUrColor);
        }

        if (i == m_currentUrFrame - 1) {
            // Pulse effect for the last scanned frame
            int pulseSize = 5; // Adjust as needed
            QRect pulseRect = frameRect.adjusted(-pulseSize, -pulseSize, pulseSize, pulseSize);
            painter.setPen(QPen(m_scannedUrColor, 2));
            painter.drawRect(pulseRect);
        }
    }
}

QPoint QrCodeScanWidget::getPointFromPixel(const QRect &rect, int pixel) const
{
    int totalPixels = calculateTotalPixels(rect);
    pixel = (pixel + totalPixels) % totalPixels; // Ensure positive value

    if (pixel < rect.width()) {
        return QPoint(rect.left() + pixel * m_borderSize, rect.top());
    }
    pixel -= rect.width();

    if (pixel < rect.height()) {
        return QPoint(rect.right(), rect.top() + pixel * m_borderSize);
    }
    pixel -= rect.height();

    if (pixel < rect.width()) {
        return QPoint(rect.right() - pixel * m_borderSize, rect.bottom());
    }
    pixel -= rect.width();

    return QPoint(rect.left(), rect.bottom() - pixel * m_borderSize);
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
