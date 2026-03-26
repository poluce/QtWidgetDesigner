#pragma once

#include <QJsonObject>
#include <QString>

namespace qtautotest {

class Selector
{
public:
    Selector() = default;

    static Selector byRef(const QString& ref)
    {
        Selector selector;
        selector.withRef(ref);
        return selector;
    }

    static Selector byObjectName(const QString& objectName)
    {
        Selector selector;
        selector.withObjectName(objectName);
        return selector;
    }

    Selector& withRef(const QString& ref) { m_data.insert(QStringLiteral("ref"), ref); return *this; }
    Selector& withPath(const QString& path) { m_data.insert(QStringLiteral("path"), path); return *this; }
    Selector& withObjectName(const QString& objectName) { m_data.insert(QStringLiteral("objectName"), objectName); return *this; }
    Selector& withClassName(const QString& className) { m_data.insert(QStringLiteral("className"), className); return *this; }
    Selector& withTextEquals(const QString& text) { m_data.insert(QStringLiteral("textEquals"), text); return *this; }
    Selector& withTextContains(const QString& text) { m_data.insert(QStringLiteral("textContains"), text); return *this; }
    Selector& withPlaceholderText(const QString& text) { m_data.insert(QStringLiteral("placeholderText"), text); return *this; }
    Selector& withWindowObjectName(const QString& objectName) { m_data.insert(QStringLiteral("windowObjectName"), objectName); return *this; }
    Selector& withWindowTitleContains(const QString& title) { m_data.insert(QStringLiteral("windowTitleContains"), title); return *this; }
    Selector& withAncestorRef(const QString& ref) { m_data.insert(QStringLiteral("ancestorRef"), ref); return *this; }
    Selector& withAncestorObjectName(const QString& name) { m_data.insert(QStringLiteral("ancestorObjectName"), name); return *this; }
    Selector& withCurrentPageOnly(bool value = true) { m_data.insert(QStringLiteral("currentPageOnly"), value); return *this; }
    Selector& withActiveWindow(bool value = true) { m_data.insert(QStringLiteral("isActiveWindow"), value); return *this; }
    Selector& withVisible(bool value) { m_data.insert(QStringLiteral("visible"), value); return *this; }
    Selector& withEnabled(bool value) { m_data.insert(QStringLiteral("enabled"), value); return *this; }
    Selector& withIndexInParent(int index) { m_data.insert(QStringLiteral("indexInParent"), index); return *this; }

    const QJsonObject& toJson() const { return m_data; }

private:
    QJsonObject m_data;
};

} // namespace qtautotest
