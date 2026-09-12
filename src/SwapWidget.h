// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_SWAPWIDGET_H
#define BESTWALLET_SWAPWIDGET_H

#include <QTimer>
#include <QWidget>

#include "api/changenow.h"

class QrCode;
class QLineEdit;
class Wallet;

namespace Ui {
    class SwapWidget;
}

// Swaps between XMR and other assets through ChangeNOW.
//
// Two faces in a stacked widget: the quote form, and the order panel shown once
// a swap exists. An order outlives the window, so its id is kept in the config
// and picked back up on the next start.
class SwapWidget : public QWidget
{
Q_OBJECT

public:
    explicit SwapWidget(Wallet *wallet, QWidget *parent = nullptr);
    ~SwapWidget() override;

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

public slots:
    void skinChanged();
    void onBalanceUpdated();

signals:
    // Asks the window to move to the Send page with the deposit filled in.
    void sendFromWallet(const QString &address, const QString &description, double amount);

    void setStatusText(const QString &text, bool override, int timeout);

private slots:
    void onAmountEdited();
    void onReceiveEdited();
    void onReverseClicked();
    void onMaxClicked();
    void onCreateClicked();
    void onNewSwapClicked();

private:
    // The asset on the far side of the swap; the other side is always XMR.
    ChangeNowApi::Asset counterAsset() const;
    QString counterLabel() const;
    QString counterNetworkName() const;
    bool counterHasMemo() const;

    ChangeNowApi::Asset fromAsset() const;
    ChangeNowApi::Asset toAsset() const;
    QString fromLabel() const;
    QString toLabel() const;

    // True when the wallet is the one paying, i.e. XMR is being sold.
    bool sendingFromWallet() const { return m_sellingXmr; }

    double enteredAmount() const;

    // What the user typed into the receive field.
    double receiveAmount() const;

    // Writes an amount field without the write being taken for typing.
    void setAmountQuietly(QLineEdit *field, const QString &text);

    // The parts of a quote that do not depend on which field asked for it.
    void applyQuote(const ChangeNowApi::Quote &quote);

    // The dollar value under each amount.
    void updateUsd();

    void setupComboBoxes();
    void applyDirection();

    // Debounced, so a typed amount is not one request per keystroke.
    void scheduleQuote();
    void requestQuote();

    // Replaces the page with the maintenance notice.
    void showUnavailable();

    void showQuoteError(const QString &error);
    void clearQuoteError();
    void updateCreateButton();

    // The amount-specific half of validation, shown against the field itself.
    QString amountError() const;
    void updateAmountNotice();
    void updateBalance();

    // Why the form cannot be submitted, or empty when it can.
    QString validationError() const;

    void showOrder(const ChangeNowApi::Order &order);
    void refreshOrder();
    void updateOrderView();
    void updateQrCode();
    void restoreOrder();
    void clearOrder();

    QString walletAddress() const;

    //! Mints a new subaddress and returns it, or empty with an error already
    //! shown. `label` is what it will be called in the Receive list.
    QString generateAddress(const QString &label);

    static QString formatCoinAmount(double amount, const QString &coin);

    // Full precision, for a figure that is acted on rather than read.
    static QString exactAmount(double amount);

    // "≈ $1,535.16", "< $0.01", or empty for nothing to show.
    static QString formatUsd(double usd);

    QScopedPointer<Ui::SwapWidget> ui;
    Wallet *m_wallet;
    ChangeNowApi *m_api;

    QTimer m_quoteDebounce;
    QTimer m_quoteRefresh;
    QTimer m_statusPoll;

    // Index into the offered-asset table for the non-XMR side.
    int m_assetIndex = 0;
    bool m_sellingXmr = false;

    ChangeNowApi::Range m_range;
    bool m_haveRange = false;
    bool m_haveQuote = false;

    ChangeNowApi::Order m_order;

    // The tag sent with the order, kept so the panel can check the exchange
    // registered it.
    QString m_requestedMemo;

    // The exchange's duration estimate, kept from the quote that preceded the
    // order because a looked-up order does not carry it.
    QString m_speedForecast;

    // Last status announced to the status bar, so it is reported once rather
    // than on every poll.
    QString m_announcedStatus;

    int m_statusPolls = 0;
    bool m_creating = false;
    bool m_unavailable = false;

    // Set while code writes the amount field, so the textChanged it causes does
    // not look like the user typing.
    bool m_fillingAmount = false;

    // Which field the user last typed into, and so which is worked out from the
    // other.
    enum class QuoteMode { FromSend, FromReceive };
    QuoteMode m_quoteMode = QuoteMode::FromSend;

    // The latest rate for the current pair, used to seed a reverse solve.
    double m_lastRate = 0;
};

#endif //BESTWALLET_SWAPWIDGET_H
