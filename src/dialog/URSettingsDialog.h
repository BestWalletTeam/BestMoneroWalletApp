// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_URSETTINGSDIALOG_H
#define BESTWALLET_URSETTINGSDIALOG_H

#include <QDialog>

#include "components.h"

namespace Ui {
    class URSettingsDialog;
}

class URSettingsDialog : public WindowModalDialog
{
    Q_OBJECT

public:
    explicit URSettingsDialog(QWidget *parent = nullptr);
    ~URSettingsDialog() override;

private:
    QScopedPointer<Ui::URSettingsDialog> ui;
};


#endif //BESTWALLET_URSETTINGSDIALOG_H
