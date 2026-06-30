// viewmodels/LookupController.hpp — the reusable ranked keyboard-lookup engine
// (SHELL_KEYBOARD_LOOKUP brief, Part 1).
//
// THE REFERENCE IMPLEMENTATION. Every keyboard-first lookup in BlissMont (items, customer,
// supplier, ledger, warehouse, tax-code) drives ONE of these. It is a QAbstractListModel
// exposing the FILTERED + RANKED rows, plus the lookup STATE the keyboard UX needs:
//   • ranked filter — exact > starts-with > word-starts-with > contains, best match first
//     (so typing "app" puts "Apple Juice" (starts-with) above "Pineapple" (contains));
//   • a highlight that is LIST-STATE, not focus-state — `currentIndex` is reset to row 0 on
//     every filter, so a best match is highlighted from the first keystroke regardless of
//     where keyboard focus lives (this is what makes "Enter without Down-arrow" work);
//   • the Enter-selection PRIORITY (resolveSelectionIndex): exact → single result →
//     highlighted → none.
//
// Pure logic, NO QML/widget dependency — the Qt key-routing (Tab===Enter, Down→table,
// printable-redirect, Esc) lives in the reusable QML helper LookupKeys.qml; THIS is the
// testable core both it and the unit tests exercise directly.
//
// Source rows are generic QVariantMap payloads (so any lookup reuses it): setItems() takes the
// list, `searchKeys` names the payload keys searched in "all" scope (the first is the primary
// name), and `barcodeKey` the single key searched in "barcode" scope. The whole payload comes
// back out via the "item" role / itemAt() / selectedItem(), so the consumer's delegate renders
// whatever fields it likes.
#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QVector>
#include <QQmlEngine>

namespace blissmont::viewmodels {

class LookupController : public QAbstractListModel {
    Q_OBJECT
    QML_ELEMENT

    // The live query. Setting it re-filters + re-ranks and resets currentIndex to the best
    // match (row 0), so the highlight tracks the filter, not keyboard focus.
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY searchTextChanged)
    // "all" (searchKeys) | "barcode" (barcodeKey only — the single indexed column, fast at scale).
    Q_PROPERTY(QString scope READ scope WRITE setScope NOTIFY scopeChanged)
    // Payload keys searched in "all" scope; the FIRST is the primary name (drives word-starts).
    Q_PROPERTY(QStringList searchKeys READ searchKeys WRITE setSearchKeys NOTIFY searchKeysChanged)
    // Payload key searched in "barcode" scope.
    Q_PROPERTY(QString barcodeKey READ barcodeKey WRITE setBarcodeKey NOTIFY barcodeKeyChanged)
    // The highlighted (current) filtered row; -1 only when there are no results.
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentIndexChanged)
    // Filtered row count (convenience for QML; == rowCount()).
    Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
    // Match quality, best (lowest) first. Drives BOTH the visible order and the highlight.
    enum class Tier { Exact = 0, StartsWith = 1, WordStarts = 2, Contains = 3, None = 4 };

    enum Role { ItemRole = Qt::UserRole + 1 };

    explicit LookupController(QObject* parent = nullptr);

    // ── QAbstractListModel ────────────────────────────────────────────────────
    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] QString searchText() const { return searchText_; }
    void setSearchText(const QString& text);
    [[nodiscard]] QString scope() const { return scope_; }
    void setScope(const QString& scope);
    [[nodiscard]] QStringList searchKeys() const { return searchKeys_; }
    void setSearchKeys(const QStringList& keys);
    [[nodiscard]] QString barcodeKey() const { return barcodeKey_; }
    void setBarcodeKey(const QString& key);
    [[nodiscard]] int currentIndex() const { return currentIndex_; }
    // Q_INVOKABLE so QML can also call it explicitly (e.g. a row click sets the highlight),
    // not only assign the property. Clamps to a valid row (or -1 when empty).
    Q_INVOKABLE void setCurrentIndex(int index);
    [[nodiscard]] int count() const { return static_cast<int>(filtered_.size()); }

    // Replace the source rows. Re-filters against the current searchText/scope and resets the
    // highlight to the best match. Each entry is a QVariantMap payload.
    Q_INVOKABLE void setItems(const QVariantList& items);

    // Payload of a FILTERED row (empty map if out of range).
    Q_INVOKABLE QVariantMap itemAt(int row) const;
    // Payload of the highlighted row (empty map if none).
    Q_INVOKABLE QVariantMap currentItem() const;

    // Enter-selection priority (never requires Down first):
    //   1) exact match exists      → its row
    //   2) else single result      → row 0
    //   3) else highlighted row    → currentIndex
    //   4) else                    → -1
    // Requires a non-empty query: an empty search has no implicit selection (so Enter on an
    // empty field is a no-op / live-scan fallback, not "pick whatever is on top").
    Q_INVOKABLE int resolveSelectionIndex() const;
    // Payload at resolveSelectionIndex() (empty map if -1) — the one call a consumer's Enter needs.
    Q_INVOKABLE QVariantMap selectedItem() const;

    // Highlight navigation (clamped; no-ops when empty). The table's arrow/page keys call these.
    Q_INVOKABLE void moveUp();
    Q_INVOKABLE void moveDown();
    Q_INVOKABLE void movePage(int delta);
    Q_INVOKABLE void moveHome();
    Q_INVOKABLE void moveEnd();

signals:
    void searchTextChanged();
    void scopeChanged();
    void searchKeysChanged();
    void barcodeKeyChanged();
    void currentIndexChanged();
    void countChanged();

private:
    struct Row {
        QVariantMap payload;
        Tier tier = Tier::None;
    };

    // Best (lowest) tier of `term` across the in-scope fields of `payload`.
    Tier rankRow(const QVariantMap& payload, const QString& term) const;
    // Tier of `term` within a single field value.
    static Tier rankField(const QString& field, const QString& term);
    // Recompute filtered_ from source_ + searchText_ + scope_; resets the highlight.
    void rebuild();
    void setCurrentIndexInternal(int index);

    QVariantList source_;
    QVector<Row> filtered_;
    QString searchText_;
    QString scope_ = QStringLiteral("all");
    QStringList searchKeys_{QStringLiteral("name")};
    QString barcodeKey_ = QStringLiteral("barcode");
    int currentIndex_ = -1;
};

}  // namespace blissmont::viewmodels
