#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

namespace qtautotest {

class SnapshotNode
{
public:
    SnapshotNode() = default;
    explicit SnapshotNode(QJsonObject object) : m_object(std::move(object)) {}

    bool isValid() const { return !m_object.isEmpty(); }
    QString ref() const { return m_object.value(QStringLiteral("ref")).toString(); }
    QString objectName() const { return m_object.value(QStringLiteral("objectName")).toString(); }
    QString className() const { return m_object.value(QStringLiteral("className")).toString(); }
    QString text() const { return m_object.value(QStringLiteral("text")).toString(); }
    QString path() const { return m_object.value(QStringLiteral("path")).toString(); }
    bool visible() const { return m_object.value(QStringLiteral("visible")).toBool(); }
    bool enabled() const { return m_object.value(QStringLiteral("enabled")).toBool(); }
    bool isInteractive() const { return m_object.value(QStringLiteral("isInteractive")).toBool(); }
    QString riskLevel() const { return m_object.value(QStringLiteral("riskLevel")).toString(); }
    const QJsonObject& toJson() const { return m_object; }

    QVector<SnapshotNode> children() const
    {
        QVector<SnapshotNode> nodes;
        const QJsonArray array = m_object.value(QStringLiteral("children")).toArray();
        nodes.reserve(array.size());
        for (const QJsonValue& value : array) {
            nodes.append(SnapshotNode(value.toObject()));
        }
        return nodes;
    }

private:
    QJsonObject m_object;
};

class SnapshotView
{
public:
    SnapshotView() = default;
    explicit SnapshotView(QJsonObject object) : m_object(std::move(object)) {}

    SnapshotNode activeWindow() const { return SnapshotNode(m_object.value(QStringLiteral("activeWindow")).toObject()); }
    SnapshotNode focusWidget() const { return SnapshotNode(m_object.value(QStringLiteral("focusWidget")).toObject()); }
    SnapshotNode activePage() const { return SnapshotNode(m_object.value(QStringLiteral("activePage")).toObject()); }
    SnapshotNode modalDialog() const { return SnapshotNode(m_object.value(QStringLiteral("modalDialog")).toObject()); }

    QVector<SnapshotNode> windows() const
    {
        QVector<SnapshotNode> nodes;
        const QJsonArray array = m_object.value(QStringLiteral("windows")).toArray();
        nodes.reserve(array.size());
        for (const QJsonValue& value : array) {
            nodes.append(SnapshotNode(value.toObject()));
        }
        return nodes;
    }

    const QJsonObject& toJson() const { return m_object; }

private:
    QJsonObject m_object;
};

} // namespace qtautotest
