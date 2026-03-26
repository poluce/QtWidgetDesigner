#include <qtautotest/qtautotest.h>

#include <QApplication>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    qtautotest::Runtime runtime;
    qtautotest::RuntimeOptions options;
    options.port = 49600;
    options.enableVisibleDemo = false;
    options.demoSpeed = 1.0;

    if (!runtime.start(options)) {
        return 2;
    }

    QWidget window;
    window.setWindowTitle(QStringLiteral("QtAutoTest SDK 最小接入示例"));

    auto* layout = new QVBoxLayout(&window);
    auto* label = new QLabel(QStringLiteral("这个窗口只通过 qtautotest::Runtime 接入了自动化桥接。"), &window);
    layout->addWidget(label);

    window.resize(420, 160);
    window.show();

    return app.exec();
}
