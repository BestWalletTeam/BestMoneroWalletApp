// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include "SwapWidget.h"
#include <QLocale>
#include "utils/AppData.h"
#include "ui_SwapWidget.h"

#include <QDebug>
#include <QFile>
#include <QHash>
#include <QIcon>
#include <QPixmap>
#include <QMessageBox>
#include <QPainter>
#include <QStyle>

#include "constants.h"
#include "dialog/QrCodeDialog.h"
#include "libwalletqt/Subaddress.h"
#include "libwalletqt/Wallet.h"
#include "libwalletqt/WalletManager.h"
#include "qrcode/QrCode.h"
#include "widgets/SwapProgress.h"
#include "utils/config.h"
#include "utils/Utils.h"

namespace {
    const QString kCoinXMR = QStringLiteral("XMR");

    // Curated subset of the exchange's catalogue. Tokens are listed once per
    // chain rather than behind a second network selector: USDT on Tron and USDT
    // on Ethereum are different assets, and a deposit to the wrong one is lost.
    struct OfferedAsset {
        const char *ticker;
        const char *network;
        const char *label;        // as shown in the combo
        const char *networkName;  // named in the address field's placeholder
        bool hasMemo;             // payout can carry a destination tag
    };

    const OfferedAsset kAssets[] = {
        {"btc",  "btc",  "BTC",           "Bitcoin",            false},
        {"ltc",  "ltc",  "LTC",           "Litecoin",           false},
        {"eth",  "eth",  "ETH",           "Ethereum",           false},
        {"usdt", "trx",  "USDT (TRC20)",  "Tron (TRC20)",       false},
        {"usdt", "eth",  "USDT (ERC20)",  "Ethereum (ERC20)",   false},
        {"usdt", "bsc",  "USDT (BSC)",    "BNB Smart Chain",    false},
        {"usdt", "sol",  "USDT (Solana)", "Solana",             false},
        {"usdc", "eth",  "USDC (ERC20)",  "Ethereum (ERC20)",   false},
        {"usdc", "bsc",  "USDC (BSC)",    "BNB Smart Chain",    false},
        {"usdc", "sol",  "USDC (Solana)", "Solana",             false},
        {"sol",  "sol",  "SOL",           "Solana",             false},
        {"xrp",  "xrp",  "XRP",           "XRP Ledger",         true},
        {"doge", "doge", "DOGE",          "Dogecoin",           false},
        {"ada",  "ada",  "ADA",           "Cardano",            false},
        {"trx",  "trx",  "TRX",           "Tron",               false},
    };

    constexpr int kAssetCount = int(sizeof(kAssets) / sizeof(kAssets[0]));

    constexpr int kQuoteDebounceMs = 500;
    constexpr int kQuoteRefreshMs = 30 * 1000;
    constexpr int kStatusPollMs = 15 * 1000;
    constexpr int kAmountDecimals = 8;

    const ChangeNowApi::Asset kXmr{QStringLiteral("xmr"), QStringLiteral("xmr"), false};

    // Held back from the balance offered to a swap: the deposit must be exact,
    // so the tx fee has to fit beside it rather than come out of it. A generous
    // reserve, since libwalletqt exposes no fee estimator here.
    constexpr quint64 kFeeReserve = 1000000000; // 0.001 XMR in atomic units

    // Bounds the status poll on an order that never reaches a terminal state.
    constexpr int kMaxStatusPolls = 4 * 60 * 4; // ~4 hours at 15s

    // A payment URI carrying the deposit amount, or empty for chains without a
    // widely honoured scheme. Tokens are excluded: EIP-681 support is patchy and
    // wants wei, and a URI no scanner recognises is worse than a bare address.
    QString depositUri(const QString &ticker, const QString &network,
                       const QString &address, const QString &amount) {
        const QString coin = ticker.toLower();
        if (!network.isEmpty() && network.toLower() != coin) {
            return {}; // a token, riding a chain that is not its own
        }

        static const QHash<QString, QString> schemes{
            {QStringLiteral("btc"),  QStringLiteral("bitcoin")},
            {QStringLiteral("ltc"),  QStringLiteral("litecoin")},
            {QStringLiteral("doge"), QStringLiteral("dogecoin")},
            {QStringLiteral("xmr"),  QStringLiteral("monero")},
        };

        const QString scheme = schemes.value(coin);
        if (scheme.isEmpty() || address.isEmpty() || amount.isEmpty()) {
            return {};
        }

        // Monero's URI spells the amount differently from the BIP-21 family.
        const QString key = coin == QLatin1String("xmr") ? QStringLiteral("tx_amount")
                                                         : QStringLiteral("amount");
        return QString("%1:%2?%3=%4").arg(scheme, address, key, amount);
    }

    // Bundled as a resource rather than fetched from the exchange's CDN, which
    // would leak which coins the user is looking at.
    //
    // Cached for the process: these are immutable brand marks, and rebuilding
    // one re-parses and re-rasterises the SVG at six sizes. applyDirection()
    // repopulates both combos, so an uncached call costs ~96 rasterisations
    // every time the pair changes -- and once during construction.
    QIcon coinIcon(const QString &ticker) {
        static QHash<QString, QIcon> cache;
        const QString key = ticker.toLower();
        const auto cached = cache.constFind(key);
        if (cached != cache.constEnd()) {
            return *cached;
        }

        QIcon source;

        // Monero wears the wallet's own mark, the same artwork the nav rail shows.
        if (ticker.compare(QLatin1String("xmr"), Qt::CaseInsensitive) == 0) {
            for (const char *size : {"32x32", "48x48", "64x64", "128x128"}) {
                const QPixmap pixmap{QString(":/assets/images/appicons/%1.png")
                                         .arg(QLatin1String(size))};
                if (!pixmap.isNull()) {
                    source.addPixmap(pixmap);
                }
            }
        }

        if (source.isNull()) {
            const QString path = QString(":/assets/images/coins/%1.svg").arg(key);
            if (!QFile::exists(path)) {
                cache.insert(key, {});
                return {};
            }
            source = QIcon{path};
        }

        // The XMR combo is disabled, and Qt desaturates icons on disabled
        // widgets. These are brand marks, so register the same artwork for both
        // modes to keep the colour.
        QIcon icon;
        for (int extent : {16, 18, 24, 32, 36, 48}) {
            const QPixmap pixmap = source.pixmap(extent, extent);
            if (pixmap.isNull()) {
                continue;
            }
            icon.addPixmap(pixmap, QIcon::Normal);
            icon.addPixmap(pixmap, QIcon::Disabled);
        }

        cache.insert(key, icon);
        return icon;
    }

