// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include <QStyle>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include "constants.h"
#include <QStackedLayout>
#include <QHBoxLayout>
#include <QFrame>

#include "HomeWidget.h"
#include "TransactionHistory.h"
#include "WalletManager.h"
#include "ui_HomeWidget.h"

#include "utils/config.h"
#include "utils/AppData.h"
#include "PriceHistory.h"
#include "components.h"

#include <algorithm>
#include <limits>
#include "rows/TransactionRow.h"



namespace {
    // Drawn rather than typed: a font's arrow glyph carries its own baseline and
    // weight, so it sits high in the badge and reads thin beside the label.
    QPixmap activityArrow(bool down, const QColor &ink, int badgeSize, qreal dpr)
    {
        QPixmap pm{QSize(badgeSize, badgeSize) * dpr};
        pm.setDevicePixelRatio(dpr);
        pm.fill(Qt::transparent);

        // Ratios of the arrow's own span, tuned against a 28px badge.
        constexpr qreal kSpan     = 0.46;   // of the badge
        constexpr qreal kWeight   = 0.125;  // stroke
        constexpr qreal kHeadLen  = 0.32;
        constexpr qreal kHeadHalf = 0.26;

        const qreal span = badgeSize * kSpan;
        const QPointF centre{badgeSize / 2.0, badgeSize / 2.0};
        const QPointF tail = centre + QPointF(0, down ? -span / 2 : span / 2);
        const QPointF tip  = centre + QPointF(0, down ?  span / 2 : -span / 2);

        QPainter painter{&pm};
        painter.setRenderHint(QPainter::Antialiasing);

        QPen pen{ink};
        pen.setWidthF(span * kWeight);
        pen.setCapStyle(Qt::RoundCap);
        pen.setJoinStyle(Qt::RoundJoin);
        painter.setPen(pen);

        painter.drawLine(tail, tip);

        const QPointF dir{0, down ? 1.0 : -1.0};
        const QPointF nrm{-dir.y(), dir.x()};
        QPainterPath head;
        head.moveTo(tip - dir * span * kHeadLen + nrm * span * kHeadHalf);
        head.lineTo(tip);
        head.lineTo(tip - dir * span * kHeadLen - nrm * span * kHeadHalf);
        painter.drawPath(head);
        painter.end();

        return pm;
    }
}

namespace {
    // The chart's five ranges map onto the four spans the price service serves;
    // "All" reaches as far back as it goes, which is a year.
    PriceHistory::Range priceRangeFor(int chartRange)
    {
        switch (chartRange) {
            case 0: return PriceHistory::Range::Day;
            case 1: return PriceHistory::Range::Week;
            case 2: return PriceHistory::Range::Month;
            default: return PriceHistory::Range::Year;
        }
    }
}

HomeWidget::HomeWidget(Wallet *wallet, QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::HomeWidget)
    , m_wallet(wallet)
{
    ui->setupUi(this);

    // Required: QStyleSheetStyle only paints a background by itself for widgets
    // whose metaObject is QWidget's, and Q_OBJECT makes this one "HomeWidget",
    // so `#HomeWidget { background: ... }` is otherwise ignored.
    this->setAttribute(Qt::WA_StyledBackground, true);

    m_prices = new PriceHistory(this);
    connect(m_prices, &PriceHistory::updated, this, [this] {
        // Both charts read from the series: the market one directly, the
        // portfolio one for its valuation.
        this->updateChart();
        if (m_wallet) {
            this->updateBalance(m_wallet->balance(), m_wallet->unlockedBalance());
        }
    });

    this->setupXmrChart();

    connect(m_wallet->history(),
            &TransactionHistory::refreshFinished,
            this,
            &HomeWidget::onHistoryChanged);
    connect(ui->viewHistory, &ClickableLabel::clicked, this, &HomeWidget::showHistory);
    connect(m_wallet, &Wallet::balanceUpdated, this, &HomeWidget::updateBalance);
    connect(m_wallet, &Wallet::connectionStatusChanged, this, &HomeWidget::updateConnectionState);

    // A wallet opened from cache already has its history and balance, and both
    // signals only fire on a change.
    this->updateConnectionState(m_wallet->connectionStatus());
    this->updateBalance(m_wallet->balance(), m_wallet->unlockedBalance());
    this->onHistoryChanged();

    // Both are denominated in fiat, so a new rate has to redraw them.
    connect(&appData()->prices, &Prices::fiatPricesUpdated, this, &HomeWidget::onPricesChanged);
    connect(&appData()->prices, &Prices::cryptoPricesUpdated, this, &HomeWidget::onPricesChanged);
}

void HomeWidget::onPricesChanged()
{
    if (!m_wallet)
        return;

    this->updateBalance(m_wallet->balance(), m_wallet->unlockedBalance());
    this->updateChart();
}

QString HomeWidget::fiatSymbol() const
{
    // m_chartCurrency, not the preference: the market series falls back to USD
    // when no rate is held for the preferred currency, and the axis has to say
    // which one the numbers are in.
    const QString code = m_chartCurrency.isEmpty()
        ? conf()->get(Config::preferredFiatCurrency).toString()
        : m_chartCurrency;
    return Utils::getCurrencyLocale(code).currencySymbol();
}

