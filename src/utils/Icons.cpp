// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include "Icons.h"

#include <QPainter>

Icons* Icons::m_instance(nullptr);

Icons::Icons()
= default;

QIcon Icons::icon(const QString& name)
{
    QIcon icon = m_iconCache.value(name);

    if (!icon.isNull()) {
        return icon;
    }

    icon = QIcon{":/assets/images/" + name};

    m_iconCache.insert(name, icon);
    return icon;
}

void Icons::setInk(const QColor &ink)
{
    if (m_ink == ink) {
        return;
    }

    m_ink = ink;
    m_tintCache.clear();
    m_fittedCache.clear();
}

QRect Icons::inkBounds(const QImage &img)
{
    int minX = img.width(), minY = img.height(), maxX = -1, maxY = -1;
    for (int y = 0; y < img.height(); ++y) {
        for (int x = 0; x < img.width(); ++x) {
            if (qAlpha(img.pixel(x, y)) > 12) {
                minX = qMin(minX, x); maxX = qMax(maxX, x);
                minY = qMin(minY, y); maxY = qMax(maxY, y);
            }
        }
    }
    return maxX < 0 ? QRect() : QRect(QPoint(minX, minY), QPoint(maxX, maxY));
}

QPixmap Icons::fittedPixmap(const QString& name, int size, qreal fill, qreal dpr,
                            const QColor *tint)
{
    // Measured large: at the final size the padding being corrected for is only
    // a pixel or two, and rounding it away defeats the point.
    constexpr int kMeasureSize = 256;
    const QPixmap source = this->icon(name).pixmap(QSize(kMeasureSize, kMeasureSize));
    if (source.isNull()) {
        return source;
    }

    const QRect ink = Icons::inkBounds(source.toImage().convertToFormat(QImage::Format_ARGB32));
    if (ink.isEmpty()) {
        return source;
    }

    QPixmap out{QSize(size, size) * dpr};
    out.setDevicePixelRatio(dpr);
    out.fill(Qt::transparent);

    const qreal scale = size * fill / qMax(ink.width(), ink.height());
    const QSizeF target = QSizeF(ink.size()) * scale;
    const QRectF dest{(size - target.width()) / 2.0,
                      (size - target.height()) / 2.0,
                      target.width(), target.height()};

    QPainter painter{&out};
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawPixmap(dest, source, QRectF(ink));
    if (tint) {
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(QRectF(0, 0, size, size), *tint);
    }
    painter.end();

    return out;
}

QIcon Icons::fitted(const QString& name, int size, qreal fill, qreal dpr)
{
    const QString key = QString("%1|%2|%3|%4|plain").arg(name).arg(size).arg(fill).arg(dpr);
    QIcon cached = m_fittedCache.value(key);
    if (!cached.isNull()) {
        return cached;
    }

    QIcon out{this->fittedPixmap(name, size, fill, dpr, nullptr)};
    m_fittedCache.insert(key, out);
    return out;
}

QIcon Icons::fittedTinted(const QString& name, int size, qreal fill, qreal dpr)
{
    const QString key = QString("%1|%2|%3|%4|ink").arg(name).arg(size).arg(fill).arg(dpr);
    QIcon cached = m_fittedCache.value(key);
    if (!cached.isNull()) {
        return cached;
    }

    QIcon out{this->fittedPixmap(name, size, fill, dpr, &m_ink)};
    m_fittedCache.insert(key, out);
    return out;
}

QPixmap Icons::tintedPixmap(const QString& name, int size)
{
    // Recolour every opaque pixel, keeping the alpha channel, so the glyph shape
    // survives but the colour follows the theme.
    QPixmap source = this->icon(name).pixmap(size, size);
    if (source.isNull()) {
        return source;
    }

    QPixmap out{source.size()};
    out.setDevicePixelRatio(source.devicePixelRatio());
    out.fill(Qt::transparent);

    QPainter painter{&out};
    painter.drawPixmap(0, 0, source);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(out.rect(), m_ink);
    painter.end();

    return out;
}

QIcon Icons::tinted(const QString& name)
{
    QIcon cached = m_tintCache.value(name);
    if (!cached.isNull()) {
        return cached;
    }

    QIcon tinted{this->tintedPixmap(name, 64)};
    m_tintCache.insert(name, tinted);
    return tinted;
}

Icons* Icons::instance()
{
    if (!m_instance) {
        m_instance = new Icons();
    }

    return m_instance;
}
