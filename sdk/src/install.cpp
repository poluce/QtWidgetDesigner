#include <qtautotest/install.h>

#include <qtautotest/runtime.h>

#include <QCoreApplication>
#include <QObject>
#include <QPointer>

namespace qtautotest {

namespace {

class InstalledRuntimeHolder : public QObject
{
public:
    explicit InstalledRuntimeHolder(QObject* parent = nullptr)
        : QObject(parent)
    {
    }

    ~InstalledRuntimeHolder() override
    {
        runtime.stop();
    }

    Runtime runtime;
    InstallOptions options;
    QString error;
};

QPointer<InstalledRuntimeHolder>& installedHolder()
{
    static QPointer<InstalledRuntimeHolder> holder;
    return holder;
}

QString& lastInstallError()
{
    static QString error;
    return error;
}

} // namespace

bool install(QCoreApplication& app, const InstallOptions& options)
{
    if (installedHolder() != nullptr && installedHolder()->runtime.isRunning() &&
        installedHolder()->options.port == options.port) {
        lastInstallError().clear();
        return true;
    }

    uninstall();

    auto* holder = new InstalledRuntimeHolder(&app);
    holder->options = options;

    RuntimeOptions runtimeOptions;
    runtimeOptions.port = options.port;

    if (!holder->runtime.start(runtimeOptions)) {
        holder->error = holder->runtime.errorString();
        lastInstallError() = holder->error;
        holder->deleteLater();
        return false;
    }

    QObject::connect(&app, &QCoreApplication::aboutToQuit, holder, []() {
        uninstall();
    });

    installedHolder() = holder;
    lastInstallError().clear();
    return true;
}

bool install(const InstallOptions& options)
{
    QCoreApplication* app = QCoreApplication::instance();
    if (app == nullptr) {
        lastInstallError() = QStringLiteral("A QCoreApplication instance must exist before install().");
        return false;
    }

    return install(*app, options);
}

void uninstall()
{
    if (installedHolder() == nullptr) {
        return;
    }

    InstalledRuntimeHolder* holder = installedHolder().data();
    installedHolder().clear();
    delete holder;
}

bool isInstalled()
{
    return installedHolder() != nullptr && installedHolder()->runtime.isRunning();
}

Runtime* installedRuntime()
{
    return installedHolder() != nullptr ? &installedHolder()->runtime : nullptr;
}

QString installErrorString()
{
    if (installedHolder() != nullptr && !installedHolder()->error.isEmpty()) {
        return installedHolder()->error;
    }
    return lastInstallError();
}

} // namespace qtautotest
