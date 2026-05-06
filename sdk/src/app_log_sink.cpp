#include "app_log_sink.h"

#include <QDateTime>
#include <QJsonObject>
#include <QMutexLocker>

AppLogSink::AppLogSink()
    : m_entries(kCapacity)
{
}

AppLogSink& AppLogSink::instance()
{
    static AppLogSink sink;
    return sink;
}

void AppLogSink::install()
{
    AppLogSink& sink = instance();
    if (sink.m_previousHandler == nullptr) {
        sink.m_previousHandler = qInstallMessageHandler(&AppLogSink::messageHandler);
    }
}

QJsonArray AppLogSink::recentEntries(int limit) const
{
    QJsonArray result;

    QMutexLocker locker(&m_mutex);

    // 确定要返回多少条
    const int available = m_count;
    const int safeLimit = limit <= 0 ? available : qMin(limit, available);
    if (safeLimit <= 0) {
        return result;
    }

    // 从 (最老 + 跳过前部) 的位置开始遍历
    const int skip = qMax(0, available - safeLimit);
    int idx = (m_head + skip) % kCapacity;

    result.reserve(safeLimit);
    for (int i = 0; i < safeLimit; ++i) {
        const LogEntry& entry = m_entries[idx];
        result.append(QJsonObject{
            {"timestampMs", QString::number(entry.timestampMs)},
            {"level", entry.level},
            {"category", entry.category},
            {"message", entry.message},
        });
        idx = (idx + 1) % kCapacity;
    }

    return result;
}

void AppLogSink::append(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    QMutexLocker locker(&m_mutex);

    // 新的日志写入 tail 位置
    const int tail = (m_head + m_count) % kCapacity;
    m_entries[tail] = LogEntry{
        QDateTime::currentMSecsSinceEpoch(),
        levelForType(type),
        QString::fromUtf8(context.category ? context.category : "default"),
        message,
    };

    if (m_count < kCapacity) {
        ++m_count;
    } else {
        // 缓冲已满，覆盖最老元素，头指针前进
        m_head = (m_head + 1) % kCapacity;
    }
}

void AppLogSink::messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    AppLogSink& sink = instance();
    sink.append(type, context, message);

    if (sink.m_previousHandler != nullptr) {
        sink.m_previousHandler(type, context, message);
    }
}

QString AppLogSink::levelForType(QtMsgType type)
{
    switch (type) {
    case QtDebugMsg:
        return QStringLiteral("调试");
    case QtInfoMsg:
        return QStringLiteral("信息");
    case QtWarningMsg:
        return QStringLiteral("警告");
    case QtCriticalMsg:
        return QStringLiteral("错误");
    case QtFatalMsg:
        return QStringLiteral("致命");
    }

    return QStringLiteral("未知");
}
