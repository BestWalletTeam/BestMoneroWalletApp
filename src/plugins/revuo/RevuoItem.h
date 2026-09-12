// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_REVUOITEM_H
#define BESTWALLET_REVUOITEM_H

#include <QString>
#include <QStringList>

struct RevuoItem : QObject
{
    Q_OBJECT

public:
    explicit RevuoItem(QObject *parent)
        : QObject(parent) {};

    QString title;
    QString url;
    QStringList newsbytes;
    QList<QPair<QString, QString>> events;
};

#endif //BESTWALLET_REVUOITEM_H
