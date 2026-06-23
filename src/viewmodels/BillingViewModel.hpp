// viewmodels/BillingViewModel.hpp — billing-screen presentation logic (spec §5).
//
// Holds ONLY pending UI intent (the current scan-field text and a transient status line) —
// never a copy of cart state (the engine owns that; QML binds the cart directly to the
// bridge's model/summary). Sends commands through the bridge and reflects discrete events
// (itemNotFound, commandRejected, orderSettled) as a status message. No QML dependency, so
// it is unit-tested headless against a bridge double.
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

#include "bridge/PosEngineBridge.hpp"

namespace blissmont::viewmodels {

class BillingViewModel : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(blissmont::bridge::PosEngineBridge* bridge READ bridge WRITE setBridge NOTIFY bridgeChanged)
    Q_PROPERTY(QString scanText READ scanText WRITE setScanText NOTIFY scanTextChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusMessageChanged)

public:
    explicit BillingViewModel(QObject* parent = nullptr);

    [[nodiscard]] blissmont::bridge::PosEngineBridge* bridge() const { return bridge_; }
    void setBridge(blissmont::bridge::PosEngineBridge* bridge);

    [[nodiscard]] QString scanText() const { return scanText_; }
    void setScanText(const QString& text);

    [[nodiscard]] QString statusMessage() const { return statusMessage_; }

    // Scan-field-is-home law (UX §3): submit the current code, then clear so focus returns
    // to an empty scan field. A blank submit is a no-op.
    Q_INVOKABLE void submitScan();

signals:
    void bridgeChanged();
    void scanTextChanged();
    void statusMessageChanged();

private:
    void setStatus(const QString& message);
    void wireBridge();

    blissmont::bridge::PosEngineBridge* bridge_ = nullptr;
    QString scanText_;
    QString statusMessage_;
};

}  // namespace blissmont::viewmodels
