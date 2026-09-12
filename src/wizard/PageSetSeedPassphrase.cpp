// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include "PageSetSeedPassphrase.h"
#include "ui_PageSetSeedPassphrase.h"
#include "WalletWizard.h"

#include <QCheckBox>

PageSetSeedPassphrase::PageSetSeedPassphrase(WizardFields *fields, QWidget *parent)
    : QWizardPage(parent)
    , ui(new Ui::PageSetSeedPassphrase)
    , m_fields(fields)
{
    ui->setupUi(this);

    this->setTitle("Seed Passphrase");

    connect(ui->check_hidePassphrase, &QCheckBox::clicked, [this](bool checked){
        ui->linePassphrase->setEchoMode(checked ? QLineEdit::Password : QLineEdit::Normal);
    });
}

void PageSetSeedPassphrase::initializePage() {
    ui->linePassphrase->setText("");
    ui->check_hidePassphrase->setChecked(true);
    ui->linePassphrase->setEchoMode(QLineEdit::Password);
}

bool PageSetSeedPassphrase::validatePage() {
    m_fields->seedOffsetPassphrase = ui->linePassphrase->text();
    return true;
}

int PageSetSeedPassphrase::nextId() const {
    if (m_fields->showSetSubaddressLookaheadPage) {
        return WalletWizard::Page_SetSubaddressLookahead;
    }

    return WalletWizard::Page_WalletFile;
}