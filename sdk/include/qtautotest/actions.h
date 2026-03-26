#pragma once

#include <qtautotest/selector.h>

#include <QJsonArray>
#include <QJsonObject>
#include <QString>

namespace qtautotest {

inline QJsonObject makeClickArguments(const Selector& selector)
{
    return QJsonObject{{QStringLiteral("selector"), selector.toJson()}};
}

inline QJsonObject makeSetTextArguments(const Selector& selector, const QString& text)
{
    return QJsonObject{
        {QStringLiteral("selector"), selector.toJson()},
        {QStringLiteral("text"), text},
    };
}

inline QJsonObject makeAssertWidgetArguments(const Selector& selector, const QJsonObject& assertions)
{
    return QJsonObject{
        {QStringLiteral("selector"), selector.toJson()},
        {QStringLiteral("assertions"), assertions},
    };
}

inline QJsonObject makeWaitForWidgetArguments(const Selector& selector, const QJsonObject& assertions,
                                              int timeoutMs = 3000, int pollIntervalMs = 100)
{
    return QJsonObject{
        {QStringLiteral("selector"), selector.toJson()},
        {QStringLiteral("assertions"), assertions},
        {QStringLiteral("timeoutMs"), timeoutMs},
        {QStringLiteral("pollIntervalMs"), pollIntervalMs},
    };
}

inline QJsonObject makePressKeyArguments(const Selector& selector, const QString& key, const QString& modifiers = QString())
{
    return QJsonObject{
        {QStringLiteral("selector"), selector.toJson()},
        {QStringLiteral("key"), key},
        {QStringLiteral("modifiers"), modifiers},
    };
}

inline QJsonObject makeShortcutArguments(const Selector& selector, const QString& shortcut)
{
    return QJsonObject{
        {QStringLiteral("selector"), selector.toJson()},
        {QStringLiteral("shortcut"), shortcut},
    };
}

inline QJsonObject makeScrollArguments(const Selector& selector, const QString& direction, int amount = 120)
{
    return QJsonObject{
        {QStringLiteral("selector"), selector.toJson()},
        {QStringLiteral("direction"), direction},
        {QStringLiteral("amount"), amount},
    };
}

inline QJsonObject makeSelectItemArguments(const Selector& selector, const QJsonObject& options)
{
    return QJsonObject{
        {QStringLiteral("selector"), selector.toJson()},
        {QStringLiteral("options"), options},
    };
}

inline QJsonObject makeTreePathArguments(const Selector& selector, const QJsonArray& path)
{
    return QJsonObject{
        {QStringLiteral("selector"), selector.toJson()},
        {QStringLiteral("path"), path},
    };
}

} // namespace qtautotest