    // Drives the "state" property the stylesheet selects on. Re-resolving a rule
    // is not free and this runs on every keystroke, so only do it on a change.
    void setWidgetState(QWidget *widget, const QString &state) {
        if (widget->property("state").toString() == state) {
            return;
        }
        widget->setProperty("state", state);
        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
    }

    // Names an asset from the order's own ticker and network, since the order
    // panel outlives the combo selection that created it.
    QString assetLabel(const QString &ticker, const QString &network) {
        for (const auto &asset : kAssets) {
            if (ticker.compare(QLatin1String(asset.ticker), Qt::CaseInsensitive) == 0
                && network.compare(QLatin1String(asset.network), Qt::CaseInsensitive) == 0) {
                return QString::fromLatin1(asset.label);
            }
        }
        // Not an offered pair: an older order, or a chain this table lacks.
        if (network.isEmpty() || ticker.compare(network, Qt::CaseInsensitive) == 0) {
            return ticker.toUpper();
        }
        return QString("%1 (%2)").arg(ticker.toUpper(), network.toUpper());
    }
}

SwapWidget::SwapWidget(Wallet *wallet, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SwapWidget)
    , m_wallet(wallet)
    , m_api(new ChangeNowApi(this))
{
    ui->setupUi(this);

    this->setupComboBoxes();

    m_quoteDebounce.setSingleShot(true);
    m_quoteDebounce.setInterval(kQuoteDebounceMs);
    connect(&m_quoteDebounce, &QTimer::timeout, this, &SwapWidget::requestQuote);

    m_quoteRefresh.setInterval(kQuoteRefreshMs);
    connect(&m_quoteRefresh, &QTimer::timeout, this, &SwapWidget::requestQuote);

    m_statusPoll.setInterval(kStatusPollMs);
    connect(&m_statusPoll, &QTimer::timeout, this, &SwapWidget::refreshOrder);

    connect(ui->lineAmountFrom, &QLineEdit::textChanged, this, &SwapWidget::onAmountEdited);
    connect(ui->lineAmountTo, &QLineEdit::textChanged, this, &SwapWidget::onReceiveEdited);
    connect(&appData()->prices, &Prices::cryptoPricesUpdated, this, &SwapWidget::updateUsd);
    connect(ui->btn_reverse, &QPushButton::clicked, this, &SwapWidget::onReverseClicked);
    connect(ui->btnMax, &QPushButton::clicked, this, &SwapWidget::onMaxClicked);
    connect(ui->label_balance, &ClickableLabel::clicked, this, &SwapWidget::onMaxClicked);
    connect(ui->btn_createSwap, &QPushButton::clicked, this, &SwapWidget::onCreateClicked);
    connect(ui->btn_newSwap, &QPushButton::clicked, this, &SwapWidget::onNewSwapClicked);

    connect(ui->btn_myAddress, &QPushButton::clicked, this, [this] {
        const QString address = this->generateAddress("Swap payout");
        if (!address.isEmpty()) {
            ui->lineDestination->setText(address);
            emit setStatusText("New address generated for this swap", true, 5000);
        }
    });
    connect(ui->btn_myRefundAddress, &QPushButton::clicked, this, [this] {
        const QString address = this->generateAddress("Swap refund");
        if (!address.isEmpty()) {
            ui->lineRefund->setText(address);
            emit setStatusText("New address generated for this refund", true, 5000);
        }
    });

    for (QLineEdit *field : {ui->lineDestination, ui->lineRefund, ui->lineMemo}) {
        connect(field, &QLineEdit::textChanged, this, [this] {
            this->clearQuoteError();
            this->updateCreateButton();
        });
    }

    connect(ui->btn_copyAddress, &QPushButton::clicked, this, [this] {
        Utils::copyToClipboard(m_order.payinAddress);
        emit setStatusText("Deposit address copied to clipboard", true, 3000);
    });
    connect(ui->btn_copyAmount, &QPushButton::clicked, this, [this] {
        // Full precision, not the eight places on display: a rounded figure
        // pasted into a send field would under-fund the deposit.
        Utils::copyToClipboard(SwapWidget::exactAmount(m_order.expectedFromAmount));
        emit setStatusText("Deposit amount copied to clipboard", true, 3000);
    });
    connect(ui->btn_copyOrderId, &QPushButton::clicked, this, [this] {
        Utils::copyToClipboard(m_order.id);
        emit setStatusText("Order ID copied to clipboard", true, 3000);
    });
    connect(ui->btn_refresh, &QPushButton::clicked, this, &SwapWidget::refreshOrder);

    connect(ui->btn_track, &QPushButton::clicked, this, [this] {
        if (!m_order.isValid()) {
            return;
        }
        Utils::externalLinkWarning(this, ChangeNowApi::trackingUrl(m_order.id));
    });

    connect(ui->btn_sendFromWallet, &QPushButton::clicked, this, [this] {
        if (m_order.payinAddress.isEmpty()) {
            return;
        }
        if (m_order.expectedFromAmount <= 0) {
            Utils::showError(this, "Swap amount unavailable",
                             "The deposit amount could not be read back from the exchange. "
                             "Refresh the swap, or copy the address and amount by hand.");
            return;
        }
        emit sendFromWallet(m_order.payinAddress,
                            QString("Swap %1 to %2 (%3)").arg(m_order.fromCurrency, m_order.toCurrency, m_order.id),
                            m_order.expectedFromAmount);
    });

    connect(ui->qrCode, &ClickableLabel::clicked, this, [this] {
        if (m_order.payinAddress.isEmpty()) {
            return;
        }
        const QString encoded = depositUri(m_order.fromCurrency, m_order.fromNetwork,
                                           m_order.payinAddress,
                                           SwapWidget::exactAmount(m_order.expectedFromAmount));
        QrCode qr{encoded.isEmpty() ? m_order.payinAddress : encoded,
                  QrCode::Version::AUTO, QrCode::ErrorCorrectionLevel::HIGH};
        QrCodeDialog dialog{this, &qr, "Deposit address"};
        dialog.exec();
    });

    connect(m_api, &ChangeNowApi::rangeReceived, this, [this](const ChangeNowApi::Range &range) {
        m_range = range;
        m_haveRange = true;

        // The range can arrive after the amount was typed, putting it out of
        // bounds, so re-evaluate rather than waiting for the next keystroke.
        this->updateCreateButton();

        // A reverse solve started before this arrived had no floor to stay
        // above and may have failed on it. With the minimum known it can be
        // retried now instead of at the next refresh, thirty seconds away.
        if (m_quoteMode == QuoteMode::FromReceive && !m_haveQuote && this->receiveAmount() > 0) {
            this->scheduleQuote();
        }
    });

    connect(m_api, &ChangeNowApi::rangeFailed, this, [this](const QString &error) {
        m_haveRange = false;
        this->updateAmountNotice();
        this->showQuoteError(QString("Could not load exchange limits: %1").arg(error));
    });

    connect(m_api, &ChangeNowApi::quoteReceived, this, [this](const ChangeNowApi::Quote &quote) {
        // A direct quote answers an edit of the send field. One still arriving
        // after the user has moved to the receive field is about a figure they
        // have stopped asking for.
        if (m_quoteMode != QuoteMode::FromSend) {
            return;
        }
        this->setAmountQuietly(ui->lineAmountTo, SwapWidget::formatCoinAmount(quote.toAmount, this->toLabel()));
        this->applyQuote(quote);
    });

    connect(m_api, &ChangeNowApi::reverseQuoteReceived, this,
            [this](double fromAmount, const ChangeNowApi::Quote &quote, bool belowMinimum) {
        if (m_quoteMode != QuoteMode::FromReceive) {
            return;
        }
        this->setAmountQuietly(ui->lineAmountFrom, SwapWidget::exactAmount(fromAmount));
        this->applyQuote(quote);
        if (belowMinimum) {
            // The deposit filled in is under what the exchange accepts, so the
            // quote belongs to the minimum rather than to this figure. The
            // notice under the send field says so and creating stays off.
            m_haveQuote = false;
            this->updateCreateButton();
        }
    });

    const auto quoteFailed = [this](QuoteMode mode, const QString &error) {
        if (m_quoteMode != mode) {
            return;
        }
        m_haveQuote = false;
        // The side being worked out, never the one being typed in.
        this->setAmountQuietly(mode == QuoteMode::FromSend ? ui->lineAmountTo : ui->lineAmountFrom, {});
        ui->label_rate->clear();
        this->showQuoteError(error);
        this->updateCreateButton();
        this->updateUsd();
    };
    connect(m_api, &ChangeNowApi::quoteFailed, this, [quoteFailed](const QString &error) {
        quoteFailed(QuoteMode::FromSend, error);
    });
    connect(m_api, &ChangeNowApi::reverseQuoteFailed, this, [quoteFailed](const QString &error) {
        quoteFailed(QuoteMode::FromReceive, error);
    });

    connect(m_api, &ChangeNowApi::orderCreated, this, [this](const ChangeNowApi::Order &order) {
        m_creating = false;
        qInfo() << "Swap created:" << order.id
                << order.fromCurrency << "->" << order.toCurrency
                << "deposit" << SwapWidget::exactAmount(order.expectedFromAmount)
                << "to" << order.payinAddress;
        this->showOrder(order);
        emit setStatusText("Swap created", true, 5000);
    });

    connect(m_api, &ChangeNowApi::orderFailed, this, [this](const QString &error) {
        m_creating = false;
        this->updateCreateButton();
        this->showQuoteError(error);
        Utils::showError(this, "Could not create the swap", error,
                         {"Check the receiving address is correct for the coin you are buying.",
                          "Check the amount is within the exchange's limits.",
                          "If the wallet is behind Tor or a proxy, check the connection is up."});
    });

    connect(m_api, &ChangeNowApi::orderUpdated, this, [this](const ChangeNowApi::Order &order) {
        // A reply for an order the user has since replaced.
        if (order.id != m_order.id) {
            return;
        }
        ui->label_orderError->clear();
        ui->label_orderError->hide();
        this->showOrder(order);
    });

    connect(m_api, &ChangeNowApi::orderUpdateFailed, this, [this](const QString &error) {
        // Non-fatal: the order and the address on screen are still good, so keep
        // the panel and carry on polling.
        ui->label_orderError->setText(QString("Could not refresh the swap status: %1").arg(error));
        ui->label_orderError->show();
    });

    connect(m_api, &ChangeNowApi::keyRequired, this, [this] {
        m_creating = false;

        // Every timer, not just the poll: a refused key does not fix itself.
        m_statusPoll.stop();
        m_quoteDebounce.stop();
        m_quoteRefresh.stop();
        this->updateCreateButton();

        const bool haveKey = ChangeNowApi::hasApiKey();
        if (haveKey) {
            qWarning() << "ChangeNOW refused the API key;"
                       << (ChangeNowApi::usingBundledKey() ? "bundled key" : "key from config");
        } else {
            qWarning() << "Swaps disabled: this build has no ChangeNOW API key";
        }

        // An order in flight outranks the notice: its id is the user's only
        // handle on money the exchange is already holding.
        if (ui->stack->currentWidget() == ui->page_order) {
            ui->label_orderError->setText(haveKey
                ? "ChangeNOW is not accepting requests from this wallet right now, so this swap "
                  "cannot be refreshed. It is still running -- follow it with the order ID below."
                : "This build of Best Wallet cannot contact ChangeNOW, so this swap cannot be "
                  "refreshed. Follow it on ChangeNOW with the order ID below.");
            ui->label_orderError->show();
            return;
        }

        this->showUnavailable();
    });

    connect(ui->btn_getLatest, &QPushButton::clicked, this, [this] {
        Utils::externalLinkWarning(this, constants::websiteUrl);
    });

    connect(ui->btn_retryAvailability, &QPushButton::clicked, this, [this] {
        // The form re-requests the range, which re-tests the credential.
        m_unavailable = false;
        ui->stack->setCurrentWidget(ui->page_quote);
        this->applyDirection();
    });

    if (m_wallet) {
        connect(m_wallet, &Wallet::balanceUpdated, this, &SwapWidget::onBalanceUpdated);
    }

    ui->orderProgress->setLightTheme(
        conf()->get(Config::skin).toString() == constants::skinNativeWhite);

    ui->label_error->hide();
    ui->label_orderError->hide();

    this->applyDirection();
    this->updateBalance();
    this->restoreOrder();
}

