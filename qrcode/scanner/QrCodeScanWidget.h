// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: 2020-2024 The Monero Project

#ifndef QRCODESCANWIDGET_H
#define QRCODESCANWIDGET_H

#include <QWidget>
#include <QCamera>
#include <QScopedPointer>
#include <QTimer>

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
#include <QAbstractVideoSurface>
#include <QCameraViewfinder>
#else
#include <QMediaCaptureSession>
#include <QVideoSink>
#endif

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

    void setProcessColor(const QColor &color);
    void setProgressColor(const QColor &color);
    void setUnscannedUrColor(const QColor &color);
    void setScannedUrColor(const QColor &color);

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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QScopedPointer<QCameraViewfinder> m_viewfinder;
#else
    QMediaCaptureSession m_captureSession;
    QVideoSink m_sink;
#endif
    ur::URDecoder m_decoder;
    bool m_done = false;
    bool m_handleFrames = true;

    int m_borderSize = 4;
    int m_totalUrFrames = 0;
    int m_currentUrFrame = 0;
    QColor m_processColor = Qt::blue;
    QColor m_progressColor = Qt::green;
    QColor m_unscannedUrColor = Qt::blue;
    QColor m_scannedUrColor = Qt::green;

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
    int m_estimatedMilliseconds = 0;
    int m_progress = 0;

    void drawProcessingAnimation(QPainter &painter, const QRect &rect);
    void updateFrameState(FrameState state);
    void animateProcessing();
    void drawProgressAnimation(QPainter &painter, const QRect &rect);
    void drawUrFramesProgress(QPainter &painter, const QRect &rect);
    int calculateTotalPixels(const QRect &rect) const;
    QPoint getPointFromPixel(const QRect &rect, int pixel) const;

public slots:
    void onFrameStateIdle();
    void onFrameStateRecognized();
    void onFrameStateValidated();
    void onProcessingTimeEstimate(int estimatedMilliseconds = 5000);
    void onFrameStateProcessing(int estimatedMilliseconds = 5000);
    void onProgressUpdate(int percent);
    void onFrameStateProgress(int percent);
    void onFrameStateError();
    void onUrFrame(int currentFrame);
    void onTotalUrFrames(int totalFrames);
};

#endif //QRCODESCANWIDGET_H
