// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include "SwapProgress.h"

#include <QFontMetrics>
#include <QHash>
#include <QPainter>
#include <QPainterPath>

namespace {
    const QStringList kSteps = {
        QStringLiteral("Awaiting deposit"),
        QStringLiteral("Confirming"),
        QStringLiteral("Exchanging"),
        QStringLiteral("Sending to you"),
    };

    constexpr int kRadius = 11;
    constexpr int kRailThickness = 2;
    constexpr int kLabelGap = 10;
    constexpr int kLabelHeight = 16;

    // Room for the outermost captions, which would otherwise sit flush against
    // the widget's edges.
    constexpr int kEdgePad = 6;

    // Room for the active stage's ring, stroked with a pen centred on the
    // circle, so half of it would fall outside the widget and be clipped.
    constexpr int kTopPad = 3;
}

SwapProgress::SwapProgress(QWidget *parent)
    : QWidget(parent)
{
}

void SwapProgress::setLightTheme(bool light) {
    if (m_light == light) {
        return;
    }
    m_light = light;
    this->update();
}

void SwapProgress::setStatus(const QString &status) {
    // Anything absent is failed, refunded, expired, or a status the API has
    // added since -- none of which is a point on a line of forward progress.
    // "verifying" is not a stage of its own, so it sits with exchanging.
    static const QHash<QString, int> kStages{
        {QStringLiteral("new"),        0},
        {QStringLiteral("waiting"),    0},
        {QStringLiteral("confirming"), 1},
        {QStringLiteral("exchanging"), 2},
        {QStringLiteral("verifying"),  2},
        {QStringLiteral("sending"),    3},
        {QStringLiteral("finished"),   int(kSteps.size())},
    };

    const auto stage = kStages.constFind(status.toLower());
    const bool trackable = stage != kStages.constEnd();
    const int current = trackable ? *stage : 0;

    if (trackable == m_trackable && current == m_current) {
        return;
    }

    m_trackable = trackable;
    m_current = current;
    this->update();
}

QSize SwapProgress::minimumSizeHint() const {
    return {320, SwapProgress::naturalHeight()};
}

QSize SwapProgress::sizeHint() const {
    return {560, SwapProgress::naturalHeight()};
}

int SwapProgress::naturalHeight() {
    return kTopPad + kRadius * 2 + kLabelGap + kLabelHeight;
}

void SwapProgress::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event)

    if (!m_trackable) {
        return;
    }

    const bool light = m_light;

    const QColor done = light ? QColor("#1F9D53") : QColor("#65cf6a");
    const QColor active = done;
    const QColor pending = light ? QColor("#C6CDD6") : QColor("#3a3b3d");
    const QColor labelDone = light ? QColor("#5A6472") : QColor("#979696");
    const QColor labelPending = light ? QColor("#8B95A3") : QColor("#7c7c7e");

    QPainter painter{this};
    painter.setRenderHint(QPainter::Antialiasing);

    QFont labelFont = this->font();
    labelFont.setPixelSize(11);
    const QFontMetrics metrics{labelFont};

    const int count = kSteps.size();
    // Inset by half the outer captions, so they have somewhere to sit.
    const int inset = kEdgePad + qMax(kRadius, metrics.horizontalAdvance(kSteps.first()) / 2);
    const int insetRight = kEdgePad + qMax(kRadius, metrics.horizontalAdvance(kSteps.last()) / 2);
    const int span = this->width() - inset - insetRight;
    if (span <= 0 || count < 2) {
        return;
    }

    const int cy = kTopPad + kRadius;
    const double step = double(span) / (count - 1);

    // Rails first, so the circles are drawn over their ends and no seam shows.
    for (int i = 0; i < count - 1; ++i) {
        const double x1 = inset + step * i + kRadius;
        const double x2 = inset + step * (i + 1) - kRadius;
        if (x2 <= x1) {
            continue;
        }
        painter.setPen(QPen(i < m_current ? done : pending, kRailThickness, Qt::SolidLine, Qt::RoundCap));
        painter.drawLine(QPointF(x1, cy), QPointF(x2, cy));
    }

    for (int i = 0; i < count; ++i) {
        const double cx = inset + step * i;
        const QRectF circle{cx - kRadius, double(cy - kRadius), kRadius * 2.0, kRadius * 2.0};

        const bool isDone = i < m_current;
        const bool isActive = i == m_current;

        painter.setBrush(Qt::NoBrush);
        if (isDone) {
            painter.setPen(Qt::NoPen);
            painter.setBrush(done);
            painter.drawEllipse(circle);

            QPainterPath tick;
            tick.moveTo(cx - kRadius * 0.42, cy);
            tick.lineTo(cx - kRadius * 0.10, cy + kRadius * 0.34);
            tick.lineTo(cx + kRadius * 0.44, cy - kRadius * 0.32);
            painter.setBrush(Qt::NoBrush);
            painter.setPen(QPen(Qt::white, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.drawPath(tick);
        }
        else if (isActive) {
            // A ring, not a disc: the stage is reached but not achieved.
            painter.setPen(QPen(active, kRailThickness + 1));
            painter.drawEllipse(circle);
            painter.setPen(Qt::NoPen);
            painter.setBrush(active);
            painter.drawEllipse(QPointF(cx, cy), kRadius * 0.36, kRadius * 0.36);
        }
        else {
            painter.setPen(QPen(pending, kRailThickness));
            painter.drawEllipse(circle);
        }

        painter.setFont(labelFont);
        painter.setPen(isDone || isActive ? (isActive ? active : labelDone) : labelPending);

        // The outer captions anchor to the padding; centring them on their own
        // circle would push them past the edge.
        const double top = cy + kRadius + kLabelGap;
        QRectF labelRect{cx - step / 2.0, top, step, double(kLabelHeight)};
        int align = Qt::AlignHCenter | Qt::AlignTop;
        if (i == 0) {
            labelRect = QRectF(kEdgePad, top, step, kLabelHeight);
            align = Qt::AlignLeft | Qt::AlignTop;
        } else if (i == count - 1) {
            labelRect = QRectF(this->width() - kEdgePad - step, top, step, kLabelHeight);
            align = Qt::AlignRight | Qt::AlignTop;
        }

        painter.drawText(labelRect, align,
                         metrics.elidedText(kSteps.at(i), Qt::ElideRight, int(step)));
    }
}
