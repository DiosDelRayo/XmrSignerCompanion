// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2024 The Monero Project

#ifndef FEATHER_QRCODESCANWIDGET_H
#define FEATHER_QRCODESCANWIDGET_H

#include <QWidget>
#include <QCamera>
#include <QScopedPointer>
#include <QMediaCaptureSession>
#include <QTimer>
#include <QVideoSink>

#include "QrScanThread.h"

#include <../bcur/bc-ur.hpp>
#include <../bcur/ur-decoder.hpp>

namespace Ui {
    class QrCodeScanWidget;
}

class QrCodeScanWidget : public QWidget 
{
    Q_OBJECT

public:
    explicit QrCodeScanWidget(QWidget *parent);
    ~QrCodeScanWidget() override;

    QString decodedString = "";
    std::string getURData();
    std::string getURType();
    QString getURError();
    
    void startCapture(bool scan_ur = false);
    void reset();
    void stop();
    void pause();

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void finished(bool success);
    
private slots:
    void onCameraSwitched(int index);
    void onDecoded(const QString &data);

private:
    void refreshCameraList();
    QImage videoFrameToImage(const QVideoFrame &videoFrame);
    void handleFrameCaptured(const QVideoFrame &videoFrame);

    QScopedPointer<Ui::QrCodeScanWidget> ui;

    bool m_scan_ur = false;
    QrScanThread *m_thread;
    QScopedPointer<QCamera> m_camera;
    QMediaCaptureSession m_captureSession;
    QVideoSink m_sink;
    ur::URDecoder m_decoder;
    bool m_done = false;
    bool m_handleFrames = true;

    enum class FrameState {
        Idle,
        Recognized,
        Validated,
        Processing,
        Progress,
        Error
    };
    FrameState m_frameState;
    QTimer m_animationTimer;
    int m_animationProgress;
    int m_estimatedMicroSeconds = 0;
    int m_progress = 0;

    void drawProcessingAnimation(QPainter &painter, const QRect &rect);
    void updateFrameState(FrameState state);
    void animateProcessing();

public slots:
    void onFrameStateIdle();
    void onFrameStateRecognized();
    void onFrameStateValidated();
    void onFrameStateProcessing();
    void onProcessingTimeEstimate(int estimatedMicroSeconds);
    void onFrameStateProcessing(int estimatedMicroSeconds);
    void onFrameStateProgress();
    void onProgressUpdate(int percent);
    void onFrameStateProgress(int percent);
    void onFrameStateError();
};

#endif //FEATHER_QRCODESCANWIDGET_H
