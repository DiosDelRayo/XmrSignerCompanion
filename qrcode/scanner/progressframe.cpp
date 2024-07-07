#include "progressframe.h"

ProgressFrame::ProgressFrame(QWidget *parent)
    : QFrame(parent), m_currentState(State::Idle), m_progressAngle(0)
{
    setFrameStyle(QFrame::NoFrame);
    setFixedSize(100, 100); // Adjust size as needed

    connect(&m_timer, &QTimer::timeout, this, &ProgressFrame::updateProgress);
}

void ProgressFrame::setState(State state)
{
    m_currentState = state;
    m_progressAngle = 0;

    switch (m_currentState) {
    case State::Idle:
        m_timer.stop();
        break;
    case State::Recognized:
        m_timer.stop();
        break;
    case State::Validated:
        m_timer.stop();
        break;
    case State::Processing:
        m_timer.start(50); // Start timer for processing animation
        break;
    }
    update();
}

void ProgressFrame::paintEvent(QPaintEvent *event)
{
    QFrame::paintEvent(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    QRect frameRect = rect().adjusted(2, 2, -2, -2);
    QPen pen(Qt::black, 2);
    painter.setPen(pen);

    switch (m_currentState) {
    case State::Idle:
        painter.setBrush(Qt::lightGray);
        painter.drawRect(frameRect);
        break;
    case State::Recognized:
        painter.setBrush(Qt::yellow);
        painter.drawRect(frameRect);
        break;
    case State::Validated:
        painter.setBrush(Qt::green);
        painter.drawRect(frameRect);
        break;
    case State::Processing:
        painter.setBrush(Qt::lightGray);
        painter.drawRect(frameRect);
        painter.setPen(QPen(Qt::black, 4));
        painter.drawArc(frameRect, 0, m_progressAngle * 16);
        break;
    }
}

void ProgressFrame::updateProgress()
{
    m_progressAngle += 10;
    if (m_progressAngle >= 360) {
        m_progressAngle = 0;
    }
    update();
}
