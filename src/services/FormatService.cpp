// services/FormatService.cpp — see FormatService.hpp. Thin Qt/QML wrapper over core/numfmt.
#include "services/FormatService.hpp"

#include "core/Format.hpp"

namespace blissmont::services {

Format::Format(QObject* parent) : QObject(parent) {}

void Format::setCurrencySymbol(const QString& symbol) {
    if (currencySymbol_ == symbol) return;
    currencySymbol_ = symbol;
    emit currencySymbolChanged();
}

QString Format::money(const QString& value) const {
    std::string m = core::numfmt::money(value.toStdString());
    const bool negative = !m.empty() && m.front() == '-';
    if (negative) m.erase(0, 1);
    const QString body = currencySymbol_ + QString::fromStdString(m);
    return negative ? QString::fromUtf8("−") + body : body;  // U+2212 MINUS SIGN
}

QString Format::amount(const QString& value) const {
    return QString::fromStdString(core::numfmt::money(value.toStdString()));
}

QString Format::plain(const QString& value, int decimals) const {
    return QString::fromStdString(core::numfmt::round(value.toStdString(), decimals));
}

QString Format::qty(const QString& value, int precision) const {
    return QString::fromStdString(core::numfmt::qty(value.toStdString(), precision));
}

QString Format::percent(const QString& value) const {
    return QString::fromStdString(core::numfmt::percent(value.toStdString()));
}

QString Format::taxRate(const QString& fraction) const {
    return QString::fromStdString(core::numfmt::fractionAsPercent(fraction.toStdString()));
}

}  // namespace blissmont::services
