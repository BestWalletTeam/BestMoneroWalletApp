// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_TXIMPORTDIALOG_H
#define BESTWALLET_TXIMPORTDIALOG_H

#include <QDialog>

#include "components.h"
#include "utils/daemonrpc.h"
#include "libwalletqt/Wallet.h"

namespace Ui {
    class TxImportDialog;
}

class TxImportDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit TxImportDialog(QWidget *parent, Wallet *wallet);
    ~TxImportDialog() override;

private slots:
    void onImport();

private:
    QScopedPointer<Ui::TxImportDialog> ui;
    Wallet *m_wallet;
};


#endif //BESTWALLET_TXIMPORTDIALOG_H
