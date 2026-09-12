// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_WALLETINFODIALOG_H
#define BESTWALLET_WALLETINFODIALOG_H

#include <QDialog>

#include "components.h"
#include "libwalletqt/Wallet.h"

namespace Ui {
    class WalletInfoDialog;
}

class WalletInfoDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit WalletInfoDialog(Wallet *wallet, QWidget *parent = nullptr);
    ~WalletInfoDialog() override;

private:
    void openWalletDir();

    QScopedPointer<Ui::WalletInfoDialog> ui;
    Wallet *m_wallet;
};

#endif //BESTWALLET_WALLETINFODIALOG_H
