// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_PASSWORDDIALOG_H
#define BESTWALLET_PASSWORDDIALOG_H

#include <QDialog>

#include "components.h"

namespace Ui {
    class PasswordDialog;
}

class PasswordDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit PasswordDialog(const QString &walletName, bool incorrectPassword, bool sensitive = false, QWidget *parent = nullptr);
    ~PasswordDialog() override;

    QString password = "";

private:
    QScopedPointer<Ui::PasswordDialog> ui;
};

#endif //BESTWALLET_PASSWORDDIALOG_H
