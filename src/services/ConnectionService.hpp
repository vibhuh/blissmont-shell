// services/ConnectionService.hpp — engine connectivity + sync status for the UI (spec §5).
//
// View-facing projection of the bridge's connection state and the engine's SyncStatusChanged
// events. The status bar / connectivity indicator binds to these. Wired to the bridge in
// main.cpp (bridge signals -> this service's slots) so the service has no gRPC dependency.
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

namespace blissmont::services {

class ConnectionService : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(bool connected READ connected NOTIFY changed)
    Q_PROPERTY(bool engineOnline READ engineOnline NOTIFY changed)  // engine's own backend sync
    Q_PROPERTY(int pendingOutbox READ pendingOutbox NOTIFY changed)
    Q_PROPERTY(QString statusText READ statusText NOTIFY changed)

public:
    explicit ConnectionService(QObject* parent = nullptr);

    [[nodiscard]] bool connected() const { return connected_; }
    [[nodiscard]] bool engineOnline() const { return engineOnline_; }
    [[nodiscard]] int pendingOutbox() const { return pendingOutbox_; }
    [[nodiscard]] QString statusText() const;

public slots:
    void setConnected(bool value);
    void applySyncStatus(bool online, int pending);

signals:
    void changed();

private:
    bool connected_ = false;
    bool engineOnline_ = false;
    int pendingOutbox_ = 0;
};

}  // namespace blissmont::services