void SwapWidget::setupComboBoxes() {
    // applyDirection rebuilds both: the XMR side gets a single fixed entry, the
    // other gets the asset list.
    for (QComboBox *combo : {ui->comboFrom, ui->comboTo}) {
        combo->setIconSize(QSize(18, 18));
        // Never clip the network out of an entry like "USDT (Solana)".
        combo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

        connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, combo](int index) {
            // The fixed XMR side has nothing to choose.
            if (index < 0 || combo->count() <= 1) {
                return;
            }
            const int asset = combo->itemData(index).toInt();
            if (asset == m_assetIndex) {
                return;
            }
            m_assetIndex = asset;
            this->applyDirection();
        });
    }
}

ChangeNowApi::Asset SwapWidget::counterAsset() const {
    const OfferedAsset &a = kAssets[qBound(0, m_assetIndex, kAssetCount - 1)];
    return {QString::fromLatin1(a.ticker), QString::fromLatin1(a.network), a.hasMemo};
}

QString SwapWidget::counterLabel() const {
    return QString::fromLatin1(kAssets[qBound(0, m_assetIndex, kAssetCount - 1)].label);
}

QString SwapWidget::counterNetworkName() const {
    return QString::fromLatin1(kAssets[qBound(0, m_assetIndex, kAssetCount - 1)].networkName);
}

