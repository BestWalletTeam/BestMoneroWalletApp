// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_OUTPUTINFODIALOG_H
#define BESTWALLET_OUTPUTINFODIALOG_H

#include <QDialog>

#include "components.h"
#include "libwalletqt/Coins.h"
#include "libwalletqt/rows/CoinsInfo.h"

namespace Ui {
    class OutputInfoDialog;
}

class OutputInfoDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit OutputInfoDialog(const CoinsInfo &cInfo, QWidget *parent = nullptr);
    ~OutputInfoDialog() override;

private:
    QScopedPointer<Ui::OutputInfoDialog> ui;
};

#endif //BESTWALLET_OUTPUTINFODIALOG_H
