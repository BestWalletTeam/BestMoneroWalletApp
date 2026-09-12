// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_WIZARDMENU_H
#define BESTWALLET_WIZARDMENU_H

#include <QPixmap>
#include <QWizardPage>

class WizardFields;
class WalletKeysFilesModel;

namespace Ui {
    class PageMenu;
}

class PageMenu : public QWizardPage
{
    Q_OBJECT

public:
    explicit PageMenu(WizardFields *fields, WalletKeysFilesModel *wallets, QWidget *parent = nullptr);
    void initializePage() override;
    bool validatePage() override;
    int nextId() const override;

public:
    QPixmap brandMark(int size, qreal dpr) const;
    // Re-renders the mark for the active skin. Called on construction and
    // whenever the theme changes while the wizard is open.
    void applyBrandMark();

private:
    Ui::PageMenu *ui;
    WalletKeysFilesModel *m_walletKeysFilesModel;
    WizardFields *m_fields;
};

#endif //BESTWALLET_WIZARDMENU_H
