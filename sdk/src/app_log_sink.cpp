#include "app_log_sink.h"

#include <QDateTime>
#include <QJsonObject>
#include <QMutexLocker>

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
    const int safeLimit = limit <= 0 ? m_entries.size() : qMin(limit, m_entries.size());
    const int startIndex = qMax(0, m_entries.size() - safeLimit);

    for (int i = startIndex; i < m_entries.size(); ++i) {
        const LogEntry& entry = m_entries.at(i);
        result.append(QJsonObject{
            {"timestampMs", QString::number(entry.timestampMs)},
            {"level", entry.level},
            {"category", entry.category},
            {"message", entry.message},
        });
    }

    return result;
}

void AppLogSink::append(QtMsgType type, const QMessageLogContext& context, const QString& message)
{
    QMutexLocker locker(&m_mutex);

    if (m_entries.size() >= m_capacity) {
        m_entries.remove(0, m_entries.size() - m_capacity + 1);
    }

    m_entries.push_back(LogEntry{
        QDateTime::currentMSecsSinceEpoch(),
        levelForType(type),
        QString::fromUtf8(context.category ? context.category : "default"),
        message,
    });
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