double HomeWidget::fiatRate() const
{
    const QString fiat = conf()->get(Config::preferredFiatCurrency).toString();
    const double rate = appData()->prices.convert("XMR", fiat, 1.0);
    // convert() reports -1 when it has no rate for the pair yet.
    if (rate > 0) {
        return rate;
    }

    // The rate feed has nothing yet. The price history's most recent sample is a
    // good enough spot price, and it survives restarts.
    const double usd = m_prices ? m_prices->spot() : 0.0;
    if (usd <= 0) {
        return 0.0;
    }
    if (fiat.compare("USD", Qt::CaseInsensitive) == 0) {
        return usd;
    }
    const double ratio = appData()->prices.convert("USD", fiat, 1.0);
    return ratio > 0 ? usd * ratio : 0.0;
}

void HomeWidget::addPlugin(Plugin *plugin)
{
    if (plugin->type() == Plugin::TAB) {
        //ui->tabHomeWidget->addTab(plugin->tab(), plugin->displayName());
    }
    else if (plugin->type() == Plugin::WIDGET) {
        ui->widgetLayout->insertWidget(0, plugin->tab());
    }
}

void HomeWidget::aboutToQuit() {
    //conf()->set(Config::homeWidget, ui->tabHomeWidget->currentIndex());
}

void HomeWidget::uiSetup() {
    //ui->tabHomeWidget->setCurrentIndex(conf()->get(Config::homeWidget).toInt());
}

void HomeWidget::updateBalance(quint64 balance, quint64 spendable)
{
    Q_UNUSED(spendable)

    // The headline is the balance in XMR; fiat is the aside beside it.
    const bool light = conf()->get(Config::skin).toString() == constants::skinNativeWhite;
    const QString muted = light ? "#8B95A3" : "#979696";

    // The status bar already honours this.
    if (conf()->get(Config::hideBalance).toBool()) {
        ui->accountBalance->setText("<span>Hidden</span>");
        return;
    }

    // amountPrecision defaults to 12, which is far more than a headline figure
    // wants; a user who has asked for fewer still gets fewer.
    constexpr int kHeadlineDecimals = 5;
    const int decimals = qMin(conf()->get(Config::amountPrecision).toInt(), kHeadlineDecimals);
    QString amount = WalletManager::displayAmount(balance, false, decimals);

    // displayAmount truncates, so a balance below the rounding step comes back a
    // flat "0"; fall back to full precision rather than show nothing.
    if (balance > 0 && amount.toDouble() == 0.0) {
        amount = WalletManager::displayAmount(balance, false, 12);
    }

    QString text = QString("<span>%1</span> <span style='color:%2; font-size:14px;'>XMR</span>")
                       .arg(amount, muted);

    const double rate = this->fiatRate();
    if (rate > 0) {
        const QString fiatCurrency = conf()->get(Config::preferredFiatCurrency).toString();
        const double fiat = rate * (double(balance) / constants::cdiv);
        text += QString(" <span style='color:%1; font-size:14px;'>· %2</span>")
                    .arg(muted, Utils::amountToCurrencyString(fiat, fiatCurrency).toHtmlEscaped());
    }

    ui->accountBalance->setText(text);
}

void HomeWidget::updateConnectionState(int status)
{
    QString state;
    QString text;

    switch (status) {
    case Wallet::ConnectionStatus_Disconnected:
        state = "disconnected";
        text = "Disconnected";
        break;

    case Wallet::ConnectionStatus_Connecting:
        state = "connecting";
        text = "Connecting";
        break;

    case Wallet::ConnectionStatus_Synchronizing:
        state = "synchronizing";
        text = "Syncing";
        break;

    case Wallet::ConnectionStatus_Synchronized:
        state = "connected";
        text = "Connected";
        break;

    case Wallet::ConnectionStatus_WrongVersion:
        state = "error";
        text = "Node error";
        break;

    default:
        // Every enumerator is handled above.
        state = "disconnected";
        text = "Disconnected";
        break;
    }

    ui->connectionIndicator->setProperty("state", state);
    ui->connectionState->setText(text);
    ui->framePill->setProperty("state", state);

    for (QWidget *w : {static_cast<QWidget*>(ui->connectionIndicator),
                       static_cast<QWidget*>(ui->framePill)}) {
        w->style()->unpolish(w);
        w->style()->polish(w);
    }
}

void HomeWidget::onHistoryChanged()
{
    if (!m_wallet)
        return;

    const QList<TransactionRow> rows = m_wallet->history()->getRows();

    QList<TransactionRow> completed;
    completed.reserve(rows.size());
    for (const auto &tx : rows) {
        if (tx.failed || tx.balanceDelta == 0)
            continue;
        completed.append(tx);
    }

    std::sort(completed.begin(), completed.end(), [](const TransactionRow &a, const TransactionRow &b) {
        return a.timestamp > b.timestamp;
    });

    this->rebuildActivityList(completed);
    this->updateChart();
}

