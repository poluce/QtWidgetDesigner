#pragma once

#include <QString>
#include <QtGlobal>

class QCoreApplication;

namespace qtautotest {

class Runtime;

struct InstallOptions
{
    quint16 port = 49555;
};

bool install(QCoreApplication& app, const InstallOptions& options = InstallOptions{});
bool install(const InstallOptions& options = InstallOptions{});
void uninstall();

bool isInstalled();
Runtime* installedRuntime();
QString installErrorString();

} // namespace qtautotest
