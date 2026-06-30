// viewmodels/LookupController.cpp — see LookupController.hpp.
#include "viewmodels/LookupController.hpp"

#include <QRegularExpression>

#include <algorithm>

namespace blissmont::viewmodels {

LookupController::LookupController(QObject* parent) : QAbstractListModel(parent) {}

int LookupController::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return static_cast<int>(filtered_.size());
}

QVariant LookupController::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() < 0 || index.row() >= filtered_.size()) return {};
    if (role == ItemRole) return filtered_.at(index.row()).payload;
    return {};
}

QHash<int, QByteArray> LookupController::roleNames() const {
    return {{ItemRole, "item"}};
}

void LookupController::setSearchText(const QString& text) {
    if (searchText_ == text) return;
    searchText_ = text;
    emit searchTextChanged();
    rebuild();
}

void LookupController::setScope(const QString& scope) {
    if (scope_ == scope) return;
    scope_ = scope;
    emit scopeChanged();
    rebuild();
}

void LookupController::setSearchKeys(const QStringList& keys) {
    if (searchKeys_ == keys) return;
    searchKeys_ = keys;
    emit searchKeysChanged();
    rebuild();
}

void LookupController::setBarcodeKey(const QString& key) {
    if (barcodeKey_ == key) return;
    barcodeKey_ = key;
    emit barcodeKeyChanged();
    rebuild();
}

void LookupController::setItems(const QVariantList& items) {
    source_ = items;
    rebuild();
}

QVariantMap LookupController::itemAt(int row) const {
    if (row < 0 || row >= filtered_.size()) return {};
    return filtered_.at(row).payload;
}

QVariantMap LookupController::currentItem() const { return itemAt(currentIndex_); }

QVariantMap LookupController::selectedItem() const { return itemAt(resolveSelectionIndex()); }

int LookupController::resolveSelectionIndex() const {
    // No implicit selection without a query: an empty field has nothing to "select".
    if (searchText_.trimmed().isEmpty() || filtered_.isEmpty()) return -1;
    // Sorted best-first, so an exact match (if any) is row 0.
    if (filtered_.front().tier == Tier::Exact) return 0;
    if (filtered_.size() == 1) return 0;                  // single filtered result
    if (currentIndex_ >= 0 && currentIndex_ < filtered_.size()) return currentIndex_;  // highlighted
    return -1;
}

void LookupController::setCurrentIndex(int index) {
    int clamped = index;
    if (filtered_.isEmpty()) clamped = -1;
    else clamped = std::clamp(index, 0, static_cast<int>(filtered_.size()) - 1);
    setCurrentIndexInternal(clamped);
}

void LookupController::setCurrentIndexInternal(int index) {
    if (currentIndex_ == index) return;
    currentIndex_ = index;
    emit currentIndexChanged();
}

void LookupController::moveUp() {
    if (filtered_.isEmpty()) return;
    setCurrentIndex(currentIndex_ <= 0 ? 0 : currentIndex_ - 1);
}

void LookupController::moveDown() {
    if (filtered_.isEmpty()) return;
    setCurrentIndex(currentIndex_ + 1);  // clamped to last
}

void LookupController::movePage(int delta) {
    if (filtered_.isEmpty()) return;
    setCurrentIndex((currentIndex_ < 0 ? 0 : currentIndex_) + delta);
}

void LookupController::moveHome() {
    if (filtered_.isEmpty()) return;
    setCurrentIndex(0);
}

void LookupController::moveEnd() {
    if (filtered_.isEmpty()) return;
    setCurrentIndex(static_cast<int>(filtered_.size()) - 1);
}

LookupController::Tier LookupController::rankField(const QString& field, const QString& term) {
    const QString f = field.toLower();
    if (f == term) return Tier::Exact;
    if (f.startsWith(term)) return Tier::StartsWith;
    // Word-starts-with: any word (split on whitespace / - _ /) begins with the term.
    static const QRegularExpression sep(QStringLiteral("[\\s\\-_/]+"));
    const QStringList words = f.split(sep, Qt::SkipEmptyParts);
    for (const QString& w : words)
        if (w.startsWith(term)) return Tier::WordStarts;
    if (f.contains(term)) return Tier::Contains;
    return Tier::None;
}

LookupController::Tier LookupController::rankRow(const QVariantMap& payload, const QString& term) const {
    const QStringList keys = scope_ == QStringLiteral("barcode") ? QStringList{barcodeKey_} : searchKeys_;
    Tier best = Tier::None;
    for (const QString& key : keys) {
        const QString value = payload.value(key).toString();
        if (value.isEmpty()) continue;
        const Tier t = rankField(value, term);
        if (static_cast<int>(t) < static_cast<int>(best)) best = t;
        if (best == Tier::Exact) break;  // can't do better
    }
    return best;
}

void LookupController::rebuild() {
    beginResetModel();
    filtered_.clear();

    const QString term = searchText_.toLower().trimmed();
    if (term.isEmpty()) {
        // No query: every row, original order, highlight the first.
        filtered_.reserve(source_.size());
        for (const QVariant& v : source_) filtered_.push_back({v.toMap(), Tier::None});
    } else {
        // Rank every source row; keep matches; stable-sort best-first (ties keep input order).
        struct Scored {
            int origIndex;
            Tier tier;
            QVariantMap payload;
        };
        QVector<Scored> scored;
        scored.reserve(source_.size());
        for (int i = 0; i < source_.size(); ++i) {
            const QVariantMap payload = source_.at(i).toMap();
            const Tier t = rankRow(payload, term);
            if (t != Tier::None) scored.push_back({i, t, payload});
        }
        std::stable_sort(scored.begin(), scored.end(), [](const Scored& a, const Scored& b) {
            return static_cast<int>(a.tier) < static_cast<int>(b.tier);
        });
        filtered_.reserve(scored.size());
        for (const Scored& s : scored) filtered_.push_back({s.payload, s.tier});
    }

    endResetModel();
    emit countChanged();
    // Highlight is list-state: reset to the best match (row 0) on every filter.
    setCurrentIndexInternal(filtered_.isEmpty() ? -1 : 0);
}

}  // namespace blissmont::viewmodels
