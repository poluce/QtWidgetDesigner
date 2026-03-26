#include "demo_visualizer.h"

#include <QApplication>
#include <QCoreApplication>
#include <QLabel>
#include <QPainter>
#include <QPointer>
#include <QPoint>
#include <QRubberBand>
#include <QTest>
#include <QTimer>
#include <QWidget>

namespace
{

    constexpr int kDefaultCursorMoveDurationMs = 700;
    constexpr int kDefaultCursorMoveSteps = 20;
    constexpr int kDefaultMousePressHoldMs = 140;
    constexpr int kDefaultKeyDelayMs = 90;
    constexpr int kDefaultPostActionPauseMs = 180;
    constexpr int kCursorWidth = 26;
    constexpr int kCursorHeight = 34;
    constexpr int kDefaultCursorX = 18;
    constexpr int kDefaultCursorY = 18;
    constexpr double kMinSpeedMultiplier = 0.1;

    struct DemoState
    {
        bool enabled = false;
        double speedMultiplier = 1.0;
        int cursorMoveDurationMs = kDefaultCursorMoveDurationMs;
        int cursorMoveSteps = kDefaultCursorMoveSteps;
        int mousePressHoldMs = kDefaultMousePressHoldMs;
        int keyDelayMs = kDefaultKeyDelayMs;
        int postActionPauseMs = kDefaultPostActionPauseMs;
        QSize cursorSize = QSize(kCursorWidth, kCursorHeight);
        QPointer<QRubberBand> highlightBand;
        QPointer<QLabel> hintLabel;
        QPointer<QLabel> fakeCursorLabel;
        QPointer<QWidget> currentWindow;
        QPoint lastCursorTopLeft = QPoint(kDefaultCursorX, kDefaultCursorY);
    };

    DemoState &state()
    {
        static DemoState instance;
        return instance;
    }

    int scaledDelay(int baseDelayMs)
    {
        const double multiplier = qMax(kMinSpeedMultiplier, state().speedMultiplier);
        return qMax(1, static_cast<int>(baseDelayMs / multiplier));
    }

    QWidget *ensureWindow(QWidget *widget)
    {
        return widget != nullptr ? widget->window() : nullptr;
    }

    void clearOverlay()
    {
        DemoState &s = state();

        if (s.highlightBand != nullptr)
        {
            s.highlightBand->hide();
            s.highlightBand->deleteLater();
            s.highlightBand = nullptr;
        }

        if (s.hintLabel != nullptr)
        {
            s.hintLabel->hide();
            s.hintLabel->deleteLater();
            s.hintLabel = nullptr;
        }

        if (s.fakeCursorLabel != nullptr)
        {
            s.fakeCursorLabel->hide();
        }
    }

    QPixmap buildCursorPixmap(const QSize &size)
    {
        QPixmap pixmap(size);
        pixmap.fill(Qt::transparent);

        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);

        const QPointF p1(4.0, 3.0);
        const QPointF p2(4.0, 27.0);
        const QPointF p3(12.0, 20.0);
        const QPointF p4(16.0, 30.0);
        const QPointF p5(20.0, 28.0);
        const QPointF p6(15.5, 18.5);
        const QPointF p7(24.0, 18.5);

        QPolygonF polygon;
        polygon << p1 << p2 << p3 << p4 << p5 << p6 << p7;

        painter.setPen(QPen(QColor(20, 20, 20, 210), 1.5));
        painter.setBrush(QColor(245, 249, 255, 245));
        painter.drawPolygon(polygon);

        painter.setPen(Qt::NoPen);
        painter.setBrush(QColor(255, 107, 53, 220));
        painter.drawEllipse(QPointF(19.0, 8.0), 4.0, 4.0);

