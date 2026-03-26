#pragma once

#include <QString>

class QPoint;
class QWidget;

namespace DemoVisualizer {

void setEnabled(bool enabled);
bool isEnabled();
void setSpeedMultiplier(double multiplier);
double speedMultiplier();

void prepareForClick(QWidget* widget, const QPoint& localPoint);
void prepareForTyping(QWidget* widget);
void finishAction();
void showClickPulse(QWidget* widget, const QPoint& localPoint);

int keyDelayMs();
int postActionPauseMs();
int mousePressHoldMs();

} // namespace DemoVisualizer
