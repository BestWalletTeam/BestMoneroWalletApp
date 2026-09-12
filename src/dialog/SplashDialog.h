// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_SPLASHDIALOG_H
#define BESTWALLET_SPLASHDIALOG_H

#include <QDialog>

#include "components.h"

namespace Ui {
    class SplashDialog;
}

class SplashDialog : public WindowModalDialog
{
Q_OBJECT

public:
    explicit SplashDialog(QWidget *parent = nullptr);
    ~SplashDialog() override;

    void setMessage(const QString &message);
    void setIcon(const QPixmap &icon);

private:
    QScopedPointer<Ui::SplashDialog> ui;
};

#endif //BESTWALLET_SPLASHDIALOG_H
