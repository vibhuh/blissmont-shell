// bridge/PosEngineBridge.hpp — the ONLY component that touches gRPC (spec §4).
//
// One QObject, exposed to QML as a singleton. Owns the localhost gRPC channel to the
// engine and the bidirectional `Session` stream. UI -> engine is a set of thin Q_INVOKABLE
// methods that each build one Command oneof and write it. Engine -> UI is: cart lines via
// CartLineModel (full reset per snapshot), totals via CartSummary Q_PROPERTYs, and discrete
// events via Qt signals. The engine is the single source of truth; the bridge never mutates
// cart state itself — it renders snapshots.
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QVariantList>

#include <atomic>
#include <memory>

#include <grpcpp/grpcpp.h>
#include "terminal/v1/terminal.grpc.pb.h"

#include "models/CartLineModel.hpp"
#include "models/CartSummary.hpp"
#include "models/TenderListModel.hpp"

namespace blissmont::bridge {

class PosEngineBridge : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(blissmont::models::CartLineModel* cart READ cart CONSTANT)
    Q_PROPERTY(blissmont::models::CartSummary* summary READ summary CONSTANT)
    Q_PROPERTY(blissmont::models::TenderListModel* tenders READ tenders CONSTANT)
    Q_PROPERTY(bool connected READ connected NOTIFY connectionChanged)

public:
    explicit PosEngineBridge(QObject* parent = nullptr);
    ~PosEngineBridge() override;

    [[nodiscard]] blissmont::models::CartLineModel* cart() const { return cart_; }
    [[nodiscard]] blissmont::models::CartSummary* summary() const { return summary_; }
    [[nodiscard]] blissmont::models::TenderListModel* tenders() const { return tenders_; }
    [[nodiscard]] bool connected() const { return connected_.load(std::memory_order_relaxed); }

    // ── UI -> engine (one per Command oneof; thin: build + write) ─────────────
    Q_INVOKABLE void connectToEngine(const QString& target = QStringLiteral("localhost:50080"));
    Q_INVOKABLE void disconnectFromEngine();

    Q_INVOKABLE void scanItem(const QString& barcode);
    Q_INVOKABLE void addLine(const QString& itemId, const QString& qty);
    Q_INVOKABLE void setQty(int lineNo, const QString& qty);
    Q_INVOKABLE void setLineDiscount(int lineNo, const QString& discount);
    Q_INVOKABLE void removeLine(int lineNo);
    Q_INVOKABLE void setOrderDiscount(const QString& discount);
    Q_INVOKABLE void addTender(const QString& method, const QString& amount, const QString& reference);
    Q_INVOKABLE void removeTender(int tenderNo);
    Q_INVOKABLE void settle();
    Q_INVOKABLE void voidCart();
    Q_INVOKABLE void addMiscCharge(const QString& description, const QString& amount);
    Q_INVOKABLE void recordPayout(const QString& amount, const QString& category, const QString& note);
    Q_INVOKABLE void startReturn(const QString& receiptNo, bool blind);
    Q_INVOKABLE void runEod();

signals:
    void connectionChanged();
    void orderSettled(const QString& receiptNo, bool provisional, const QString& total);
    void itemNotFound(const QString& barcode);
    void commandRejected(const QString& code, const QString& message);
    void shiftStateChanged(const QString& shiftId, const QString& status);
    void syncStatusChanged(bool online, int pending);
    // Device config relayed by the engine over the Session stream (contracts
    // v1.1.0; payment methods added in v1.2.0). Emitted on connect, on reconnect,
    // and on every config change, with the device-domain fields the UI gates on.
    // Carries plain Qt types so services stay free of any gRPC/proto dependency
    // (the bridge owns that translation). paymentMethods is a QVariantList of
    // QVariantMap rows {method, displayName, hotkey, sortOrder, enabled,
    // referenceMode} — the device-surface fields only; tender secrets never cross.
    void configUpdated(bool allowReturns, bool payoutEnabled, bool allowDiscounts,
                       const QString& tenderCompleteMode, const QString& currencySymbol,
                       const QVariantList& paymentMethods);
    void authRequired(const QString& action, const QString& reason);
    void returnContextLoaded();
    void historyResults();

private:
    using Command = blissmont::terminal::v1::Command;
    using Event = blissmont::terminal::v1::Event;
    using Stream = grpc::ClientReaderWriter<Command, Event>;

    void writeCommand(Command cmd);          // thread-safe writer (1 writer)
    void readLoop();                         // worker thread (1 reader)
    void applyEvent(const Event& evt);       // UI thread (marshaled via QueuedConnection)
    void setConnected(bool value);

    blissmont::models::CartLineModel* cart_;
    blissmont::models::CartSummary* summary_;
    blissmont::models::TenderListModel* tenders_;

    std::shared_ptr<grpc::Channel> channel_;
    std::unique_ptr<blissmont::terminal::v1::TerminalEngine::Stub> stub_;
    std::unique_ptr<grpc::ClientContext> ctx_;
    std::unique_ptr<Stream> stream_;

    std::unique_ptr<QThread> readerThread_;
    QMutex writeMutex_;
    std::atomic<bool> connected_{false};
    std::atomic<bool> stopping_{false};
};

}  // namespace blissmont::bridge
