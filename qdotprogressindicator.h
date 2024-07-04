#ifndef QDOTPROGRESSINDICATOR_H
#define QDOTPROGRESSINDICATOR_H

#include <QWidget>

class QDotProgressIndicator : public QWidget
{
    Q_OBJECT

public:
    explicit QDotProgressIndicator(QWidget *parent = nullptr);

    void setTotalSteps(int steps);
    int currentStep() const { return m_currentStep; }
    int totalSteps() const { return m_totalSteps; }

public slots:
    void setCurrentStep(int step);
    void next();
    void previous();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_totalSteps;
    int m_currentStep;
};

#endif // QDOTPROGRESSINDICATOR_H
