// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef HOMEWIDGET_H
#define HOMEWIDGET_H

#include <QWidget>
#include <QPointF>
#include <QVector>
#include <QLabel>
#include <QDateTime>

#include <QtCharts/QChartView>
#include <QtCharts/QChart>
#include <QtCharts/QLineSeries>
#include <QtCharts/QAreaSeries>
#include <QtCharts/QScatterSeries>
#include <QtCharts/QDateTimeAxis>
#include <QtCharts/QValueAxis>

#include "plugins/Plugin.h"
#include "Wallet.h"
#include "rows/TransactionRow.h"

namespace Ui {
    class HomeWidget;
}

class PriceHistory;

class HomeWidget : public QWidget
{
    Q_OBJECT

public:
    explicit HomeWidget(Wallet *wallet, QWidget *parent = nullptr);
    ~HomeWidget();

    void addPlugin(Plugin *plugin);
    void aboutToQuit();
    void uiSetup();

    // Chart colours are painted by QtCharts rather than the stylesheet, so
    // they have to be re-applied when the theme changes.
    void skinChanged();

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

signals:
    void showHistory();

private slots:
    void updateBalance(quint64 balance, quint64 spendable);
    void updateConnectionState(int status);
    void onHistoryChanged();
    void onPricesChanged();

private:
    enum class ChartRange { Day, Week, Month, Year, All };
    // What the chart is of. Portfolio is the wallet's own value over time;
    // Market is the XMR price itself, independent of what you hold.
    enum class ChartMode { Market, Portfolio };

    QScopedPointer<Ui::HomeWidget> ui;
    Wallet *m_wallet;
    // Not const: both ask PriceHistory for data, which fetches on demand.
    QVector<QPointF> chartSeries();
    QVector<QPointF> portfolioHistory();
    QVector<QPointF> marketHistory();

    // USD -> the preferred fiat currency, or 1.0 with m_chartCurrency left at
    // USD when no rate for it is held.
    double usdToChartCurrency() const;
    void updateChart();
    void setupXmrChart();
    void setupRangeSelector();
    void setupModeSelector();
    void applyChartTheme();
    QVector<QPointF> pointsForRange(const QVector<QPointF> &all) const;

    // How far back the selected range reaches, in ms. Zero for "All", which is
    // bounded by the data rather than by a span.
    qint64 rangeSpanMs() const;

    // Epoch ms of the earliest moment this wallet can account for: its restore
    // height. Zero when it scanned from the beginning. Negative until resolved.
    double historyStartMs() const;
    mutable double m_historyStartMs = -1.0;
    void updateHover(const QPoint &viewportPos);
    void hideHover();
    void rebuildActivityList(const QList<TransactionRow> &completed);
    QWidget* buildActivityRow(const TransactionRow &tx);

    QString formatXmr(double);
    QString formatRelativeTime(const QDateTime &);

    // XMR -> preferred fiat, or 0 when no rate has arrived yet.
    double fiatRate() const;
    QString fiatSymbol() const;

    QChart *m_xmrChart = nullptr;
    QChartView *m_xmrChartView = nullptr;
    QWidget *m_chartEmptyBox = nullptr;
    QLabel *m_chartEmptyTitle = nullptr;
    QLabel *m_chartEmptySubtitle = nullptr;

    // Straight segments, not splines: a balance series steps, and QSplineSeries
    // overshoots into visible loops at every jump. With a point per price sample
    // the line reads as a curve anyway.
    QLineSeries *m_xmrLine = nullptr;
    QLineSeries *m_xmrUpper = nullptr;
    QLineSeries *m_xmrLower = nullptr;
    QAreaSeries *m_xmrArea = nullptr;

    QDateTimeAxis *m_axisX = nullptr;
    QValueAxis *m_axisY = nullptr;

    ChartRange m_range = ChartRange::Month;
    ChartMode m_mode = ChartMode::Portfolio;
    QVector<QPointF> m_plotted;          // points currently on screen
    // Whether the plotted series is money and, if so, in which currency. The
    // axis, card title and hover readout all label from these.
    mutable bool m_chartInFiat = false;
    mutable QString m_chartCurrency;

    // Timestamps of the balance jumps in the plotted series, ascending. The
    // resampler puts these back verbatim so a deposit stays a step instead of
    // being averaged into a ramp. Empty in market mode, which has no steps.
    QVector<double> m_stepStamps;

    // XMR price history in USD: the market chart plots it directly, and the
    // portfolio chart values each balance point against it. Disk-cached, so
    // both work before -- or without -- a successful request.
    PriceHistory *m_prices = nullptr;

    QScatterSeries *m_hoverMarker = nullptr;
    QFrame *m_hoverTip = nullptr;
    QLabel *m_hoverValue = nullptr;
    QLabel *m_hoverDate = nullptr;
};

#endif //HOMEWIDGET_H
