// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_NODEMODEL_H
#define BESTWALLET_NODEMODEL_H

#include <QAbstractTableModel>
#include <QIcon>

class NodeEntry;

class NodeModel : public QAbstractTableModel {
    Q_OBJECT

public:
    enum ModelColumn {
        URL,
        Height,
        COUNT
    };

    explicit NodeModel(int nodeSource, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    NodeEntry node(int row);

    void clear();
    void updateNodes(QList<NodeEntry> nodes);

private:
    QList<NodeEntry> m_nodes;
    int m_nodeSource;
    QIcon m_offline;
    QIcon m_online;
};

#endif //BESTWALLET_NODEMODEL_H
