// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_KEYSDIALOG_H
#define BESTWALLET_KEYSDIALOG_H

#include <QDialog>

#include "components.h"
#include "libwalletqt/Wallet.h"

namespace Ui {
    class KeysDialog;
}

class KeysDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit KeysDialog(Wallet *wallet, QWidget *parent = nullptr);
    ~KeysDialog() override;

private:
    QScopedPointer<Ui::KeysDialog> ui;
};


#endif //BESTWALLET_KEYSDIALOG_H