bool SwapWidget::counterHasMemo() const {
    return kAssets[qBound(0, m_assetIndex, kAssetCount - 1)].hasMemo;
}

ChangeNowApi::Asset SwapWidget::fromAsset() const {
    return m_sellingXmr ? kXmr : this->counterAsset();
}

ChangeNowApi::Asset SwapWidget::toAsset() const {
    return m_sellingXmr ? this->counterAsset() : kXmr;
}

QString SwapWidget::fromLabel() const {
    return m_sellingXmr ? kCoinXMR : this->counterLabel();
}

QString SwapWidget::toLabel() const {
    return m_sellingXmr ? this->counterLabel() : kCoinXMR;
}

void SwapWidget::applyDirection() {
    const bool payingFromWallet = this->sendingFromWallet();

    // Repopulating emits currentIndexChanged, which would re-enter here.
    QSignalBlocker blockFrom{ui->comboFrom};
    QSignalBlocker blockTo{ui->comboTo};

    QComboBox *xmrSide = payingFromWallet ? ui->comboFrom : ui->comboTo;
    QComboBox *assetSide = payingFromWallet ? ui->comboTo : ui->comboFrom;

    xmrSide->clear();
    xmrSide->addItem(coinIcon(kXmr.ticker), kCoinXMR);
    xmrSide->setEnabled(false);

    assetSide->clear();
    for (int i = 0; i < kAssetCount; ++i) {
        assetSide->addItem(coinIcon(QString::fromLatin1(kAssets[i].ticker)),
                           QString::fromLatin1(kAssets[i].label), i);
    }
    assetSide->setCurrentIndex(m_assetIndex);
    assetSide->setEnabled(true);

    const QString from = this->fromLabel();
    const QString to = this->toLabel();

    // Balance and Max only mean anything for the coin this wallet holds.
    ui->label_balance->setVisible(payingFromWallet);
    ui->btnMax->setVisible(payingFromWallet);

    // The wallet only has an address for the XMR side.
    ui->btn_myAddress->setVisible(!payingFromWallet);
    ui->btn_myRefundAddress->setVisible(payingFromWallet);

    ui->label_destinationCaption->setText(QString("Receiving address · %1").arg(to));
    ui->lineDestination->setPlaceholderText(
        payingFromWallet ? QString("Your %1 address on %2").arg(to, this->counterNetworkName())
                         : QString("Your %1 address, where the swap pays out").arg(to));
    ui->label_refundCaption->setText(QString("Refund address · %1").arg(from));
    ui->lineRefund->setPlaceholderText(
        payingFromWallet ? QString("Your %1 address, where the deposit returns if the swap fails").arg(from)
                         : QString("Your %1 address on %2, where the deposit returns if the swap fails")
                               .arg(from, this->counterNetworkName()));

    const bool wantsMemo = payingFromWallet && this->counterHasMemo();
    ui->label_memoCaption->setVisible(wantsMemo);
    ui->lineMemo->setVisible(wantsMemo);
    ui->label_memoHint->setVisible(wantsMemo);
    ui->label_memoCaption->setText(QString("Destination tag · %1").arg(to));
    if (!wantsMemo) {
        ui->lineMemo->clear();
    }

    // The right address on the wrong chain loses the funds, so restate the chain
    // beside the field rather than leaving it implied by the combo entry.
    ui->label_networkNotice->setText(
        QString("%1 is sent over %2. An address on any other chain will lose the funds.")
            .arg(to, payingFromWallet ? this->counterNetworkName() : QString("Monero")));
    ui->label_networkNotice->setVisible(payingFromWallet);

    // With a non-XMR deposit the wallet has no address to offer, so the field
    // has to say it wants one on the chain the deposit comes from.
    ui->label_refundHint->setVisible(!payingFromWallet);
    ui->label_refundHint->setText(
        QString("You send the %1 from a wallet or exchange of your own. Put an address from "
                "there here, so the deposit can be returned if the swap cannot complete.")
            .arg(from));

    // Held over from the old pair, so now for the wrong chain.
    ui->lineDestination->clear();
    ui->lineRefund->clear();

    // A refund travels back on the deposit's chain, so when the wallet is the
    // depositor its own address is nearly always the answer. Still editable.
    //
    // Not generated here: applyDirection runs on every asset change, and
    // minting a subaddress each time someone browses the list would leave a
    // trail of unused ones. The button does that on request.
    if (payingFromWallet) {
        ui->lineRefund->setText(this->walletAddress());
    }

    m_haveRange = false;
    m_haveQuote = false;
    m_quoteMode = QuoteMode::FromSend;
    m_lastRate = 0;
    ui->label_rate->clear();
    this->setAmountQuietly(ui->lineAmountTo, {});
    this->clearQuoteError();

    this->updateBalance();
    m_api->requestRange(this->fromAsset(), this->toAsset());
    this->scheduleQuote();
    this->updateCreateButton();
}

void SwapWidget::onReverseClicked() {
    m_sellingXmr = !m_sellingXmr;
    this->applyDirection();
}

double SwapWidget::enteredAmount() const {
    return ui->lineAmountFrom->text().trimmed().toDouble();
}

