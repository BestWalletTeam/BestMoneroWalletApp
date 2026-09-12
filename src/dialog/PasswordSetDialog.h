// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_PASSWORDSETDIALOG_H
#define BESTWALLET_PASSWORDSETDIALOG_H

#include <QDialog>

#include "components.h"

namespace Ui {
    class PasswordSetDialog;
}

class PasswordSetDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit PasswordSetDialog(const QString &helpText, QWidget *parent = nullptr);
    ~PasswordSetDialog() override;

    QString password();

private:
    QScopedPointer<Ui::PasswordSetDialog> ui;
};


#endif //BESTWALLET_PASSWORDSETDIALOG_H
