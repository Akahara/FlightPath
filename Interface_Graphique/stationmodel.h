#ifndef STATIONMODEL_H
#define STATIONMODEL_H

#include "QtCore/qabstractitemmodel.h"
#include "Solver/src/station.h"

class StationModel : public QAbstractTableModel
{
    QList<Station> m_stations;
public:
    StationModel(QObject * parent = {}) : QAbstractTableModel{parent} {}

    int rowCount(const QModelIndex &parent = QModelIndex()) const override { return m_stations.count(); }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override { return 8; }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override {
        if (role == Qt::CheckStateRole && index.column() == 0) {
            // Renvoyer l'état de la case à cocher
            return m_stations[index.row()].isExcluded() ? Qt::Checked : Qt::Unchecked;
        }
        if(role == Qt::DisplayRole || role == Qt::EditRole) {
          switch (index.column()) {
            //case 0: return m_excluded[index.row()] ? Qt::Checked : Qt::Unchecked;
            case 1: return QString::fromStdString(m_stations[index.row()].getOACI());
            case 2: return QString::fromStdString(m_stations[index.row()].getName());
            case 3: return m_stations[index.row()].getLocation().lat;
            case 4: return m_stations[index.row()].getLocation().lon;
            case 5: return QString::fromStdString(m_stations[index.row()].getStatus());
            case 6: return QString::fromStdString(m_stations[index.row()].getNightVFR());
            case 7: return QString::fromStdString(m_stations[index.row()].getFuel());
            default: return {};
          }
        }
        return QVariant();
    }

    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override {
        if (role == Qt::DisplayRole) {
            switch (section) {
            case 0: return "Exclure";
            case 1: return "code OACI";
            case 2: return "Nom";
            case 3: return "Latitude";
            case 4: return "Longitude";
            case 5: return "Statut";
            case 6: return "VFR de nuit";
            case 7: return "Carburant";
            default: return {};
            }
        }
        return QVariant();
    }

    Qt::ItemFlags flags(const QModelIndex &index) const override {
        Qt::ItemFlags flags = QAbstractTableModel::flags(index);
        if (index.column() == 0) {
            flags |= Qt::ItemIsUserCheckable;
        } else if (index.column() == 3 || index.column() == 4) {
            // Nothing
        }
        else {
            flags |= Qt::ItemIsEditable;
        }
        return flags;
    }

    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override {
        emit dataChanged(index, index, {role});
        if (role == Qt::CheckStateRole && index.column() == 0 && index.isValid()) {
            // Mettre à jour la valeur de la case à cocher
            m_stations[index.row()].setExcluded(value == Qt::Checked);
            emit dataChanged(index, index, {role});
            return true;
        } else if (role == Qt::EditRole && index.isValid()) {
            switch (index.column()) {
            case 1: m_stations[index.row()].setOACI(value.toString().toStdString()); break;
            case 2: m_stations[index.row()].setName(value.toString().toStdString()); break;
            case 3: return false;
            case 4: return false;
            case 5: m_stations[index.row()].setStatus(value.toString().toStdString()); break;
            case 6: m_stations[index.row()].setNightVFR(value.toString().toStdString()); break;
            case 7: m_stations[index.row()].setFuel(value.toString().toStdString()); break;
            default: return false;
            }
            emit dataChanged(index, index, {role});
            return true;
        }

        return QAbstractTableModel::setData(index, value, role);
    }

    void append(const Station &station) {
        beginInsertRows({}, m_stations.count(), m_stations.count());
        m_stations.append(station);
        endInsertRows();
    }

    QList<Station> &getStations() { return m_stations; }
};

#endif // STATIONMODEL_H
