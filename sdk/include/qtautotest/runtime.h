#pragma once

#include <QString>
#include <QtGlobal>

namespace qtautotest {

struct RuntimeOptions
{
    quint16 port = 49555;
};

class Runtime
{
public:
    Runtime();
    ~Runtime();

    bool start(const RuntimeOptions& options = RuntimeOptions{});
    void stop();

    bool isRunning() const;
    quint16 port() const;
    QString errorString() const;

private:
    class Impl;
    Impl* m_impl = nullptr;
};

} // namespace qtautotest
