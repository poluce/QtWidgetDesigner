#pragma once

#include <QString>

namespace DemoVisualizer {

void install();
void uninstall();
void setEnabled(bool enabled);
bool isEnabled();
void setSpeedMultiplier(double multiplier);
double speedMultiplier();

} // namespace DemoVisualizer