        return pixmap;
    }

    void ensureCursorOverlay(QWidget *window)
    {
        if (window == nullptr)
        {
            return;
        }

        DemoState &s = state();

        if (s.currentWindow != nullptr && s.currentWindow != window)
        {
            if (s.fakeCursorLabel != nullptr)
            {
                s.fakeCursorLabel->hide();
                s.fakeCursorLabel->deleteLater();
                s.fakeCursorLabel = nullptr;
            }
        }

        if (s.fakeCursorLabel == nullptr)
        {
            s.fakeCursorLabel = new QLabel(window);
            s.fakeCursorLabel->setAttribute(Qt::WA_TransparentForMouseEvents, true);
            s.fakeCursorLabel->setPixmap(buildCursorPixmap(s.cursorSize));
            s.fakeCursorLabel->resize(s.cursorSize);
        }

        s.currentWindow = window;
        s.fakeCursorLabel->show();
        s.fakeCursorLabel->raise();
    }

    void ensureOverlay(QWidget *widget, const QString &message)
    {
        QWidget *window = ensureWindow(widget);
        if (window == nullptr)
        {
            return;
        }

        DemoState &s = state();

        ensureCursorOverlay(window);

        if (s.highlightBand == nullptr)
        {
            s.highlightBand = new QRubberBand(QRubberBand::Rectangle, window);
            s.highlightBand->setStyleSheet(QStringLiteral(
                "border: 3px solid #ff6b35; background: rgba(255,107,53,0.12);"));
        }

        const QPoint topLeft = widget->mapTo(window, QPoint(0, 0));
        const QRect rect(topLeft, widget->size());
        s.highlightBand->setGeometry(rect.adjusted(-3, -3, 3, 3));
        s.highlightBand->show();
        s.highlightBand->raise();

        if (s.hintLabel == nullptr)
        {
            s.hintLabel = new QLabel(window);
            s.hintLabel->setStyleSheet(QStringLiteral(
                "background: rgba(20,20,20,0.88); color: white; padding: 6px 10px; "
                "border-radius: 8px; font-weight: 600;"));
        }

        s.hintLabel->setText(message);
        s.hintLabel->adjustSize();
        const int x = qMax(8, rect.left());
        const int y = qMax(8, rect.top() - s.hintLabel->height() - 10);
        s.hintLabel->move(x, y);
        s.hintLabel->show();
        s.hintLabel->raise();

        QCoreApplication::processEvents();
    }

    QPoint targetCursorTopLeft(QWidget *widget, const QPoint &localPoint)
    {
        QWidget *window = ensureWindow(widget);
        if (window == nullptr)
        {
            return QPoint(18, 18);
        }

        const QPoint target = widget->mapTo(window, localPoint);
        return QPoint(target.x() - 6, target.y() - 4);
    }

    void moveCursorSmoothly(QWidget *widget, const QPoint &localPoint)
    {
        QWidget *window = ensureWindow(widget);
        if (window == nullptr)
        {
            return;
        }

        DemoState &s = state();
        ensureCursorOverlay(window);

        const QPoint start = s.lastCursorTopLeft;
        const QPoint finish = targetCursorTopLeft(widget, localPoint);

        for (int step = 1; step <= s.cursorMoveSteps; ++step)
        {
            const qreal ratio = static_cast<qreal>(step) / static_cast<qreal>(s.cursorMoveSteps);
            const int x = start.x() + static_cast<int>((finish.x() - start.x()) * ratio);
            const int y = start.y() + static_cast<int>((finish.y() - start.y()) * ratio);
            s.fakeCursorLabel->move(x, y);
            s.fakeCursorLabel->raise();
            QCoreApplication::processEvents();
            QTest::qWait(qMax(1, scaledDelay(s.cursorMoveDurationMs) / s.cursorMoveSteps));
        }

        s.lastCursorTopLeft = finish;
    }

    void createClickPulse(QWidget *widget, const QPoint &localPoint)
    {
        QWidget *window = ensureWindow(widget);
        if (window == nullptr)
        {
            return;
        }

        auto *pulse = new QWidget(window);
        pulse->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        pulse->setStyleSheet(QStringLiteral(
            "background: rgba(255,107,53,0.22); border: 2px solid rgba(255,107,53,0.95); border-radius: 16px;"));
        pulse->resize(32, 32);

        const QPoint target = widget->mapTo(window, localPoint);
        pulse->move(target.x() - pulse->width() / 2, target.y() - pulse->height() / 2);
        pulse->show();
        pulse->raise();

        QCoreApplication::processEvents();
        QTest::qWait(scaledDelay(70));

        pulse->resize(46, 46);
        pulse->move(target.x() - pulse->width() / 2, target.y() - pulse->height() / 2);
        pulse->setStyleSheet(QStringLiteral(
            "background: rgba(255,107,53,0.12); border: 2px solid rgba(255,107,53,0.55); border-radius: 23px;"));
        QCoreApplication::processEvents();
        QTest::qWait(scaledDelay(80));

        pulse->deleteLater();
    }

    void prepare(QWidget *widget, const QPoint &localPoint, const QString &message)
    {
        DemoState &s = state();
        if (!s.enabled || widget == nullptr)
        {
            return;
        }

        QWidget *window = ensureWindow(widget);
        if (window == nullptr)
        {
            return;
        }

        window->show();
        window->raise();
        window->activateWindow();
        QCoreApplication::processEvents();

        ensureOverlay(widget, message);
        moveCursorSmoothly(widget, localPoint);
        QTest::qWait(scaledDelay(80));
    }

} // namespace

namespace DemoVisualizer
{

    void setEnabled(bool enabled)
    {
        DemoState &s = state();
        s.enabled = enabled;

        if (!enabled)
        {
            clearOverlay();
        }
    }

    bool isEnabled()
    {
        return state().enabled;
    }

    void setSpeedMultiplier(double multiplier)
    {
        state().speedMultiplier = qMax(0.1, multiplier);
    }

    double speedMultiplier()
    {
        return state().speedMultiplier;
    }

    void prepareForClick(QWidget *widget, const QPoint &localPoint)
    {
        prepare(widget, localPoint, QStringLiteral("LLM 准备点击这里"));
    }

    void prepareForTyping(QWidget *widget)
    {
        const QPoint center = widget != nullptr ? widget->rect().center() : QPoint();
        prepare(widget, center, QStringLiteral("LLM 准备在这里输入文字"));
    }

    void finishAction()
    {
        if (!state().enabled)
        {
            return;
        }

        QTest::qWait(scaledDelay(state().postActionPauseMs));
        clearOverlay();
        QCoreApplication::processEvents();
    }

    void showClickPulse(QWidget *widget, const QPoint &localPoint)
    {
        if (!state().enabled)
        {
            return;
        }

        createClickPulse(widget, localPoint);
    }

    int keyDelayMs()
    {
        return scaledDelay(state().keyDelayMs);
    }

    int postActionPauseMs()
    {
        return scaledDelay(state().postActionPauseMs);
    }

    int mousePressHoldMs()
    {
        return scaledDelay(state().mousePressHoldMs);
    }

} // namespace DemoVisualizer
