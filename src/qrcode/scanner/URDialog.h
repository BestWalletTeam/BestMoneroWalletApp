// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_URDIALOG_H
#define BESTWALLET_URDIALOG_H

#include "components.h"

namespace Ui {
    class URDialog;
}

class URDialog : public WindowModalDialog
{
    Q_OBJECT

public:
    explicit URDialog(QWidget *parent);
    ~URDialog() override;

private:
    QScopedPointer<Ui::URDialog> ui;
};


#endif //BESTWALLET_URDIALOG_H