void SwapWidget::onAmountEdited() {
    if (m_fillingAmount) {
        return;
    }
    m_quoteMode = QuoteMode::FromSend;
    this->scheduleQuote();
    this->updateCreateButton();
}

void SwapWidget::onReceiveEdited() {
    if (m_fillingAmount) {
        return;
    }
    // Typing what should arrive makes the deposit the figure to work out.
    m_quoteMode = QuoteMode::FromReceive;
    this->scheduleQuote();
    this->updateCreateButton();
}

double SwapWidget::receiveAmount() const {
    return ui->lineAmountTo->text().trimmed().toDouble();
}

void SwapWidget::setAmountQuietly(QLineEdit *field, const QString &text) {
    // Either amount field re-quotes when edited, so a write from code has to be
    // told apart from typing, or each fill would set off the other side.
    m_fillingAmount = true;
    field->setText(text);
    m_fillingAmount = false;
}

void SwapWidget::scheduleQuote() {
    m_haveQuote = false;

    const bool fromSend = m_quoteMode == QuoteMode::FromSend;
    const double amount = fromSend ? this->enteredAmount() : this->receiveAmount();
    if (amount <= 0) {
        m_quoteDebounce.stop();
        m_quoteRefresh.stop();
        this->setAmountQuietly(fromSend ? ui->lineAmountTo : ui->lineAmountFrom, {});
        ui->label_rate->clear();
        this->updateUsd();
        return;
    }

    // The typed side's dollar value is local arithmetic, so it follows the
    // keystroke; the other side's waits for the quote.
    this->updateUsd();
    m_quoteDebounce.start();
}

void SwapWidget::requestQuote() {
    if (m_quoteMode == QuoteMode::FromSend) {
        const double amount = this->enteredAmount();
        if (amount <= 0) {
            m_quoteRefresh.stop();
            return;
        }
        m_api->requestQuote(this->fromAsset(), this->toAsset(), amount);
    } else {
        const double target = this->receiveAmount();
        if (target <= 0) {
            m_quoteRefresh.stop();
            return;
        }
        // Seeded from the last rate seen for this pair so the solve starts
        // close; with none yet, the target itself is a usable first guess.
        const double seed = m_lastRate > 0 ? target / m_lastRate : target;
        m_api->requestReverseQuote(this->fromAsset(), this->toAsset(), target, seed,
                                   m_haveRange ? m_range.minAmount : 0);
    }

    if (!m_quoteRefresh.isActive()) {
        m_quoteRefresh.start();
    }
}

void SwapWidget::applyQuote(const ChangeNowApi::Quote &quote) {
    m_haveQuote = true;
    m_lastRate = quote.rate;
    if (!quote.speedForecast.isEmpty()) {
        m_speedForecast = quote.speedForecast;
    }

    QString rate = QString("1 %1 ≈ %2 %3")
                       .arg(this->fromLabel(),
                            SwapWidget::formatCoinAmount(quote.rate, this->toLabel()),
                            this->toLabel());
    if (quote.withdrawalFee > 0) {
        // Stated because it explains the minimum, which varies several-fold
        // between networks of the same coin.
        rate += QString("  ·  includes %1 %2 network fee")
                    .arg(SwapWidget::formatCoinAmount(quote.withdrawalFee, this->toLabel()),
                         this->toLabel());
    }
    ui->label_rate->setText(rate);

    if (!quote.warning.isEmpty()) {
        this->showQuoteError(quote.warning);
    } else {
        this->clearQuoteError();
    }

    this->updateCreateButton();
    this->updateUsd();
}

void SwapWidget::updateUsd() {
    // From the wallet's own price feed rather than a request of its own, so it
    // costs nothing per keystroke and follows the user's network settings. A
    // coin the feed does not carry shows nothing rather than a guess.
    const auto usd = [](const QString &ticker, double amount) -> QString {
        if (amount <= 0) {
            return {};
        }
        const QString symbol = ticker.toUpper();
        if (!appData()->prices.canConvert(symbol, "USD")) {
            return {};
        }
        return SwapWidget::formatUsd(appData()->prices.convert(symbol, "USD", amount));
    };

    const QString from = usd(this->fromAsset().ticker, this->enteredAmount());
    const QString to = usd(this->toAsset().ticker, this->receiveAmount());
    ui->label_fromUsd->setText(from);
    ui->label_fromUsd->setVisible(!from.isEmpty());
    ui->label_toUsd->setText(to);
    ui->label_toUsd->setVisible(!to.isEmpty());
}

QString SwapWidget::formatUsd(double usd) {
    if (usd <= 0) {
        return {};
    }
    if (usd < 0.01) {
        return QStringLiteral("< $0.01");
    }
    return QStringLiteral("≈ $%1").arg(QLocale().toString(usd, 'f', 2));
}

void SwapWidget::onMaxClicked() {
    if (!this->sendingFromWallet() || !m_wallet) {
        return;
    }

    // Unlocked rather than total, less the fee reserve; see kFeeReserve.
    const quint64 spendable = m_wallet->unlockedBalance();
    const quint64 swappable = spendable > kFeeReserve ? spendable - kFeeReserve : 0;

    m_quoteMode = QuoteMode::FromSend;
    this->setAmountQuietly(ui->lineAmountFrom, WalletManager::displayAmount(swappable, false));

    this->scheduleQuote();
    this->updateCreateButton();
}

void SwapWidget::updateBalance() {
    if (!this->sendingFromWallet() || !m_wallet) {
        ui->label_balance->clear();
        return;
    }

    ui->label_balance->setText(QString("Balance: %1 XMR")
                                   .arg(WalletManager::displayAmount(m_wallet->unlockedBalance(), false)));
}

void SwapWidget::onBalanceUpdated() {
    this->updateBalance();
    // A balance that has just dropped can invalidate an amount already typed.
    this->updateCreateButton();
}

QString SwapWidget::walletAddress() const {
    if (!m_wallet) {
        return {};
    }
    return m_wallet->address(m_wallet->currentSubaddressAccount(), 0);
}

