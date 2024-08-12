#ifndef PAGECONTROLLER_H
#define PAGECONTROLLER_H

#include <QObject>

class PageController : public QObject
{
    Q_OBJECT

public:
    explicit PageController(QObject *parent = nullptr);
    virtual ~PageController() {}

    virtual void moveToNextPage() = 0;
    virtual void moveToPreviousPage() = 0;
};

#endif // PAGECONTROLLER_H
