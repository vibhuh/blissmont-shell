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

void ConnectionService::applySyncStatus(bool online, int pending) {
    if (engineOnline_ == online && pendingOutbox_ == pending) return;
    engineOnline_ = online;
    pendingOutbox_ = pending;
    emit changed();
}

}  // namespace blissmont::services
