#pragma once

#include <QPoint>

class QWidget;

namespace qtautotest {

class ActionObserver
{
public:
    virtual ~ActionObserver() = default;

    virtual bool isEnabled() const = 0;
    virtual void prepareForClick(QWidget* widget, const QPoint& localPoint) = 0;
    virtual void prepareForTyping(QWidget* widget) = 0;
    virtual void showClickPulse(QWidget* widget, const QPoint& localPoint) = 0;
    virtual void finishAction() = 0;
    virtual int keyDelayMs() const = 0;
    virtual int mousePressHoldMs() const = 0;
};

void setActionObserver(ActionObserver* observer);
ActionObserver* actionObserver();

} // namespace qtautotest
