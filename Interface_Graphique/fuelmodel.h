#ifndef FUELMODEL_H
#define FUELMODEL_H

#include "QtCore/qabstractitemmodel.h"

class FuelModel : public QAbstractTableModel
{
    QMap<std::string, bool> m_fuels;
public:
    FuelModel(QObject * parent = {}) : QAbstractTableModel{parent} {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override { return m_fuels.count(); }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override { return 2; }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == Qt::CheckStateRole && index.column() == 0) {
            // Renvoyer l'état de la case à cocher
            return m_fuels.values()[index.row()] ? Qt::Checked : Qt::Unchecked;
        }
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch (index.column()) {
            //case 0: return m_fuels.values()[index.row()] ? Qt::Checked : Qt::Unchecked;
            case 1: return QString::fromStdString(m_fuels.keys()[index.row()]);
            default: return {};
            }
        }
        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole) {
            switch (section) {
            case 0: return "Check";
            case 1: return "Carburant";
            default: return {};
            }
        }
        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override {
        Qt::ItemFlags flags = QAbstractTableModel::flags(index);
        if (index.column() == 0) {
            flags |= Qt::ItemIsUserCheckable;
        } else {
            flags |= Qt::ItemIsEditable;
        }
        return flags;
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override {
        if (role == Qt::CheckStateRole && index.column() == 0) {
            m_fuels[m_fuels.keys()[index.row()]] = value.toBool();
            emit dataChanged(index, index);
            return true;
        }
        return false;
    }

    void append(const std::string &fuel) {
        beginInsertRows({}, m_fuels.count(), m_fuels.count());
        m_fuels.insert(fuel, true);
        endInsertRows();
    }

    void setFuels(const QMap<std::string, bool> &fuels) {
        beginResetModel();
        m_fuels = fuels;
        endResetModel();
    }

    QMap<std::string, bool> &getFuels() { return m_fuels; }
};

#endif // FUELMODEL_H
