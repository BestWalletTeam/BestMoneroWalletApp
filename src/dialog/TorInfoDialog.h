// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_TORINFODIALOG_H
#define BESTWALLET_TORINFODIALOG_H

#include <QDialog>

namespace Ui {
    class TorInfoDialog;
}

class TorInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit TorInfoDialog(QWidget *parent = nullptr);
    ~TorInfoDialog() override;

public slots:
    void onLogsUpdated();

private slots:
    void onConnectionStatusChanged(bool connected);
    void onStatusChanged(const QString &msg = "");

private:
    QScopedPointer<Ui::TorInfoDialog> ui;
};


#endif //BESTWALLET_TORINFODIALOG_H
