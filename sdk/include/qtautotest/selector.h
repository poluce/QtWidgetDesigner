#pragma once

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

    Selector& withRef(const QString& ref) { m_ref = ref; m_hasRef = true; return *this; }
    Selector& withPath(const QString& path) { m_path = path; m_hasPath = true; return *this; }
    Selector& withObjectName(const QString& objectName) { m_objectName = objectName; m_hasObjectName = true; return *this; }
    Selector& withClassName(const QString& className) { m_className = className; m_hasClassName = true; return *this; }
    Selector& withTextEquals(const QString& text) { m_textEquals = text; m_hasTextEquals = true; return *this; }
    Selector& withTextContains(const QString& text) { m_textContains = text; m_hasTextContains = true; return *this; }
    Selector& withPlaceholderText(const QString& text) { m_placeholderText = text; m_hasPlaceholderText = true; return *this; }
    Selector& withWindowObjectName(const QString& objectName) { m_windowObjectName = objectName; m_hasWindowObjectName = true; return *this; }
    Selector& withWindowTitleContains(const QString& title) { m_windowTitleContains = title; m_hasWindowTitleContains = true; return *this; }
    Selector& withAncestorRef(const QString& ref) { m_ancestorRef = ref; m_hasAncestorRef = true; return *this; }
    Selector& withAncestorObjectName(const QString& name) { m_ancestorObjectName = name; m_hasAncestorObjectName = true; return *this; }
    Selector& withCurrentPageOnly(bool value = true) { m_currentPageOnly = value; m_hasCurrentPageOnly = true; return *this; }
    Selector& withActiveWindow(bool value = true) { m_activeWindow = value; m_hasActiveWindow = true; return *this; }
    Selector& withVisible(bool value) { m_visible = value; m_hasVisible = true; return *this; }
    Selector& withEnabled(bool value) { m_enabled = value; m_hasEnabled = true; return *this; }
    Selector& withIndexInParent(int index) { m_indexInParent = index; m_hasIndexInParent = true; return *this; }

    bool isEmpty() const
    {
        return !m_hasRef &&
               !m_hasPath &&
               !m_hasObjectName &&
               !m_hasClassName &&
               !m_hasTextEquals &&
               !m_hasTextContains &&
               !m_hasPlaceholderText &&
               !m_hasWindowObjectName &&
               !m_hasWindowTitleContains &&
               !m_hasAncestorRef &&
               !m_hasAncestorObjectName &&
               !m_hasCurrentPageOnly &&
               !m_hasActiveWindow &&
               !m_hasVisible &&
               !m_hasEnabled &&
               !m_hasIndexInParent;
    }

    bool hasRef() const { return m_hasRef; }
    const QString& ref() const { return m_ref; }
    bool hasPath() const { return m_hasPath; }
    const QString& path() const { return m_path; }
    bool hasObjectName() const { return m_hasObjectName; }
    const QString& objectName() const { return m_objectName; }
    bool hasClassName() const { return m_hasClassName; }
    const QString& className() const { return m_className; }
    bool hasTextEquals() const { return m_hasTextEquals; }
    const QString& textEquals() const { return m_textEquals; }
    bool hasTextContains() const { return m_hasTextContains; }
    const QString& textContains() const { return m_textContains; }
    bool hasPlaceholderText() const { return m_hasPlaceholderText; }
    const QString& placeholderText() const { return m_placeholderText; }
    bool hasWindowObjectName() const { return m_hasWindowObjectName; }
    const QString& windowObjectName() const { return m_windowObjectName; }
    bool hasWindowTitleContains() const { return m_hasWindowTitleContains; }
    const QString& windowTitleContains() const { return m_windowTitleContains; }
    bool hasAncestorRef() const { return m_hasAncestorRef; }
    const QString& ancestorRef() const { return m_ancestorRef; }
    bool hasAncestorObjectName() const { return m_hasAncestorObjectName; }
    const QString& ancestorObjectName() const { return m_ancestorObjectName; }
    bool hasCurrentPageOnly() const { return m_hasCurrentPageOnly; }
    bool currentPageOnly() const { return m_currentPageOnly; }
    bool hasActiveWindow() const { return m_hasActiveWindow; }
    bool activeWindow() const { return m_activeWindow; }
    bool hasVisible() const { return m_hasVisible; }
    bool visible() const { return m_visible; }
    bool hasEnabled() const { return m_hasEnabled; }
    bool enabled() const { return m_enabled; }
    bool hasIndexInParent() const { return m_hasIndexInParent; }
    int indexInParent() const { return m_indexInParent; }

private:
    QString m_ref;
    QString m_path;
    QString m_objectName;
    QString m_className;
    QString m_textEquals;
    QString m_textContains;
    QString m_placeholderText;
    QString m_windowObjectName;
    QString m_windowTitleContains;
    QString m_ancestorRef;
    QString m_ancestorObjectName;
    bool m_currentPageOnly = false;
    bool m_activeWindow = false;
    bool m_visible = false;
    bool m_enabled = false;
    int m_indexInParent = -1;

    bool m_hasRef = false;
    bool m_hasPath = false;
    bool m_hasObjectName = false;
    bool m_hasClassName = false;
    bool m_hasTextEquals = false;
    bool m_hasTextContains = false;
    bool m_hasPlaceholderText = false;
    bool m_hasWindowObjectName = false;
    bool m_hasWindowTitleContains = false;
    bool m_hasAncestorRef = false;
    bool m_hasAncestorObjectName = false;
    bool m_hasCurrentPageOnly = false;
    bool m_hasActiveWindow = false;
    bool m_hasVisible = false;
    bool m_hasEnabled = false;
    bool m_hasIndexInParent = false;
};

} // namespace qtautotest
