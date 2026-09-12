// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_NETWORKMANAGER_H
#define BESTWALLET_NETWORKMANAGER_H

#include <QNetworkAccessManager>

QNetworkAccessManager* getNetworkSocks5();
QNetworkAccessManager* getNetworkClearnet();

QNetworkAccessManager* getNetwork(const QString &address = "");

#endif //BESTWALLET_NETWORKMANAGER_H
