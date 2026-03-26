#include <qtautotest/harness.h>

#include <QElapsedTimer>
#include <QProcess>
#include <QProcessEnvironment>
#include <QThread>

namespace qtautotest {

class ProcessHarness::Impl
{
public:
    HarnessOptions options;
    QProcess process;
    QString error;
};

ProcessHarness::ProcessHarness()
    : m_impl(new Impl())
{
}

ProcessHarness::~ProcessHarness()
{
    stop();
    delete m_impl;
}

bool ProcessHarness::start(const HarnessOptions& options)
{
    stop();

    m_impl->options = options;
    m_impl->error.clear();

    m_impl->process.setProgram(options.program);
    m_impl->process.setArguments(options.arguments);
    if (!options.workingDirectory.isEmpty()) {
        m_impl->process.setWorkingDirectory(options.workingDirectory);
    }
    m_impl->process.setProcessEnvironment(options.environment);
    m_impl->process.start();

    if (!m_impl->process.waitForStarted(options.startupTimeoutMs)) {
        m_impl->error = m_impl->process.errorString();
        return false;
    }

    return true;
}

bool ProcessHarness::restart()
{
    const HarnessOptions options = m_impl->options;
    if (options.program.isEmpty()) {
        m_impl->error = QStringLiteral("No harness options have been configured.");
        return false;
    }

    return start(options);
}

void ProcessHarness::stop()
{
    if (m_impl->process.state() == QProcess::NotRunning) {
        return;
    }

    m_impl->process.terminate();
    if (!m_impl->process.waitForFinished(3000)) {
        m_impl->process.kill();
        m_impl->process.waitForFinished(3000);
    }
}

bool ProcessHarness::isRunning() const
{
    return m_impl->process.state() != QProcess::NotRunning;
}

qint64 ProcessHarness::processId() const
{
    return m_impl->process.processId();
}

bool ProcessHarness::waitUntilReady(int timeoutMs)
{
    const int effectiveTimeoutMs = timeoutMs > 0 ? timeoutMs : m_impl->options.startupTimeoutMs;
    QElapsedTimer timer;
    timer.start();

    while (timer.elapsed() <= effectiveTimeoutMs) {
        const BridgeCallResult result = bridgeClient().call(QStringLiteral("ping"), QJsonObject{}, 1000);
        if (result.transportOk && result.response.value(QStringLiteral("ok")).toBool()) {
            return true;
        }

        if (m_impl->process.state() == QProcess::NotRunning) {
            m_impl->error = QStringLiteral("Target process exited before the bridge became ready.");
            return false;
        }

        QThread::msleep(static_cast<unsigned long>(m_impl->options.pollIntervalMs > 0 ? m_impl->options.pollIntervalMs : 100));
    }

    m_impl->error = QStringLiteral("Timed out waiting for the bridge to become ready.");
    return false;
}

QString ProcessHarness::errorString() const
{
    return m_impl->error;
}

QUrl ProcessHarness::bridgeUrl() const
{
    return m_impl->options.bridgeUrl;
}

BridgeClient ProcessHarness::bridgeClient() const
{
    return BridgeClient(m_impl->options.bridgeUrl);
}

} // namespace qtautotest
