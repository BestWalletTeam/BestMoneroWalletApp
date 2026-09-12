// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_SWAPPROGRESS_H
#define BESTWALLET_SWAPPROGRESS_H

#include <QWidget>

// The four stages a swap passes through, drawn as a stepper. Painted rather
// than composed from widgets: the rails have to meet the circles exactly and
// take their colour from the step behind them.
class SwapProgress : public QWidget
{
    Q_OBJECT

public:
    explicit SwapProgress(QWidget *parent = nullptr);

    // Positions the stepper from the exchange's status vocabulary.
    void setStatus(const QString &status);

    void setLightTheme(bool light);

    // False when the status maps onto no stage -- a failure or a refund -- so
    // the caller can show the textual status instead of drawing this.
    bool isTrackable() const { return m_trackable; }

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

    // The height the stepper needs to draw without clipping.
    static int naturalHeight();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // Index of the stage in progress; equal to the step count once finished.
    int m_current = 0;
    bool m_trackable = true;
    bool m_light = false;
};

#endif //BESTWALLET_SWAPPROGRESS_H