void HomeWidget::rebuildActivityList(const QList<TransactionRow> &completed)
{
    QLayoutItem *item;
    while ((item = ui->activityList->takeAt(0)) != nullptr) {
        if (QWidget *w = item->widget()) {
            w->deleteLater();
        }
        delete item;
    }

    if (completed.isEmpty()) {
        auto *empty = new QLabel("No transactions yet");
        empty->setObjectName("activityEmptyLabel");
        empty->setAlignment(Qt::AlignCenter);
        ui->activityList->addWidget(empty);
        return;
    }

    const int maxRows = qMin(completed.size(), 3);
    for (int i = 0; i < maxRows; ++i) {
        ui->activityList->addWidget(this->buildActivityRow(completed.at(i)));
    }
}

QWidget* HomeWidget::buildActivityRow(const TransactionRow &tx)
{
    const bool isReceived = tx.balanceDelta > 0;
    const double amount = std::abs(static_cast<double>(tx.balanceDelta) / 1e12);

    auto *row = new QFrame(this);
    row->setObjectName("activityRow");

    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);

    // Mirrors #activityBadgeIn / #activityBadgeOut: a painted glyph cannot pick
    // its ink up from the stylesheet.
    const bool light = conf()->get(Config::skin).toString() == constants::skinNativeWhite;
    const QColor badgeInk = isReceived ? (light ? QColor("#1F9D53") : QColor("#65cf6a"))
                                       : QColor(constants::accentColor);

    constexpr int kBadgeSize = 28;
    auto *badge = new QLabel();
    badge->setObjectName(isReceived ? "activityBadgeIn" : "activityBadgeOut");
    badge->setAlignment(Qt::AlignCenter);
    badge->setFixedSize(kBadgeSize, kBadgeSize);
    badge->setPixmap(activityArrow(isReceived, badgeInk, kBadgeSize,
                                   this->devicePixelRatioF()));
    layout->addWidget(badge);

    auto *label = new QLabel(QString("%1 %2 XMR").arg(isReceived ? "Received" : "Sent", formatXmr(amount)));
    label->setObjectName("activityLabel");
    layout->addWidget(label);

    layout->addStretch();

    auto *time = new QLabel(formatRelativeTime(tx.timestamp));
    time->setObjectName("activityTime");
    layout->addWidget(time);

    return row;
}

qint64 HomeWidget::rangeSpanMs() const
{
    switch (m_range) {
        case ChartRange::Day:   return 1LL * 86400 * 1000;
        case ChartRange::Week:  return 7LL * 86400 * 1000;
        case ChartRange::Month: return 30LL * 86400 * 1000;
        case ChartRange::Year:  return 365LL * 86400 * 1000;
        case ChartRange::All:   return 0;
    }
    return 0;
}

double HomeWidget::historyStartMs() const
{
    if (!m_wallet) {
        return 0.0;
    }

    // Cached: heightToDate walks the whole restore-height table, and this runs
    // twice per redraw -- on every price tick -- for an answer fixed at the
    // wallet's creation height.
    if (m_historyStartMs >= 0.0) {
        return m_historyStartMs;
    }

    // A wallet restored from a recent height never saw the blocks before it, so
    // it cannot say what it held back then -- not even that it held nothing.
    const quint64 height = m_wallet->getWalletCreationHeight();
    if (height <= 1) {
        m_historyStartMs = 0.0;
        return m_historyStartMs;
    }

    const QDateTime when =
        appData()->restoreHeights[constants::networkType]->heightToDate(int(height));
    m_historyStartMs = when.isValid() ? double(when.toMSecsSinceEpoch()) : 0.0;
    return m_historyStartMs;
}

QVector<QPointF> HomeWidget::pointsForRange(const QVector<QPointF> &all) const
{
    if (all.isEmpty()) {
        return all;
    }

    QVector<QPointF> window = all;

    const qint64 spanMs = this->rangeSpanMs();
    if (spanMs > 0) {
        const double cutoff = all.last().x() - double(spanMs);
        window.clear();
        window.reserve(all.size());
        for (int i = 0; i < all.size(); ++i) {
            const QPointF &point = all.at(i);
            if (point.x() < cutoff) {
                continue;
            }

            // Meet the left edge exactly: the first sample inside the window can
            // land well after the cutoff, since history is only daily beyond
            // ninety days.
            if (window.isEmpty() && i > 0 && point.x() > cutoff) {
                const QPointF &prev = all.at(i - 1);
                const double dx = point.x() - prev.x();
                const double t = dx > 0 ? (cutoff - prev.x()) / dx : 0.0;
                window.append(QPointF(cutoff, prev.y() + (point.y() - prev.y()) * t));
            }

            window.append(point);
        }
    }

    // Never leave the chart with a single point: a range shorter than the gap
    // between samples would otherwise draw nothing at all.
    if (window.size() < 2) {
        return all.mid(qMax(0, all.size() - 2));
    }

    // The source mixes daily and intraday samples, so taken as-is a long range
    // is a spiky cluster at the recent end and smooth everywhere else.
    constexpr int kMaxPoints = 400;
    if (window.size() <= kMaxPoints) {
        return window;
    }

    const double first = window.first().x();
    const double last = window.last().x();
    const double step = (last - first) / (kMaxPoints - 1);

    QVector<QPointF> even;
    even.reserve(kMaxPoints + m_stepStamps.size());
    int cursor = 0;
    for (int i = 0; i < kMaxPoints; ++i) {
        const double target = first + step * i;
        while (cursor + 1 < window.size() && window.at(cursor + 1).x() <= target) {
            ++cursor;
        }

        // Interpolated, not snapped: holding the last sample until the next one
        // lands turns a drifting price into a staircase.
        const QPointF &a = window.at(cursor);
        if (cursor + 1 >= window.size()) {
            even.append(QPointF(target, a.y()));
            continue;
        }
        const QPointF &b = window.at(cursor + 1);
        const double dx = b.x() - a.x();
        const double t = dx > 0 ? (target - a.x()) / dx : 0.0;
        even.append(QPointF(target, a.y() + (b.y() - a.y()) * t));
    }
    even.last() = window.last();

    // An even grid can land either side of a deposit and draw it as a ramp, so
    // the exact points around each jump are put back.
    if (!m_stepStamps.isEmpty()) {
        for (const QPointF &point : window) {
            if (std::binary_search(m_stepStamps.constBegin(), m_stepStamps.constEnd(), point.x())) {
                even.append(point);
            }
        }
        std::sort(even.begin(), even.end(),
                  [](const QPointF &a, const QPointF &b) { return a.x() < b.x(); });
    }

    return even;
}

