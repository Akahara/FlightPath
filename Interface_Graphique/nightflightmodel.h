#ifndef NIGHTFLIGHTMODEL_H
#define NIGHTFLIGHTMODEL_H

#include "QtCore/qabstractitemmodel.h"

class NightFlightModel : public QAbstractTableModel
{
    QMap<std::string, bool> m_statuses;
public:
    NightFlightModel(QObject * parent = {}) : QAbstractTableModel{parent} {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override { return m_statuses.count(); }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override { return 2; }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == Qt::CheckStateRole && index.column() == 0)
            return m_statuses.values()[index.row()] ? Qt::Checked : Qt::Unchecked;
        if (role == Qt::DisplayRole || role == Qt::EditRole) {
            switch (index.column()) {
            case 1: return QString::fromStdString(m_statuses.keys()[index.row()]);
            default: return {};
            }
        }
        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole) {
            switch (section) {
            case 0: return "Check";
            case 1: return "VFR nuit";
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
            m_statuses[m_statuses.keys()[index.row()]] = value.toBool();
            emit dataChanged(index, index);
            return true;
        }
        return false;
    }

    void append(const std::string &status) {
        beginInsertRows({}, m_statuses.count(), m_statuses.count());
        m_statuses.insert(status, true);
        endInsertRows();
    }

    void setStatuses(const QMap<std::string, bool> &statuses) {
        beginResetModel();
        m_statuses = statuses;
        endResetModel();
    }

    QMap<std::string, bool> &getStatuses() { return m_statuses; }
};

#endif // NIGHTFLIGHTMODEL_H
