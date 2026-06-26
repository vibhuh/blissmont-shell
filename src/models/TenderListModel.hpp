// models/TenderListModel.hpp — the bill's running tenders as a QAbstractListModel.
//
// Reset wholesale from each CartUpdated snapshot (same discipline as CartLineModel): the
// engine pushes a full cart — including its tenders — on every change, so we replace rows
// rather than patch them. QML's TenderPanel binds the tender rows (with the remove
// affordance) to the role names below; remove sends RemoveTender{tenderNo}.
#pragma once

#include <QAbstractListModel>
#include <QList>
#include <QString>

#include <google/protobuf/repeated_ptr_field.h>

#include "terminal/v1/terminal.pb.h"

namespace blissmont::models {

class TenderListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Role {
        TenderNoRole = Qt::UserRole + 1,
        MethodRole,
        AmountRole,
        ReferenceRole,
    };

    explicit TenderListModel(QObject* parent = nullptr);

    int rowCount(const QModelIndex& parent = {}) const override;
    QVariant data(const QModelIndex& index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    // Replace all rows from a fresh engine snapshot.
    void reset(const blissmont::terminal::v1::CartUpdated& snapshot);
    // Replace all rows from a bare repeated Tender field (e.g. BillDetail.payments for a
    // recalled bill). The CartUpdated overload delegates here — same row shape, two sources.
    void reset(const google::protobuf::RepeatedPtrField<
               blissmont::terminal::v1::Tender>& tenders);

private:
    struct Tender {
        int tenderNo = 0;
        QString method;
        QString amount;
        QString reference;
    };
    QList<Tender> tenders_;
};

}  // namespace blissmont::models
