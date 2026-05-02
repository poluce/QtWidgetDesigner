#include <qtautotest/qtautotest.h>

#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QVBoxLayout>
#include <QWidget>

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    if (!qtautotest::install(app)) {
        qCritical() << "QtAutoTest install failed:" << qtautotest::installErrorString();
        return 2;
    }

    QWidget window;
    window.setWindowTitle(QStringLiteral("__PROJECT_NAME__"));

    auto* layout = new QVBoxLayout(&window);
    layout->addWidget(new QLabel(QStringLiteral("QtAutoTest minimal template is running."), &window));

    window.resize(420, 160);
    window.show();

    return app.exec();
}
