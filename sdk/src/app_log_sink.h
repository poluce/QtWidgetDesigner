#pragma once

#include <QJsonArray>
#include <QLoggingCategory>
#include <QMutex>
#include <QString>
#include <QVector>

class AppLogSink
{
public:
    static AppLogSink& instance();

    static void install();

    QJsonArray recentEntries(int limit) const;

private:
    struct LogEntry
    {
        qint64 timestampMs = 0;
        QString level;
        QString category;
        QString message;
    };

    AppLogSink() = default;

    void append(QtMsgType type, const QMessageLogContext& context, const QString& message);
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);
    static QString levelForType(QtMsgType type);

    mutable QMutex m_mutex;
    QVector<LogEntry> m_entries;
    QtMessageHandler m_previousHandler = nullptr;
    const int m_capacity = 2000;
};