void HomeWidget::updateChart()
{
    QVector<QPointF> history = pointsForRange(this->chartSeries());
    const bool hasHistory = !history.isEmpty();
    const double nowMs = double(QDateTime::currentDateTime().toMSecsSinceEpoch());

    if (!hasHistory) {
        // Flat, muted placeholder signalling "no activity yet" instead of
        // plotting fabricated data that could be mistaken for real history.
        history = {
            QPointF(nowMs - 7.0 * 86400 * 1000, 0.0),
            QPointF(nowMs, 0.0)
        };
    }

    m_plotted = hasHistory ? history : QVector<QPointF>{};
    this->hideHover();

    m_xmrUpper->replace(history);
    m_xmrLine->replace(history);

    // Extent of what is plotted; the axes are set from it so the fill spans the
    // plot area edge to edge.
    double minX = history.first().x();
    double maxX = history.first().x();
    double minY = history.first().y();
    double maxY = history.first().y();
    for (const QPointF &point : history) {
        minX = qMin(minX, point.x());
        maxX = qMax(maxX, point.x());
        minY = qMin(minY, point.y());
        maxY = qMax(maxY, point.y());
    }

    // A range button picks a window, not merely a filter, or a young account
    // shows the same fortnight on 1M, 1Y and All. Only the portfolio series is
    // extended back across the window; the market one is as wide as the price
    // source gave, and stating more would strand its line in an empty plot.
    const qint64 spanMs = this->rangeSpanMs();
    double axisMin = minX;
    double axisMax = maxX;
    if (spanMs > 0 && m_mode == ChartMode::Portfolio && hasHistory) {
        axisMax = qMax(maxX, nowMs);
        axisMin = qMin(minX, axisMax - double(spanMs));
    }

    if (axisMax > axisMin) {
        m_axisX->setRange(QDateTime::fromMSecsSinceEpoch(qint64(axisMin)),
                          QDateTime::fromMSecsSinceEpoch(qint64(axisMax)));
    }

    if (!hasHistory) {
        // Every y is 0, so a fitted axis would run the line through the
        // empty-state text. Pin it to the bottom as a baseline.
        m_axisY->setRange(0.0, 1.0);
    }
    else {
        // Headroom so the curve never touches an edge. Proportional, since an
        // absolute pad means something different per unit on the axis.
        const double span = maxY - minY;
        const double pad = span > 0 ? span * 0.12 : qMax(maxY * 0.1, 0.01);
        m_axisY->setRange(qMax(0.0, minY - pad), maxY + pad);
    }

    // The fill closes on the bottom of the plot, not on zero. A zero baseline
    // puts most of the area path below the visible range, and the gradient is
    // relative to the path's bounding box, so only its top few percent land in
    // the plot and the fill reads as a flat slab.
    QVector<QPointF> lower;
    lower.reserve(history.size());
    for (const QPointF &point : history)
        lower.append(QPointF(point.x(), m_axisY->min()));

    m_xmrLower->replace(lower);

    if (m_mode == ChartMode::Market) {
        ui->label_chartTitle->setText("XMR Price");
        // On "All" the price source gives less than the button promises, so say
        // how far back it actually reaches.
        ui->label_chartSubtitle->setText(
            m_range == ChartRange::All
                ? "The market price of Monero - price data goes back one year"
                : "The market price of Monero over time");
    }
    else {
        // With no rate the series falls back to XMR.
        ui->label_chartTitle->setText(m_chartInFiat ? "Portfolio Value" : "Balance");

        // A line starting short of the left edge means the window reaches past
        // the restore height. Said plainly, or the chart looks like it lost the
        // history rather than never having had it.
        const double knownFrom = this->historyStartMs();
        const bool blankLeft = hasHistory && minX > axisMin + 60000.0 && knownFrom > 0;
        const QString startsOn =
            blankLeft ? QDateTime::fromMSecsSinceEpoch(qint64(knownFrom)).toString("MMM d, yyyy")
                      : QString();

        if (m_chartInFiat) {
            ui->label_chartSubtitle->setText(
                startsOn.isEmpty()
                    ? QString("What your Monero has been worth over time")
                    : QString("What your Monero has been worth - this wallet's history starts %1")
                          .arg(startsOn));
        }
        else {
            // Say why it is in XMR, or the fallback is indistinguishable from
            // the feature being missing.
            const QString why = m_prices->status();
            ui->label_chartSubtitle->setText(
                why.isEmpty() ? "Your balance in XMR - waiting for price data"
                              : QString("Your balance in XMR - %1").arg(why));
        }
    }

    // Why it is empty differs per mode: an untouched wallet versus a price feed
    // with no source.
    if (m_chartEmptyTitle && m_chartEmptySubtitle) {
        if (m_mode == ChartMode::Market) {
            if (m_prices->isFetching(priceRangeFor(int(m_range)))) {
                m_chartEmptyTitle->setText("Loading price history...");
                m_chartEmptySubtitle->setText("Fetching XMR prices");
            }
            else if (!m_prices->status().isEmpty()) {
                // Cached data would have been drawn instead, so there is
                // genuinely nothing yet; it keeps retrying.
                m_chartEmptyTitle->setText("Waiting for price data");
                m_chartEmptySubtitle->setText(m_prices->status());
            }
            else {
                m_chartEmptyTitle->setText("Loading price history...");
                m_chartEmptySubtitle->setText("Fetching XMR prices");
            }
        }
        else {
            m_chartEmptyTitle->setText("No transaction history yet");
            m_chartEmptySubtitle->setText("Your balance will be charted here after your first transaction");
        }
    }

    // Y ticks name the unit the series is actually in, which is the preferred
    // fiat currency, or XMR when the series fell back to it.
    m_axisY->setLabelFormat(m_chartInFiat ? QString("%1%.0f").arg(this->fiatSymbol())
                                          : QString("%.2f"));

    // Tick labels that suit the visible span.
    switch (m_range) {
        case ChartRange::Day:   m_axisX->setFormat("HH:mm");    m_axisX->setTickCount(7); break;
        case ChartRange::Week:  m_axisX->setFormat("ddd");      m_axisX->setTickCount(7); break;
        case ChartRange::Month: m_axisX->setFormat("MMM d");    m_axisX->setTickCount(6); break;
        case ChartRange::Year:  m_axisX->setFormat("MMM yyyy"); m_axisX->setTickCount(6); break;
        case ChartRange::All:   m_axisX->setFormat("MMM yyyy"); m_axisX->setTickCount(6); break;
    }

    this->applyChartTheme();

    if (m_chartEmptyBox) {
        m_chartEmptyBox->setVisible(!hasHistory);
    }
    m_axisX->setVisible(hasHistory);
    m_axisY->setVisible(hasHistory);
}

