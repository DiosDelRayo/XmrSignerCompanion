#ifndef PROGRESSFRAME_H
#define PROGRESSFRAME_H

#include <QFrame>
#include <QTimer>
#include <QPainter>

class ProgressFrame : public QFrame
{
    Q_OBJECT

public:
    enum class State {
        Idle,
        Recognized,
        Validated,
        Processing
    };

    explicit ProgressFrame(QWidget *parent = nullptr);
    void setState(State state);

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void updateProgress();

private:
    State m_currentState;
    QTimer m_timer;
    int m_progressAngle;
};

#endif // PROGRESSFRAME_H
