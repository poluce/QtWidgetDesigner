#include <qtautotest/runtime.h>

#include "agent_bridge_server.h"
#include "app_log_sink.h"
#include "demo_visualizer.h"

namespace qtautotest {

class Runtime::Impl
{
public:
    RuntimeOptions options;
    AgentBridgeServer* bridge = nullptr;
    QString error;
};

Runtime::Runtime()
    : m_impl(new Impl())
{
}

Runtime::~Runtime()
{
    stop();
    delete m_impl;
}

bool Runtime::start(const RuntimeOptions& options)
{
    stop();

    m_impl->options = options;
    m_impl->error.clear();

    AppLogSink::install();
    DemoVisualizer::setEnabled(options.enableVisibleDemo);
    DemoVisualizer::setSpeedMultiplier(options.demoSpeed);

    m_impl->bridge = new AgentBridgeServer();
    if (!m_impl->bridge->start(options.port)) {
        m_impl->error = m_impl->bridge->errorString();
        delete m_impl->bridge;
        m_impl->bridge = nullptr;
        return false;
    }

    return true;
}

void Runtime::stop()
{
    if (m_impl->bridge != nullptr) {
        delete m_impl->bridge;
        m_impl->bridge = nullptr;
    }
}

bool Runtime::isRunning() const
{
    return m_impl->bridge != nullptr;
}

quint16 Runtime::port() const
{
    return m_impl->bridge != nullptr ? m_impl->bridge->port() : 0;
}

QString Runtime::errorString() const
{
    return m_impl->error;
}

} // namespace qtautotest
