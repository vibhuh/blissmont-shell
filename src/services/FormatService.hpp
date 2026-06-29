// services/FormatService.hpp — the `Format` QML singleton (refinement brief, Phase 1).
//
// ONE formatting standard, routed through ONE object, so every numeric value on screen
// (sale grid, totals, tender, history, status line, customer/disc) is rendered the same
// way — no per-widget formatting, no raw engine precision ("1003.0000000000") leaking to
// the cashier. The number math lives in core/numfmt (Qt-free, unit-tested); this thin
// wrapper adds the currency symbol and exposes the helpers to QML as `Format.money(...)`.
//
// currencySymbol mirrors ConfigService.currencySymbol (bound in Main.qml) so a config
// push that changes the symbol flows through every formatted amount with no other wiring.
#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>

namespace blissmont::services {

class Format : public QObject {
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString currencySymbol READ currencySymbol WRITE setCurrencySymbol
                   NOTIFY currencySymbolChanged)

public:
    explicit Format(QObject* parent = nullptr);

    [[nodiscard]] QString currencySymbol() const { return currencySymbol_; }
    void setCurrencySymbol(const QString& symbol);

    // Grouped, exactly 2 decimals, WITH currency symbol: "₹1,003.00", "₹0.00". Negatives
    // render as "−₹5.00" (sign before the symbol).
    Q_INVOKABLE QString money(const QString& value) const;
    // Same as money() but WITHOUT the symbol — for cells that show the symbol separately.
    Q_INVOKABLE QString amount(const QString& value) const;
    // Plain 2-dp value: NO symbol, NO grouping ("1003.00"). For editable amount fields whose
    // text is sent back to the engine, which parses a bare decimal (a grouping comma would
    // break it). plain("1003.0000000000") -> "1003.00"; plain("600.5") -> "600.50".
    Q_INVOKABLE QString plain(const QString& value, int decimals = 2) const;
    // Quantity: whole numbers bare ("1"), fractional trimmed to UOM precision ("1.25").
    Q_INVOKABLE QString qty(const QString& value, int precision = 3) const;
    // Percent value -> "18.00%". (Already-percent input, e.g. a bill-discount percentage.)
    Q_INVOKABLE QString percent(const QString& value) const;
    // The engine's tax_rate_str is a FRACTION ("0.05") -> "5.00%".
    Q_INVOKABLE QString taxRate(const QString& fraction) const;

signals:
    void currencySymbolChanged();

private:
    QString currencySymbol_ = QStringLiteral("₹");
};

}  // namespace blissmont::services
