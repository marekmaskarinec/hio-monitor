#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QSortFilterProxyModel>
#include <QIcon>
#include <QLockFile>
#include <QDir>

#include "app_environment.h"
#include "import_qml_components_plugins.h"
#include "import_qml_plugins.h"
#include "searchcomponent.h"
#include "chester.h"
#include "bluetooth.h"
#include "messagemodel.h"
#include "devicemodel.h"
#include "filehandler.h"

static void initBackend() {
    SearchComponent::registerQmlType();
    MessageModel::registerQmlType();
}

int main(int argc, char *argv[]) 
{
    set_qt_environment();
    QGuiApplication app(argc, argv);

    QLockFile lockFile(QDir::temp().absoluteFilePath("HARDWARIOMonitor.lock"));
    if (!lockFile.tryLock(100)) {
        // An instance of the application is already running
        return 1;
    }

    app.setOrganizationName("HARDWARIO");
    app.setOrganizationDomain("IoT");
    app.setWindowIcon(QIcon(":/resources/icons/hwMainIcon.svg"));
    initBackend();

    QQmlApplicationEngine engine;

//    MessageModel messageModel;
//    engine.rootContext()->setContextProperty("messageModel", &messageModel);

    DeviceModel deviceModel;
    QSortFilterProxyModel proxyModel;
    proxyModel.setSourceModel(&deviceModel);
    proxyModel.setSortRole(DeviceModel::SortRole);
    proxyModel.sort(0, Qt::DescendingOrder);
    engine.rootContext()->setContextProperty("deviceModel", &deviceModel);
    engine.rootContext()->setContextProperty("sortDeviceModel", &proxyModel);

    auto commandHistoryFile = new FileHandler("hardwario-monitor-command-history.txt");
    // TODO: connect sendCommandSucceeded signals intead of passing the file as an asrgument
    const auto chester = new Chester(&engine, commandHistoryFile);
    const auto bluetooth = new Bluetooth(&engine, &proxyModel, commandHistoryFile);

    engine.rootContext()->setContextProperty("chester", chester);
    engine.rootContext()->setContextProperty("bluetooth", bluetooth);

    const QUrl url(u"qrc:Main/main.qml"_qs);
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreated, &app,
        [url](QObject *obj, const QUrl &objUrl) {
            if (!obj && url == objUrl)
                QCoreApplication::exit(-1);
        },
        Qt::QueuedConnection);

    engine.addImportPath(QCoreApplication::applicationDirPath() + "/qml");
    engine.addImportPath(":/");

    engine.load(url);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
