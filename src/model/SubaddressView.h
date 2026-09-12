// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_SUBADDRESSVIEW_H
#define BESTWALLET_SUBADDRESSVIEW_H

#include <QTreeView>
#include <QKeyEvent>
#include <QClipboard>

class SubaddressView : public QTreeView
{
Q_OBJECT

public:
    SubaddressView(QWidget* parent = nullptr);

signals:
    void copyAddress();

protected:
    void keyPressEvent(QKeyEvent *event);
};

#endif //BESTWALLET_SUBADDRESSVIEW_H
