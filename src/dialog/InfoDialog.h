// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_INFODIALOG_H
#define BESTWALLET_INFODIALOG_H

#include <QDialog>

#include "components.h"

namespace Ui {
    class InfoDialog;
}

class InfoDialog : public WindowModalDialog
{
    Q_OBJECT

public:
    explicit InfoDialog(QWidget *parent, const QString &title, const QString &infoText);
    ~InfoDialog() override;

private:
    QScopedPointer<Ui::InfoDialog> ui;
};


#endif //BESTWALLET_INFODIALOG_H
