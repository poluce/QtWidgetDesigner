#include "main_window.h"
#include "demo_visualizer.h"

#include <qtautotest/runtime.h>

#include <QApplication>
#include <QCommandLineOption>
#include <QCommandLineParser>
#include <QCoreApplication>
#include <QDebug>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("QtAutoTestDemo"));
    QCoreApplication::setApplicationVersion(QStringLiteral("0.1.0"));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Qt Widgets sandbox with an LLM automation bridge."));
    parser.addHelpOption();
    parser.addVersionOption();

    QCommandLineOption portOption(QStringList{QStringLiteral("p"), QStringLiteral("port")},
                                  QStringLiteral("Agent bridge port."),
                                  QStringLiteral("port"),
                                  QStringLiteral("49555"));
    parser.addOption(portOption);

    QCommandLineOption demoVisibleOption(
        QStringList{QStringLiteral("demo-visible")},
        QStringLiteral("Enable a user-visible demo mode with slow cursor movement, highlight, and slow typing."));
    parser.addOption(demoVisibleOption);

    QCommandLineOption demoSpeedOption(
        QStringList{QStringLiteral("demo-speed")},
        QStringLiteral("Demo speed multiplier. Examples: 0.5 (slower), 1 (default), 2 (faster)."),
        QStringLiteral("multiplier"),
        QStringLiteral("1"));
    parser.addOption(demoSpeedOption);

    parser.process(app);

    bool ok = false;
    const int port = parser.value(portOption).toInt(&ok);
    if (!ok || port <= 0 || port > 65535) {
        qCritical() << "无效端口:" << parser.value(portOption);
        return 1;
    }

    bool speedOk = false;
    const double demoSpeed = parser.value(demoSpeedOption).toDouble(&speedOk);
    if (!speedOk || demoSpeed <= 0.0) {
        qCritical() << "无效演示速度:" << parser.value(demoSpeedOption);
        return 1;
    }

    qtautotest::Runtime runtime;
    qtautotest::RuntimeOptions runtimeOptions;
    runtimeOptions.port = static_cast<quint16>(port);

    if (!runtime.start(runtimeOptions)) {
        qCritical() << "无法在端口上启动桥接服务" << port << ":" << runtime.errorString();
        return 2;
    }

    DemoVisualizer::install();
    DemoVisualizer::setEnabled(parser.isSet(demoVisibleOption));
    DemoVisualizer::setSpeedMultiplier(demoSpeed);

    MainWindow window(runtime.port());
    window.show();

    qInfo() << "桥接服务监听地址 ws://127.0.0.1:" + QString::number(runtime.port());
    qInfo() << "应用已启动。";

    return app.exec();
}
