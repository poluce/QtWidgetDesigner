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

    AppLogSink();

    void append(QtMsgType type, const QMessageLogContext& context, const QString& message);
    static void messageHandler(QtMsgType type, const QMessageLogContext& context, const QString& message);
    static QString levelForType(QtMsgType type);

    mutable QMutex m_mutex;

    // 固定大小环形缓冲，避免 QVector::remove(0) 的 O(n) 元素搬移
    static constexpr int kCapacity = 2000;
    QVector<LogEntry> m_entries;
    int m_head = 0;   // 最老元素的逻辑索引
    int m_count = 0;  // 当前有效元素个数

    QtMessageHandler m_previousHandler = nullptr;
};
