#include <qtautotest/build_harness.h>

#include <QProcess>

namespace qtautotest {

class BuildHarness::Impl
{
public:
    BuildHarnessOptions options;
    CommandResult configureResult;
    CommandResult buildResult;
    QString error;
    ProcessHarness processHarness;
};

namespace {

CommandResult runCommand(const CommandSpec& command)
{
    CommandResult result;

    if (command.program.isEmpty()) {
        result.ok = true;
        return result;
    }

    QProcess process;
    process.setProgram(command.program);
    process.setArguments(command.arguments);
    if (!command.workingDirectory.isEmpty()) {
        process.setWorkingDirectory(command.workingDirectory);
    }
    process.setProcessEnvironment(command.environment);
    process.start();

    if (!process.waitForStarted(command.timeoutMs)) {
        result.errorString = process.errorString();
        result.standardOutput = QString::fromLocal8Bit(process.readAllStandardOutput());
        result.standardError = QString::fromLocal8Bit(process.readAllStandardError());
        return result;
    }

    if (!process.waitForFinished(command.timeoutMs)) {
        process.kill();
        process.waitForFinished(3000);
        result.errorString = QStringLiteral("Command timed out.");
        result.standardOutput = QString::fromLocal8Bit(process.readAllStandardOutput());
        result.standardError = QString::fromLocal8Bit(process.readAllStandardError());
        return result;
    }

    result.exitCode = process.exitCode();
    result.standardOutput = QString::fromLocal8Bit(process.readAllStandardOutput());
    result.standardError = QString::fromLocal8Bit(process.readAllStandardError());
    result.ok = process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
    if (!result.ok && result.errorString.isEmpty()) {
        result.errorString = process.errorString();
    }

    return result;
}

} // namespace

BuildHarness::BuildHarness()
    : m_impl(new Impl())
{
}

BuildHarness::~BuildHarness()
{
    stop();
    delete m_impl;
}

bool BuildHarness::run(const BuildHarnessOptions& options)
{
    stop();
    m_impl->options = options;
    m_impl->error.clear();

    if (!runConfigure()) {
        return false;
    }
    if (!runBuild()) {
        return false;
    }
    return startApp();
}

bool BuildHarness::runConfigure()
{
    m_impl->configureResult = runCommand(m_impl->options.configure);
    if (!m_impl->configureResult.ok) {
        m_impl->error = !m_impl->configureResult.errorString.isEmpty()
            ? m_impl->configureResult.errorString
            : QStringLiteral("Configure step failed.");
        return false;
    }
    return true;
}

bool BuildHarness::runBuild()
{
    m_impl->buildResult = runCommand(m_impl->options.build);
    if (!m_impl->buildResult.ok) {
        m_impl->error = !m_impl->buildResult.errorString.isEmpty()
            ? m_impl->buildResult.errorString
            : QStringLiteral("Build step failed.");
        return false;
    }
    return true;
}

bool BuildHarness::startApp()
{
    if (!m_impl->processHarness.start(m_impl->options.run)) {
        m_impl->error = m_impl->processHarness.errorString();
        return false;
    }
    return true;
}

bool BuildHarness::waitUntilReady(int timeoutMs)
{
    if (!m_impl->processHarness.waitUntilReady(timeoutMs)) {
        m_impl->error = m_impl->processHarness.errorString();
        return false;
    }
    return true;
}

void BuildHarness::stop()
{
    m_impl->processHarness.stop();
}

bool BuildHarness::isRunning() const
{
    return m_impl->processHarness.isRunning();
}

QString BuildHarness::errorString() const
{
    return m_impl->error;
}

QUrl BuildHarness::bridgeUrl() const
{
    return m_impl->processHarness.bridgeUrl();
}

AutomationClient BuildHarness::automationClient() const
{
    return m_impl->processHarness.automationClient();
}

const CommandResult& BuildHarness::configureResult() const
{
    return m_impl->configureResult;
}

const CommandResult& BuildHarness::buildResult() const
{
    return m_impl->buildResult;
}

const HarnessOptions& BuildHarness::runOptions() const
{
    return m_impl->options.run;
}

} // namespace qtautotest
