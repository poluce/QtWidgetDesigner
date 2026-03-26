#pragma once

#include <qtautotest/bridge_client.h>

#include <QProcessEnvironment>
#include <QUrl>

namespace qtautotest {

struct HarnessOptions
{
    QString program;
    QStringList arguments;
    QString workingDirectory;
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    QUrl bridgeUrl = QUrl(QStringLiteral("ws://127.0.0.1:49555"));
    int startupTimeoutMs = 10000;
    int pollIntervalMs = 100;
};

class ProcessHarness
{
public:
    ProcessHarness();
    ~ProcessHarness();

    bool start(const HarnessOptions& options);
    bool restart();
    void stop();

    bool isRunning() const;
    qint64 processId() const;
    bool waitUntilReady(int timeoutMs = -1);

    QString errorString() const;
    QUrl bridgeUrl() const;
    BridgeClient bridgeClient() const;

private:
    class Impl;
    Impl* m_impl = nullptr;
};

} // namespace qtautotest
