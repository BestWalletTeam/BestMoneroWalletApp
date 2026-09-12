// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_MULTILINEINPUTDIALOG_H
#define BESTWALLET_MULTILINEINPUTDIALOG_H

#include <QDialog>

#include "components.h"

namespace Ui {
    class MultiLineInputDialog;
}

class MultiLineInputDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit MultiLineInputDialog(QWidget *parent, const QString &title, const QString &label, const QStringList &defaultList);
    ~MultiLineInputDialog() override;

    QStringList getList();

private:
    QScopedPointer<Ui::MultiLineInputDialog> ui;
};


#endif //BESTWALLET_MULTILINEINPUTDIALOG_H
