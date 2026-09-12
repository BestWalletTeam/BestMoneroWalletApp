// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_PASSWORDCHANGEDIALOG_H
#define BESTWALLET_PASSWORDCHANGEDIALOG_H

#include <QDialog>

#include "components.h"
#include "libwalletqt/Wallet.h"

namespace Ui {
    class PasswordChangeDialog;
}

class PasswordChangeDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit PasswordChangeDialog(QWidget *parent, Wallet *wallet);
    ~PasswordChangeDialog() override;

private:
    void passwordsMatch();
    void setPassword();

    QScopedPointer<Ui::PasswordChangeDialog> ui;
    Wallet *m_wallet;
};

#endif //BESTWALLET_PASSWORDCHANGEDIALOG_H
