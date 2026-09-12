// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_SIGNVERIFYDIALOG_H
#define BESTWALLET_SIGNVERIFYDIALOG_H

#include <QDialog>

#include "components.h"
#include "libwalletqt/Wallet.h"

namespace Ui {
    class SignVerifyDialog;
}

class SignVerifyDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit SignVerifyDialog(Wallet *wallet, QWidget *parent = nullptr);
    ~SignVerifyDialog() override;

private slots:
    void signMessage();
    void verifyMessage();
    void copyToClipboard();

private:
    QScopedPointer<Ui::SignVerifyDialog> ui;
    Wallet *m_wallet;
};


#endif //BESTWALLET_SIGNVERIFYDIALOG_H
