// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_SEEDDIALOG_H
#define BESTWALLET_SEEDDIALOG_H

#include <QDialog>

#include "components.h"
#include "libwalletqt/Wallet.h"

namespace Ui {
    class SeedDialog;
}

class SeedDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit SeedDialog(Wallet *wallet, QWidget *parent = nullptr);
    ~SeedDialog() override;

private:
    void setSeed(const QString &seed);

    QScopedPointer<Ui::SeedDialog> ui;
    Wallet *m_wallet;
};


#endif //BESTWALLET_SEEDDIALOG_H