QString SwapWidget::generateAddress(const QString &label) {
    if (!m_wallet || !m_wallet->subaddress()) {
        return {};
    }

    // The primary address is the one identity the wallet cannot rotate. Handing
    // it to an exchange ties every swap to the same string, and to whatever
    // else it has been used for, when a fresh subaddress costs nothing.
    if (!m_wallet->subaddress()->addRow(label)) {
        Utils::showError(this, "Could not generate an address",
                         m_wallet->subaddress()->getError(),
                         {"Enter the address by hand, or copy one from the Receive page."});
        return {};
    }

    const qsizetype count = m_wallet->subaddress()->count();
    if (count <= 0) {
        return {};
    }
    return m_wallet->subaddress()->getRow(count - 1).address;
}

QString SwapWidget::validationError() const {
    const QString from = this->fromLabel();

    if (ui->lineAmountFrom->text().trimmed().isEmpty()) {
        return "Enter an amount to swap.";
    }
    if (this->enteredAmount() <= 0) {
        return "Enter an amount greater than zero.";
    }

    const QString amountProblem = this->amountError();
    if (!amountProblem.isEmpty()) {
        return amountProblem;
    }

    const QString destination = ui->lineDestination->text().trimmed();
    if (destination.isEmpty()) {
        return QString("Enter the %1 address the swap should pay out to.").arg(this->toLabel());
    }

    // Only Monero addresses can be checked here; the exchange rejects a bad one
    // on any other chain at order creation.
    if (!m_sellingXmr && !WalletManager::addressValid(destination, constants::networkType)) {
        return "The receiving address is not a valid Monero address.";
    }

    const QString refund = ui->lineRefund->text().trimmed();
    if (refund.isEmpty()) {
        return QString("Enter a %1 address for refunds. The exchange returns your deposit "
                       "there if the swap cannot complete.").arg(from);
    }
    if (m_sellingXmr && !WalletManager::addressValid(refund, constants::networkType)) {
        return "The refund address is not a valid Monero address.";
    }

    // A Monero address on a non-Monero chain is certainly wrong, and saying so
    // here names the field the exchange's own error would not.
    if (!m_sellingXmr && WalletManager::addressValid(refund, constants::networkType)) {
        return QString("That is a Monero address, but the refund address must be a %1 address. "
                       "Your deposit is in %1, so a refund can only go back to %1.").arg(from);
    }
    if (m_sellingXmr && WalletManager::addressValid(destination, constants::networkType)) {
        return QString("That is a Monero address, but the receiving address must be a %1 address "
                       "on %2.").arg(this->toLabel(), this->counterNetworkName());
    }

    if (!m_haveQuote) {
        return "Waiting for a rate from the exchange.";
    }

    return {};
}

QString SwapWidget::amountError() const {
    const double amount = this->enteredAmount();
    if (amount <= 0) {
        // Nothing typed yet is not a rejection.
        return {};
    }

    const QString from = this->fromLabel();

    if (m_haveRange && amount < m_range.minAmount) {
        return QString("The minimum the exchange accepts is %1 %2.")
            .arg(SwapWidget::formatCoinAmount(m_range.minAmount, from), from);
    }
    if (m_haveRange && m_range.hasMax && amount > m_range.maxAmount) {
        return QString("The maximum the exchange accepts is %1 %2.")
            .arg(SwapWidget::formatCoinAmount(m_range.maxAmount, from), from);
    }
    if (this->sendingFromWallet() && m_wallet) {
        const quint64 spendable = m_wallet->unlockedBalance();
        const quint64 requested = WalletManager::amountFromDouble(amount);

        if (requested > spendable) {
            return QString("Not enough spendable balance. You have %1 XMR.")
                .arg(WalletManager::displayAmount(spendable, false));
        }
        if (requested + kFeeReserve > spendable) {
            return QString("Leave room for the transaction fee. The most you can swap is %1 XMR.")
                .arg(WalletManager::displayAmount(spendable > kFeeReserve ? spendable - kFeeReserve : 0, false));
        }
    }

    return {};
}

void SwapWidget::updateAmountNotice() {
    // Under the send field, since the limits are denominated in the coin sent.
    const QString problem = this->amountError();
    const QString coin = this->fromLabel();

    QString text = problem;
    if (text.isEmpty() && m_haveRange) {
        text = QString("Minimum %1 %2")
                   .arg(SwapWidget::formatCoinAmount(m_range.minAmount, coin), coin);
        if (m_range.hasMax) {
            text += QString(" · Maximum %1 %2")
                        .arg(SwapWidget::formatCoinAmount(m_range.maxAmount, coin), coin);
        }
    }

    ui->label_amountNotice->setText(text);
    ui->label_amountNotice->setVisible(!text.isEmpty());

    setWidgetState(ui->label_amountNotice, problem.isEmpty() ? "info" : "invalid");
    setWidgetState(ui->lineAmountFrom, problem.isEmpty() ? "" : "invalid");
}

void SwapWidget::updateCreateButton() {
    this->updateAmountNotice();

    const QString problem = this->validationError();
    ui->btn_createSwap->setEnabled(problem.isEmpty() && !m_creating);
    ui->btn_createSwap->setText(m_creating ? "Creating swap…" : "Create swap");
    ui->btn_createSwap->setToolTip(problem);
}

void SwapWidget::showUnavailable() {
    // A whole-page state rather than an inline error: the credential belongs to
    // the build, so nothing on the form can be corrected to fix it.
    m_unavailable = true;
    if (!ChangeNowApi::hasApiKey()) {
        ui->label_unavailableTitle->setText("Swaps aren't included in this build");
        ui->label_unavailableBody->setText(
            "This copy of Best Wallet was built without an exchange key, so it cannot offer "
            "swaps. The official release includes them. Your wallet and funds are unaffected.");
        ui->btn_getLatest->setText("Get the official release");
        ui->btn_retryAvailability->hide();
    }
    ui->stack->setCurrentWidget(ui->page_unavailable);
}

void SwapWidget::showQuoteError(const QString &error) {
    if (error.isEmpty()) {
        this->clearQuoteError();
        return;
    }
    ui->label_error->setText(error);
    ui->label_error->show();
}

void SwapWidget::clearQuoteError() {
    ui->label_error->clear();
    ui->label_error->hide();
}

