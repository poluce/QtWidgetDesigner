#pragma once

#include <qtautotest/harness.h>

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

namespace qtautotest {

struct CommandSpec
{
    QString program;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    int timeoutMs = 300000;
};

struct CommandResult
{
    bool ok = false;
    int exitCode = -1;
    QString errorString;
    QString standardOutput;
    QString standardError;
};

struct BuildHarnessOptions
{
    CommandSpec configure;
    CommandSpec build;
    HarnessOptions run;
};

class BuildHarness
{
public:
    BuildHarness();
    ~BuildHarness();

    bool run(const BuildHarnessOptions& options);
    bool runConfigure();
    bool runBuild();
    bool startApp();
    bool waitUntilReady(int timeoutMs = -1);
    void stop();

    bool isRunning() const;
    QString errorString() const;
    QUrl bridgeUrl() const;
    AutomationClient automationClient() const;

    const CommandResult& configureResult() const;
    const CommandResult& buildResult() const;
    const HarnessOptions& runOptions() const;

private:
    class Impl;
    Impl* m_impl = nullptr;
};

} // namespace qtautotest