void HomeWidget::applyChartTheme()
{
    const bool light = conf()->get(Config::skin).toString() == constants::skinNativeWhite;
    const QColor accent{229, 107, 63};
    const QColor axisText = light ? QColor("#8B95A3") : QColor("#7c7c7e");

    const bool hasHistory = !m_plotted.isEmpty();

    if (hasHistory) {
        // The same ramp on both skins: a linear alpha falloff loses lightness at
        // the same rate over either ground, so only the starting alpha matters.
        QLinearGradient gradient(0, 0, 0, 1);
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        gradient.setColorAt(0.0, QColor(229, 107, 63, 150));
        gradient.setColorAt(0.5, QColor(229, 107, 63, 75));
        gradient.setColorAt(1.0, QColor(229, 107, 63, 0));
        m_xmrArea->setPen(Qt::NoPen);
        m_xmrArea->setBrush(gradient);

        QPen linePen{accent};
        linePen.setWidth(3);
        m_xmrLine->setPen(linePen);
    }
    else {
        m_xmrArea->setPen(Qt::NoPen);
        m_xmrArea->setBrush(Qt::NoBrush);

        QPen linePen{light ? QColor("#C0C7D1") : QColor(90, 90, 90)};
        linePen.setWidth(2);
        linePen.setStyle(Qt::DashLine);
        m_xmrLine->setPen(linePen);
    }

    for (QAbstractAxis *axis : {static_cast<QAbstractAxis*>(m_axisX),
                                static_cast<QAbstractAxis*>(m_axisY)}) {
        if (!axis) {
            continue;
        }
        axis->setLabelsColor(axisText);
        axis->setLabelsFont(QFont("Inter", 8));
        axis->setLineVisible(false);
        axis->setGridLineVisible(false);
    }

    if (m_hoverMarker) {
        m_hoverMarker->setColor(accent);
        m_hoverMarker->setBorderColor(light ? QColor("#FFFFFF") : QColor("#101112"));
    }
}

void HomeWidget::skinChanged()
{
    this->applyChartTheme();

    // The badges are painted glyphs and the balance's muted half is inline HTML,
    // so neither follows the stylesheet on its own.
    this->onHistoryChanged();
    if (m_wallet) {
        this->updateBalance(m_wallet->balance(), m_wallet->unlockedBalance());
    }
}

