// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_PASSWORD_H
#define BESTWALLET_PASSWORD_H

#include <QWizardPage>

class WizardFields;

namespace Ui {
    class PageSetPassword;
}

class PageSetPassword : public QWizardPage
{
Q_OBJECT

public:
    explicit PageSetPassword(WizardFields *fields, QWidget *parent = nullptr);
    void initializePage() override;
    bool validatePage() override;
    int nextId() const override;
    bool isComplete() const override;

signals:
    void createWallet();

private:
    Ui::PageSetPassword *ui;

    WizardFields *m_fields;
};


#endif //BESTWALLET_PASSWORD_H
