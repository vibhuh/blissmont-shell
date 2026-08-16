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

    // ── Config freshness (contracts v1.18.0) ─────────────────────────────────
    //
    // A till holds its whole operating config — permissions, discount caps, payment
    // rails, the cash-variance gate, the receipt layout. Before v1.18.0 a terminal
    // that booted while the backend was unreachable held that config FOREVER with no
    // retry and no alarm, and no surface said so. Retrying silently would have been
    // half a fix: the silence was the defect, so the verdict has to reach a human.
    //
    // configStale is the ENGINE's verdict, deliberately not a raw age the shell
    // re-thresholds. The engine applies two very different thresholds depending on
    // whether it is online (5 min — a server-side defect) or offline (24 h — an
    // outage the POS is designed to trade through), and it holds both signals at the
    // instant it decides. Re-deriving that here would let two surfaces disagree
    // about when to worry.
    Q_PROPERTY(bool configStale READ configStale NOTIFY changed)
    Q_PROPERTY(QString configVerifiedAt READ configVerifiedAt NOTIFY changed)
    Q_PROPERTY(QString configStatusText READ configStatusText NOTIFY changed)

public:
    explicit ConnectionService(QObject* parent = nullptr);

    [[nodiscard]] bool connected() const { return connected_; }
    [[nodiscard]] bool engineOnline() const { return engineOnline_; }
    [[nodiscard]] int pendingOutbox() const { return pendingOutbox_; }
    [[nodiscard]] QString statusText() const;

    [[nodiscard]] bool configStale() const { return configStale_; }
    [[nodiscard]] QString configVerifiedAt() const { return configVerifiedAt_; }
    [[nodiscard]] QString configStatusText() const;

public slots:
    void setConnected(bool value);
    void applySyncStatus(bool online, int pending, bool configStale, const QString& configVerifiedAt);

signals:
    void changed();

private:
    bool connected_ = false;
    bool engineOnline_ = false;
    int pendingOutbox_ = 0;
    bool configStale_ = false;
    QString configVerifiedAt_;
};

}  // namespace blissmont::services