void SwapWidget::onCreateClicked() {
    const QString problem = this->validationError();
    if (!problem.isEmpty()) {
        this->showQuoteError(problem);
        return;
    }

    // Caught here rather than surfacing as a bare 401 from the API.
    if (!ChangeNowApi::hasApiKey()) {
        this->showQuoteError("Swaps are unavailable in this build: it was compiled without an "
                             "exchange API key.");
        return;
    }

    m_creating = true;
    m_requestedMemo = ui->lineMemo->isVisible() ? ui->lineMemo->text().trimmed() : QString();
    this->clearQuoteError();
    this->updateCreateButton();

    m_api->createOrder(this->fromAsset(), this->toAsset(), this->enteredAmount(),
                       ui->lineDestination->text().trimmed(),
                       ui->lineRefund->text().trimmed(),
                       ui->lineMemo->isVisible() ? ui->lineMemo->text().trimmed() : QString());
}

void SwapWidget::showOrder(const ChangeNowApi::Order &order) {
    const bool isNewOrder = m_order.id != order.id;

    // Merged, not replaced: a poll about progress must not wipe the details
    // established at creation.
    ChangeNowApi::Order merged = order;
    merged.completeFrom(m_order);
    m_order = merged;

    if (isNewOrder) {
        m_statusPolls = 0;
    }

    if (isNewOrder) {
        conf()->set(Config::swapOrderId, order.id);
    }

    ui->stack->setCurrentWidget(ui->page_order);

    // The form's timers are about a quote that is no longer on screen.
    m_quoteDebounce.stop();
    m_quoteRefresh.stop();

    this->updateOrderView();
    this->updateQrCode();

    if (ChangeNowApi::isFinalStatus(order.status)) {
        m_statusPoll.stop();
    } else if (!m_statusPoll.isActive()) {
        m_statusPoll.start();
    }
}

void SwapWidget::updateOrderView() {
    if (!m_order.isValid()) {
        return;
    }

    const QString from = m_order.fromCurrency.isEmpty()
                             ? this->fromLabel()
                             : assetLabel(m_order.fromCurrency, m_order.fromNetwork);
    const QString to = m_order.toCurrency.isEmpty()
                           ? this->toLabel()
                           : assetLabel(m_order.toCurrency, m_order.toNetwork);
    const bool awaitingDeposit = !ChangeNowApi::isFinalStatus(m_order.status)
                                 && !ChangeNowApi::isInProgress(m_order.status);

    const QString status = m_order.status.toLower();
    const bool finished = ChangeNowApi::isFinalStatus(m_order.status);
    const bool succeeded = status == "finished";

    QString headline;
    if (awaitingDeposit) {
        headline = QString("Send %1 %2 to complete your swap")
                       .arg(SwapWidget::formatCoinAmount(m_order.expectedFromAmount, from), from);
    } else if (!finished) {
        headline = QString("Swapping %1 to %2").arg(from, to);
    } else if (succeeded) {
        headline = "Swap complete";
    } else if (status == "refunded") {
        headline = "Deposit refunded";
    } else if (status == "expired") {
        headline = "Swap expired";
    } else {
        headline = "Swap failed";
    }
    ui->label_orderHeadline->setText(headline);

    // The stepper cannot express a refund or an expiry, so those fall back to
    // the status line.
    ui->orderProgress->setStatus(m_order.status);
    const bool tracked = ui->orderProgress->isTrackable();
    ui->orderProgress->setVisible(tracked);
    ui->label_orderStatus->setVisible(!tracked);

    ui->label_orderStatus->setText(ChangeNowApi::statusText(m_order.status));

    // Only the quote endpoints report a forecast, so an order restored from the
    // config after a restart has none.
    const bool showEta = !finished && !m_speedForecast.isEmpty();
    ui->label_orderEta->setVisible(showEta);
    if (showEta) {
        ui->label_orderEta->setText(
            QString("Usually takes %1 minutes. Most of it is waiting for Monero confirmations.")
                .arg(QString(m_speedForecast).replace('-', QString::fromUtf8("–"))));
    }
    const QString outcomeState =
        succeeded ? "done" : (status == "refunded" ? "neutral" : "failed");
    setWidgetState(ui->label_orderStatus, finished ? outcomeState : "pending");

    ui->label_depositAmount->setText(
        QString("%1 %2").arg(SwapWidget::formatCoinAmount(m_order.expectedFromAmount, from), from));
    ui->label_depositAddress->setText(m_order.payinAddress);

    ui->card_deposit->setVisible(awaitingDeposit);

    // Only when this wallet is the one paying.
    ui->btn_sendFromWallet->setVisible(m_order.fromCurrency.compare("XMR", Qt::CaseInsensitive) == 0);

    QString receive = succeeded
        ? QString("Paid out in %1").arg(to)
        : QString("You receive ≈ %1 %2").arg(SwapWidget::formatCoinAmount(m_order.expectedToAmount, to), to);
    if (!m_order.payoutAddress.isEmpty()) {
        receive += QString("\nto %1").arg(m_order.payoutAddress);
    }
    ui->label_orderReceive->setText(receive);

    // What happened, and what the user can still do about it.
    QString outcome;
    if (succeeded) {
        outcome = QString("%1 %2 sent to your address.")
                      .arg(SwapWidget::formatCoinAmount(m_order.expectedToAmount, to), to);
        outcome += " It can take a few minutes to appear in your wallet.";
    } else if (status == "refunded") {
        outcome = QString("Your %1 deposit was returned to the refund address. Nothing was exchanged.")
                      .arg(from);
    } else if (status == "expired") {
        outcome = "No deposit arrived before the deadline, so the order closed. If you did send one, "
                  "contact ChangeNOW with the order ID below -- do not send another.";
    } else if (finished) {
        outcome = "The exchange could not complete this swap. If your deposit was already sent, it is "
                  "returned to the refund address; contact ChangeNOW with the order ID below if it "
                  "does not arrive.";
    }
    ui->label_orderOutcome->setText(outcome);
    ui->label_orderOutcome->setVisible(!outcome.isEmpty());
    // Refunded shares the failure layout but not its colour.
    setWidgetState(ui->label_orderOutcome, outcomeState);

    // Once, on the transition: updateOrderView runs on every poll.
    if (m_announcedStatus != status && !status.isEmpty()) {
        qInfo() << "Swap" << m_order.id << "status:" << status;
    }
    if (finished && m_announcedStatus != status) {
        emit setStatusText(succeeded ? QString("Swap complete: %1 %2 received")
                                           .arg(SwapWidget::formatCoinAmount(m_order.expectedToAmount, to), to)
                                     : QString("Swap %1").arg(ChangeNowApi::statusText(m_order.status).toLower()),
                           true, 10000);
    }
    m_announcedStatus = status;

    ui->label_orderId->setText(QString("Order ID: %1").arg(m_order.id));

    // A tag that did not come back registered was dropped, and a payout without
    // one can be unattributable. Stop the user before they fund it.
    if (!m_requestedMemo.isEmpty() && m_order.payoutExtraId.isEmpty()) {
        ui->label_orderError->setText(
            QString("The exchange did not register the destination tag \"%1\". Do not send this "
                    "deposit if your receiving address needs one -- the payout could arrive "
                    "unattributed. Start a new swap, or contact ChangeNOW with the order ID.")
                .arg(m_requestedMemo));
        ui->label_orderError->show();
    }

    if (!m_order.payoutExtraId.isEmpty()) {
        ui->label_orderExpiry->setText(QString("Destination tag: %1").arg(m_order.payoutExtraId));
    } else if (!m_order.payoutHash.isEmpty()) {
        ui->label_orderExpiry->setText(QString("Payout transaction: %1").arg(m_order.payoutHash));
    } else if (awaitingDeposit && m_order.validUntil.isValid()) {
        ui->label_orderExpiry->setText(
            QString("Deposit before %1").arg(m_order.validUntil.toLocalTime().toString("yyyy-MM-dd HH:mm")));
    } else {
        ui->label_orderExpiry->clear();
    }

    ui->btn_refresh->setEnabled(!finished);

    // The same button, but abandoning a live swap is not finishing one.
    ui->btn_newSwap->setText(finished ? "Finish" : "Discard swap");
}

