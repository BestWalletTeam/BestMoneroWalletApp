// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_VIEWONLYDIALOG_H
#define BESTWALLET_VIEWONLYDIALOG_H

#include <QDialog>

#include "components.h"
#include "libwalletqt/Wallet.h"

namespace Ui {
    class ViewOnlyDialog;
}

class ViewOnlyDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit ViewOnlyDialog(Wallet *wallet, QWidget *parent = nullptr);
    ~ViewOnlyDialog() override;

private slots:
    void onWriteViewOnlyWallet();

private:
    QString toString();
    QString toJsonString();
    void copyToClipboard();

    QScopedPointer<Ui::ViewOnlyDialog> ui;
    Wallet *m_wallet;
};


#endif //BESTWALLET_KEYSDIALOG_H
