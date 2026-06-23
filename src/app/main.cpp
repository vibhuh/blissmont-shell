// app/main.cpp — GUI bootstrap (spec §5).
// QGuiApplication + QML engine, load the Blissmont.Shell module's Main window. All C++ logic
// lives in libshell (linked here and by the tests); this file is pure presentation wiring.
// The bridge<->service connections and the frozen keymap (F-key Shortcuts) live in QML.
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>

int main(int argc, char* argv[]) {
    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("BlissMont POS"));
    QGuiApplication::setOrganizationName(QStringLiteral("BlissMont"));

    QQuickStyle::setStyle(QStringLiteral("Basic"));  // deterministic, keyboard-first controls

    QQmlApplicationEngine engine;
    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
        [] { QCoreApplication::exit(-1); }, Qt::QueuedConnection);

    engine.loadFromModule("Blissmont.Shell", "Main");

    return app.exec();
}
