// services/ConnectionService.cpp — see ConnectionService.hpp.
#include "services/ConnectionService.hpp"

namespace blissmont::services {

ConnectionService::ConnectionService(QObject* parent) : QObject(parent) {}

QString ConnectionService::statusText() const {
    if (!connected_) return QStringLiteral("Disconnected from engine");
    if (!engineOnline_) {
        return pendingOutbox_ > 0
                   ? QStringLiteral("Offline — %1 pending").arg(pendingOutbox_)
                   : QStringLiteral("Offline");
    }
    return pendingOutbox_ > 0 ? QStringLiteral("Online — syncing %1").arg(pendingOutbox_)
                              : QStringLiteral("Online");
}

void ConnectionService::setConnected(bool value) {
    if (connected_ == value) return;
    connected_ = value;
    emit changed();
}

// configStatusText is what a human is shown when the config has gone stale. The
// two cases read differently on purpose: "the server is reachable and we still
// cannot refresh" is an operational defect worth escalating, while "we have been
// offline a long time" is something the shop already knows and can act on.
QString ConnectionService::configStatusText() const {
    if (!configStale_) return {};
    if (engineOnline_) {
        // Online and unable to refresh: the shape of a server-side fault, not an
        // outage. Say so plainly — this is the case that hid for three weeks.
        return QStringLiteral("Config not updating — contact support");
    }
    return configVerifiedAt_.isEmpty()
               ? QStringLiteral("Config never synced")
               : QStringLiteral("Config last synced %1").arg(configVerifiedAt_);
}

void ConnectionService::applySyncStatus(bool online, int pending, bool configStale,
                                        const QString& configVerifiedAt) {
    if (engineOnline_ == online && pendingOutbox_ == pending && configStale_ == configStale &&
        configVerifiedAt_ == configVerifiedAt) {
        return;
    }
    engineOnline_ = online;
    pendingOutbox_ = pending;
    configStale_ = configStale;
    configVerifiedAt_ = configVerifiedAt;
    emit changed();
}

}  // namespace blissmont::services