void HomeWidget::setupRangeSelector()
{
    const QVector<QPair<QPushButton*, ChartRange>> buttons = {
        {ui->btn_range1D,  ChartRange::Day},
        {ui->btn_range1W,  ChartRange::Week},
        {ui->btn_range1M,  ChartRange::Month},
        {ui->btn_range1Y,  ChartRange::Year},
        {ui->btn_rangeAll, ChartRange::All},
    };

    for (const auto &entry : buttons) {
        QPushButton *button = entry.first;
        const ChartRange range = entry.second;
        connect(button, &QPushButton::clicked, this, [this, range]{
            if (m_range == range) {
                return;
            }
            m_range = range;
            this->updateChart();
        });
    }
}

void HomeWidget::setupModeSelector()
{
    const QVector<QPair<QPushButton*, ChartMode>> buttons = {
        {ui->btn_modeMarket,    ChartMode::Market},
        {ui->btn_modePortfolio, ChartMode::Portfolio},
    };

    for (const auto &entry : buttons) {
        QPushButton *button = entry.first;
        const ChartMode mode = entry.second;
        connect(button, &QPushButton::clicked, this, [this, mode]{
            if (m_mode == mode) {
                return;
            }
            m_mode = mode;
            this->updateChart();
        });
    }
}

void HomeWidget::updateHover(const QPoint &viewportPos)
{
    if (m_plotted.size() < 2 || !m_hoverTip) {
        return;
    }

    const QPointF scenePos = m_xmrChartView->mapToScene(viewportPos);
    const QPointF chartPos = m_xmrChart->mapFromScene(scenePos);
    if (!m_xmrChart->plotArea().contains(chartPos)) {
        this->hideHover();
        return;
    }

    const QPointF value = m_xmrChart->mapToValue(chartPos, m_xmrLine);

    // Nearest along the time axis, so the pointer need not be on the line.
    QPointF nearest = m_plotted.first();
    for (const QPointF &point : m_plotted) {
        if (qAbs(point.x() - value.x()) < qAbs(nearest.x() - value.x())) {
            nearest = point;
        }
    }

    m_hoverMarker->replace(QList<QPointF>{nearest});
    m_hoverMarker->setVisible(true);

    const QDateTime stamp = QDateTime::fromMSecsSinceEpoch(qint64(nearest.x()));
    m_hoverValue->setText(
        m_chartInFiat ? Utils::amountToCurrencyString(nearest.y(), m_chartCurrency)
                      : QString("%L1 XMR").arg(nearest.y(), 0, 'f', 4));
    // Time of day on every range: the date alone does not identify a sample.
    m_hoverDate->setText(stamp.toString("MMM d, HH:mm"));
    m_hoverTip->adjustSize();

    // Anchor the callout just above the point, kept inside the chart card.
    const QPointF markerScene = m_xmrChart->mapToScene(m_xmrChart->mapToPosition(nearest, m_xmrLine));
    const QPoint markerInView = m_xmrChartView->mapFromScene(markerScene);
    const QPoint markerInHost = m_xmrChartView->mapTo(ui->chartContainer, markerInView);

    int x = markerInHost.x() - m_hoverTip->width() / 2;
    int y = markerInHost.y() - m_hoverTip->height() - 14;
    x = qBound(4, x, qMax(4, ui->chartContainer->width() - m_hoverTip->width() - 4));
    if (y < 4) {
        y = markerInHost.y() + 14;      // flip below when near the top edge
    }

    m_hoverTip->move(x, y);
    m_hoverTip->raise();
    m_hoverTip->show();
}

void HomeWidget::hideHover()
{
    if (m_hoverTip) {
        m_hoverTip->hide();
    }
    if (m_hoverMarker) {
        m_hoverMarker->setVisible(false);
    }
}

bool HomeWidget::eventFilter(QObject *watched, QEvent *event)
{
    if (m_xmrChartView && watched == m_xmrChartView->viewport()) {
        switch (event->type()) {
            case QEvent::MouseMove:
                this->updateHover(static_cast<QMouseEvent*>(event)->pos());
                break;
            case QEvent::Leave:
                this->hideHover();
                break;
            default:
                break;
        }
    }

    return QWidget::eventFilter(watched, event);
}

