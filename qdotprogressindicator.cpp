#include "qdotprogressindicator.h"
#include <QPainter>

QDotProgressIndicator::QDotProgressIndicator(QWidget *parent)
    : QWidget(parent), m_totalSteps(10), m_currentStep(0)
{
    setFixedHeight(40); // Adjust as needed
}

void QDotProgressIndicator::setTotalSteps(int steps)
{
    m_totalSteps = steps;
    update();
}

void QDotProgressIndicator::setCurrentStep(int step)
{
    if (step != m_currentStep && step >= 0 && step < m_totalSteps) {
        m_currentStep = step;
        update();
    }
}

void QDotProgressIndicator::next()
{
    if (m_currentStep < m_totalSteps - 1) {
        setCurrentStep(m_currentStep + 1);
    }
}

void QDotProgressIndicator::previous()
{
    if (m_currentStep > 0) {
        setCurrentStep(m_currentStep - 1);
    }
}

void QDotProgressIndicator::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    int dotDiameter = height() / 2;
    int spacing = width() / m_totalSteps;

    for (int i = 0; i < m_totalSteps; ++i) {
        QColor dotColor = (i == m_currentStep) ? Qt::white : Qt::gray;
        painter.setBrush(dotColor);
        painter.setPen(Qt::NoPen);

        int x = i * spacing + spacing / 2 - dotDiameter / 2;
        int y = height() / 2 - dotDiameter / 2;
        painter.drawEllipse(x, y, dotDiameter, dotDiameter);
    }
}
