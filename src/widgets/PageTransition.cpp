// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include "PageTransition.h"

#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <QPropertyAnimation>
#include <QStackedWidget>
#include <QWidget>

namespace {
    // Short enough to stay out of the way of rapid navigation.
    constexpr int kDurationMs = 160;
    // Slight travel; enough to give the change direction without feeling loose.
    constexpr int kSlidePx = 14;
}

PageTransition::PageTransition(QStackedWidget *stack, QObject *parent)
    : QObject(parent)
    , m_stack(stack)
{
    connect(m_stack, &QStackedWidget::currentChanged, this, &PageTransition::onCurrentChanged);
}

void PageTransition::onCurrentChanged(int index)
{
    // Settle any in-flight transition, so pages never fight over the effect.
    this->finalize();

    // Pages are added during construction; animating then would fade the
    // window in piecemeal on startup.
    if (!m_stack->isVisible()) {
        return;
    }

    QWidget *page = m_stack->widget(index);
    if (!page) {
        return;
    }

    auto *effect = new QGraphicsOpacityEffect(page);
    effect->setOpacity(0.0);
    page->setGraphicsEffect(effect);

    m_page = page;
    m_effect = effect;
    m_restPos = page->pos();

    auto *fade = new QPropertyAnimation(effect, "opacity");
    fade->setDuration(kDurationMs);
    fade->setStartValue(0.0);
    fade->setEndValue(1.0);
    fade->setEasingCurve(QEasingCurve::OutCubic);

    auto *slide = new QPropertyAnimation(page, "pos");
    slide->setDuration(kDurationMs);
    slide->setStartValue(m_restPos + QPoint(kSlidePx, 0));
    slide->setEndValue(m_restPos);
    slide->setEasingCurve(QEasingCurve::OutCubic);

    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(fade);
    group->addAnimation(slide);
    m_animation = group;

    connect(group, &QAbstractAnimation::finished, this, &PageTransition::finalize);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void PageTransition::finalize()
{
    if (m_animation) {
        // Detach first: stop() emits finished(), which would re-enter here.
        QParallelAnimationGroup *animation = m_animation;
        m_animation = nullptr;
        animation->disconnect(this);
        animation->stop();  // DeleteWhenStopped disposes of it
    }

    if (m_page) {
        // Leaving it in place keeps routing the page through an offscreen pixmap.
        m_page->setGraphicsEffect(nullptr);
        m_page->move(m_restPos);
    }

    m_page = nullptr;
    m_effect = nullptr;
}
