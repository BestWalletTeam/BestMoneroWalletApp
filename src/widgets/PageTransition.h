// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_PAGETRANSITION_H
#define BESTWALLET_PAGETRANSITION_H

#include <QObject>
#include <QPoint>
#include <QPointer>

class QGraphicsOpacityEffect;
class QParallelAnimationGroup;
class QStackedWidget;
class QWidget;

// Fades and slides the incoming page whenever a QStackedWidget changes page.
//
// Listens to currentChanged rather than intercepting the navigation calls, so
// existing call sites keep working untouched. Only one page is animated at a
// time: a new transition finalises the previous one first.
class PageTransition : public QObject
{
    Q_OBJECT

public:
    explicit PageTransition(QStackedWidget *stack, QObject *parent = nullptr);

private slots:
    void onCurrentChanged(int index);

private:
    void finalize();

    QStackedWidget *m_stack;
    QPointer<QWidget> m_page;
    QPointer<QGraphicsOpacityEffect> m_effect;
    QPointer<QParallelAnimationGroup> m_animation;
    QPoint m_restPos;
};

#endif //BESTWALLET_PAGETRANSITION_H
