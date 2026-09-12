// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_BALANCEDIALOG_H
#define BESTWALLET_BALANCEDIALOG_H

#include "components.h"
#include "libwalletqt/Wallet.h"

namespace Ui {
    class BalanceDialog;
}

class BalanceDialog : public WindowModalDialog
{
    Q_OBJECT

public:
    explicit BalanceDialog(QWidget *parent, Wallet *wallet);
    ~BalanceDialog() override;

private:
    void updateBalance();

    QScopedPointer<Ui::BalanceDialog> ui;
    Wallet *m_wallet;
};

#endif //BESTWALLET_BALANCEDIALOG_H