void HomeWidget::setupXmrChart()
{
    m_xmrUpper = new QLineSeries();
    m_xmrLower = new QLineSeries();
    m_xmrLine  = new QLineSeries();

    m_xmrArea = new QAreaSeries(m_xmrUpper, m_xmrLower);

    m_hoverMarker = new QScatterSeries();
    m_hoverMarker->setMarkerSize(11);
    m_hoverMarker->setBorderColor(QColor("#101112"));
    m_hoverMarker->setVisible(false);

    m_xmrChart = new QChart();
    m_xmrChart->legend()->hide();
    m_xmrChart->setBackgroundVisible(false);
    // Room for the axis labels, which otherwise collide with the container's
    // rounded corners.
    m_xmrChart->setMargins(QMargins(4, 6, 10, 5));

    m_xmrChart->addSeries(m_xmrArea);
    m_xmrChart->addSeries(m_xmrLine);
    m_xmrChart->addSeries(m_hoverMarker);

    m_axisX = new QDateTimeAxis();
    m_axisY = new QValueAxis();
    m_xmrChart->addAxis(m_axisX, Qt::AlignBottom);
    m_xmrChart->addAxis(m_axisY, Qt::AlignLeft);

    for (QAbstractSeries *series : {static_cast<QAbstractSeries*>(m_xmrArea),
                                    static_cast<QAbstractSeries*>(m_xmrLine),
                                    static_cast<QAbstractSeries*>(m_hoverMarker)}) {
        series->attachAxis(m_axisX);
        series->attachAxis(m_axisY);
    }

    m_xmrChartView = new QChartView(m_xmrChart);
    m_xmrChartView->setRenderHint(QPainter::Antialiasing);
    m_xmrChartView->setFrameShape(QFrame::NoFrame);
    m_xmrChartView->setAttribute(Qt::WA_TranslucentBackground);
    m_xmrChartView->setStyleSheet("background: transparent; border: none;");
    m_xmrChartView->setMouseTracking(true);
    m_xmrChartView->viewport()->setMouseTracking(true);
    m_xmrChartView->viewport()->installEventFilter(this);

    // Overlaid on the chart rather than swapped in for it, so the empty state
    // keeps the same layout as the populated one.
    m_chartEmptyBox = new QWidget(ui->chartContainer);
    m_chartEmptyBox->setObjectName("chartEmptyBox");
    m_chartEmptyBox->setAttribute(Qt::WA_TransparentForMouseEvents);

    auto *emptyLayout = new QVBoxLayout(m_chartEmptyBox);
    emptyLayout->setSpacing(4);
    emptyLayout->addStretch();

    m_chartEmptyTitle = new QLabel("No transaction history yet");
    m_chartEmptyTitle->setObjectName("chartEmptyTitle");
    m_chartEmptyTitle->setAlignment(Qt::AlignCenter);
    m_chartEmptyTitle->setWordWrap(true);
    emptyLayout->addWidget(m_chartEmptyTitle);

    m_chartEmptySubtitle = new QLabel("Your balance will be charted here after your first transaction");
    m_chartEmptySubtitle->setObjectName("chartEmptySubtitle");
    m_chartEmptySubtitle->setAlignment(Qt::AlignCenter);
    m_chartEmptySubtitle->setWordWrap(true);
    emptyLayout->addWidget(m_chartEmptySubtitle);

    emptyLayout->addStretch();
    // Leaves room for the baseline the empty chart draws along the bottom.
    emptyLayout->addSpacing(28);

    auto *stack = new QStackedLayout(ui->chartContainer);
    stack->setStackingMode(QStackedLayout::StackAll);
    stack->setContentsMargins(8, 8, 8, 8);
    stack->addWidget(m_xmrChartView);
    stack->addWidget(m_chartEmptyBox);

    // A child of the chart container, so it floats above the plot.
    m_hoverTip = new QFrame(ui->chartContainer);
    m_hoverTip->setObjectName("chartTooltip");
    m_hoverTip->setAttribute(Qt::WA_TransparentForMouseEvents);
    m_hoverTip->hide();

    auto *tipLayout = new QVBoxLayout(m_hoverTip);
    tipLayout->setContentsMargins(10, 6, 10, 6);
    tipLayout->setSpacing(1);

    m_hoverValue = new QLabel();
    m_hoverValue->setObjectName("chartTooltipValue");
    tipLayout->addWidget(m_hoverValue);

    m_hoverDate = new QLabel();
    m_hoverDate->setObjectName("chartTooltipDate");
    tipLayout->addWidget(m_hoverDate);

    this->setupRangeSelector();
    this->setupModeSelector();

    // Populate series and ranges now that everything is attached.
    this->updateChart();
}

QString HomeWidget::formatXmr(double amount)
{
    return QString::number(amount, 'f', 6)
    .remove(QRegularExpression("0+$"))
        .remove(QRegularExpression("\\.$"));

}

QString HomeWidget::formatRelativeTime(const QDateTime &timestamp)
{
    if (!timestamp.isValid()) {
        return QString();
    }

    const qint64 seconds = timestamp.secsTo(QDateTime::currentDateTime());
    if (seconds < 60) {
        return QString("Just now");
    }

    const qint64 minutes = seconds / 60;
    if (minutes < 60) {
        return QString("%1 min%2 ago")
            .arg(minutes)
            .arg(minutes == 1 ? "" : "s");
    }

    const qint64 hours = minutes / 60;
    if (hours < 24) {
        return QString("%1 hour%2 ago")
            .arg(hours)
            .arg(hours == 1 ? "" : "s");
    }

    const qint64 days = hours / 24;

    if (days == 1)
        return "Yesterday";

    if (days < 7)
        return QString("%1 days ago").arg(days);

    const qint64 weeks = days / 7;

    if (weeks < 4)
        return QString("%1 week%2 ago")
            .arg(weeks)
            .arg(weeks == 1 ? "" : "s");

    // If a month or more, return full date
    return timestamp.toLocalTime().toString("MMM d, yyyy");
}

QVector<QPointF> HomeWidget::chartSeries()
{
    return m_mode == ChartMode::Market ? this->marketHistory()
                                       : this->portfolioHistory();
}