void SwapWidget::updateQrCode() {
    if (m_order.payinAddress.isEmpty()) {
        ui->qrCode->clear();
        return;
    }

    const QString encoded = depositUri(m_order.fromCurrency, m_order.fromNetwork,
                                       m_order.payinAddress,
                                       SwapWidget::exactAmount(m_order.expectedFromAmount));
    const QrCode qr{encoded.isEmpty() ? m_order.payinAddress : encoded,
                    QrCode::Version::AUTO, QrCode::ErrorCorrectionLevel::MEDIUM};
    if (!qr.isValid()) {
        ui->qrCode->clear();
        return;
    }

    // Radius matches the label's own, set by the stylesheet.
    ui->qrCode->setPixmap(Utils::qrCodePlate(qr, ui->qrCode->size(),
                                             this->devicePixelRatioF(), 14, 12));
}

void SwapWidget::refreshOrder() {
    if (!m_order.isValid()) {
        m_statusPoll.stop();
        return;
    }

    // Manual refreshes do not count against the bound; only the timer's do.
    if (m_statusPoll.isActive() && ++m_statusPolls > kMaxStatusPolls) {
        m_statusPoll.stop();
        ui->label_orderError->setText(
            "This swap has not changed for several hours, so the wallet has stopped checking. "
            "Use Refresh, or follow it on ChangeNOW with the order ID below.");
        ui->label_orderError->show();
        return;
    }

    m_api->requestOrder(m_order.id);
}

void SwapWidget::restoreOrder() {
    const QString id = conf()->get(Config::swapOrderId).toString();
    if (id.isEmpty()) {
        return;
    }

    // Only the id survives a restart, so open on a placeholder and fill in from
    // the exchange rather than waiting for the reply.
    m_order = ChangeNowApi::Order{};
    m_order.id = id;

    ui->stack->setCurrentWidget(ui->page_order);
    ui->label_orderHeadline->setText("Restoring your swap…");
    ui->label_orderStatus->clear();
    ui->orderProgress->hide();
    ui->label_orderId->setText(QString("Order ID: %1").arg(id));
    ui->card_deposit->hide();
    ui->btn_newSwap->setText("Discard swap");

    this->refreshOrder();
    if (!m_statusPoll.isActive()) {
        m_statusPoll.start();
    }
}

void SwapWidget::onNewSwapClicked() {
    // The exchange may be holding funds for an unfinished swap.
    if (m_order.isValid() && !ChangeNowApi::isFinalStatus(m_order.status)) {
        const auto answer = QMessageBox::question(
            this, "Discard this swap?",
            "This swap has not finished. Best Wallet will stop tracking it, and the "
            "deposit address will no longer be shown here.\n\n"
            "If you have already sent your deposit, copy the order ID first -- "
            "ChangeNOW's support can only find the swap by that ID.",
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);

        if (answer != QMessageBox::Yes) {
            return;
        }
    }

    this->clearOrder();
}

void SwapWidget::clearOrder() {
    m_statusPoll.stop();
    m_order = ChangeNowApi::Order{};
    conf()->set(Config::swapOrderId, "");

    ui->label_orderError->clear();
    ui->label_orderError->hide();
    ui->qrCode->clear();
    ui->stack->setCurrentWidget(ui->page_quote);

    m_requestedMemo.clear();
    m_announcedStatus.clear();
    m_speedForecast.clear();
    m_statusPolls = 0;
    this->setAmountQuietly(ui->lineAmountFrom, {});
    this->setAmountQuietly(ui->lineAmountTo, {});
    ui->label_rate->clear();
    m_haveQuote = false;
    this->updateAmountNotice();

    this->clearQuoteError();
    this->updateCreateButton();
}

QString SwapWidget::exactAmount(double amount) {
    // Twelve places is Monero's own precision, more than any offered asset needs.
    return Utils::trimmedDecimal(amount, 12);
}

QString SwapWidget::formatCoinAmount(double amount, const QString &coin) {
    // Kept so adding an asset with different precision changes this function
    // rather than every call site.
    Q_UNUSED(coin)
    return Utils::trimmedDecimal(amount, kAmountDecimals);
}

void SwapWidget::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);

    if (m_unavailable) {
        return;
    }

    if (ui->stack->currentWidget() == ui->page_quote) {
        this->scheduleQuote();
    }
}

void SwapWidget::hideEvent(QHideEvent *event) {
    QWidget::hideEvent(event);

    // Stop querying the exchange for a rate nobody is looking at. The status
    // poll keeps running, so a swap in flight still reaches the status bar.
    m_quoteDebounce.stop();
    m_quoteRefresh.stop();
}

void SwapWidget::skinChanged() {
    ui->orderProgress->setLightTheme(
        conf()->get(Config::skin).toString() == constants::skinNativeWhite);

    // The plate paints its own white ground, but its corner radius comes from
    // the stylesheet the new skin may have changed.
    this->updateQrCode();
}

SwapWidget::~SwapWidget() = default;
