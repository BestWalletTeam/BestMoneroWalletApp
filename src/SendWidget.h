// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_SENDWIDGET_H
#define BESTWALLET_SENDWIDGET_H

#include <QWidget>

class Wallet;

namespace Ui {
    class SendWidget;
}

class SendWidget : public QWidget
{
Q_OBJECT

public:
    explicit SendWidget(Wallet *wallet, QWidget *parent = nullptr);
    void fill(const QString &address, const QString &description, double amount = 0, bool overrideDescription = true);
    void fill(double amount);
    void clearFields();
    void payToMany();
    ~SendWidget() override;

public slots:
    void skinChanged();
    void scanClicked();
    void sendClicked();
    void clearClicked();
    void aliasClicked();
    void btnMaxClicked();
    void amountEdited(const QString &text);
    void addressEdited();
    void currencyComboChanged(int index);
    void fillAddress(const QString &address);
    void updateConversionLabel();
    void onOpenAliasResolved(const QString &openAlias, const QString &address, bool dnssecValid);
    void onPreferredFiatCurrencyChanged();
    void setWebsocketEnabled(bool enabled);

    void setManualFeeSelectionEnabled(bool enabled);
    void setSubtractFeeFromAmountEnabled(bool enabled);

    void disableSendButton();
    void enableSendButton();

    void disallowSending();

private slots:
    void onDataFromQR(const QString &data);

private:
    void setupComboBox();
    // Writes the amount field without the write looking like the user typing
    // (lineAmount is watched with textChanged, not textEdited).
    void setAmountText(const QString &text);
    double amountDouble();
    bool keyImageSync(bool sendAll, quint64 amount);

    quint64 amount();
    double conversionAmount();

    // "Max" sweeps the account rather than sending the figure in the field.
    bool m_sendAll = false;
    bool m_fillingAmount = false;

    QScopedPointer<Ui::SendWidget> ui;
    Wallet *m_wallet;
    bool m_disallowSending = false;
};

#endif // BESTWALLET_SENDWIDGET_H