double HomeWidget::usdToChartCurrency() const
{
    const QString fiat = conf()->get(Config::preferredFiatCurrency).toString();
    if (fiat.compare("USD", Qt::CaseInsensitive) == 0) {
        m_chartCurrency = "USD";
        return 1.0;
    }

    const double ratio = appData()->prices.convert("USD", fiat, 1.0);
    if (ratio > 0) {
        m_chartCurrency = fiat;
        return ratio;
    }

    // No rate for the preferred currency, so the numbers stay in the dollars
    // they arrived as, and say so.
    m_chartCurrency = "USD";
    return 1.0;
}

QVector<QPointF> HomeWidget::marketHistory()
{
    m_chartInFiat = true;
    m_stepStamps.clear();

    QVector<QPointF> series = m_prices->series(priceRangeFor(int(m_range)));
    const double ratio = this->usdToChartCurrency();
    if (ratio != 1.0) {
        for (QPointF &point : series) {
            point.setY(point.y() * ratio);
        }
    }
    return series;
}

QVector<QPointF> HomeWidget::portfolioHistory()
{
    QVector<QPointF> history;
    m_stepStamps.clear();

    if (!m_wallet)
        return history;

    const QList<TransactionRow> rows = m_wallet->history()->getRows();

    QVector<TransactionRow> completed;
    completed.reserve(rows.size());
    for (const auto &tx : rows) {
        if (tx.failed || tx.balanceDelta == 0)
            continue;
        completed.append(tx);
    }

    // What the holding was worth, not how many coins it was: each point is the
    // running balance valued at the XMR price at that moment. Asking for the
    // year series also lets this work from the disk cache.
    m_prices->series(priceRangeFor(int(m_range)));
    m_prices->series(PriceHistory::Range::Year);

    const double usdRatio = this->usdToChartCurrency();
    const double spotRate = this->fiatRate();

    // m_chartInFiat records which unit came out of this: with no price data at
    // all the series falls back to plain XMR, and the axis, card title and hover
    // readout have to agree. Decided before the early return, so an empty wallet
    // still labels its chart.
    m_chartInFiat = m_prices->hasAnyData() || spotRate > 0;
    if (!m_chartInFiat) {
        m_chartCurrency = conf()->get(Config::preferredFiatCurrency).toString();
    }

    if (completed.isEmpty())
        return history;

    // Price a moment: the historical rate where it is known, otherwise today's.
    auto valueAt = [this, usdRatio, spotRate](double whenMs, double balanceXmr) {
        if (balanceXmr <= 0.0) {
            return 0.0;
        }
        const double usd = m_prices->priceAt(whenMs);
        if (usd > 0) {
            return balanceXmr * usd * usdRatio;
        }
        return balanceXmr * (spotRate > 0 ? spotRate : 1.0);
    };

    std::sort(completed.begin(), completed.end(),
              [](const TransactionRow &a, const TransactionRow &b) {
                  return a.timestamp < b.timestamp;
              });

    // Sampled at every moment the value could have changed: each transaction
    // and each known price. The coin count is flat between transactions, but
    // what it is worth is not.
    const double firstMs = double(completed.first().timestamp.toMSecsSinceEpoch());
    const double nowMs = double(QDateTime::currentDateTime().toMSecsSinceEpoch());

    QVector<double> stamps;
    stamps.reserve(completed.size() + 400);
    for (const auto &tx : completed) {
        stamps.append(double(tx.timestamp.toMSecsSinceEpoch()));
    }
    for (const QPointF &sample : m_prices->pricesBetween(firstMs, nowMs)) {
        stamps.append(sample.x());
    }
    stamps.append(nowMs);

    std::sort(stamps.begin(), stamps.end());
    stamps.erase(std::unique(stamps.begin(), stamps.end()), stamps.end());

    // Start from empty, an hour before the first transaction, so the opening
    // deposit reads as a step up rather than as the baseline of the chart.
    history.reserve(stamps.size() + 2);

    // ...and hold that baseline back across the rest of the selected range, so
    // the series is as wide as the window rather than as the account's activity.
    // Only as far back as the wallet has scanned, though: a restore height
    // inside the window is where the line starts.
    const double baselineMs = firstMs - 3600.0 * 1000;
    const qint64 spanMs = this->rangeSpanMs();
    if (spanMs > 0) {
        const double windowStart = qMax(nowMs - double(spanMs), this->historyStartMs());
        if (windowStart < baselineMs) {
            history.append(QPointF(windowStart, 0.0));
        }
    }

    history.append(QPointF(baselineMs, 0.0));

    double runningBalance = 0.0;
    int cursor = 0;
    for (const double when : stamps) {
        // Every transaction at or before this moment is already in the balance.
        bool stepped = false;
        while (cursor < completed.size()
               && double(completed.at(cursor).timestamp.toMSecsSinceEpoch()) <= when) {
            if (!stepped) {
                // Hold the old level up to the transaction, so the change reads
                // as a step rather than a diagonal from the previous sample.
                history.append(QPointF(when - 1.0, valueAt(when, qMax(0.0, runningBalance))));
                // Appended in ascending order, which binary_search relies on.
                m_stepStamps.append(when - 1.0);
                m_stepStamps.append(when);
                stepped = true;
            }
            runningBalance += static_cast<double>(completed.at(cursor).balanceDelta) / 1e12;
            ++cursor;
        }
        history.append(QPointF(when, valueAt(when, qMax(0.0, runningBalance))));
    }

    return history;
}

HomeWidget::~HomeWidget() = default;
