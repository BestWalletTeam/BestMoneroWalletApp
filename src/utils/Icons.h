// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_ICONS_H
#define BESTWALLET_ICONS_H

#include <QColor>
#include <QIcon>
#include <QHash>
#include <QImage>
#include <QPixmap>
#include <QRect>
#include <QString>

class Icons {
public:
    QIcon icon(const QString& name);

    // Monochrome glyphs ship in a single colour, so they are recoloured for the
    // active theme rather than duplicated per theme.
    void setInk(const QColor &ink);
    QColor ink() const { return m_ink; }
    QIcon tinted(const QString& name);
    QPixmap tintedPixmap(const QString& name, int size);

    // Icons here come from several sources and pad themselves differently inside
    // their own canvas, so sizing by the canvas leaves a row of them looking
    // mismatched. These fit the drawn extent into `fill` of the box instead.
    //
    // `fill` is the share of the box the glyph's longest side takes. Solid
    // shapes need a smaller one than line art to look the same weight.
    QIcon fitted(const QString& name, int size, qreal fill, qreal dpr = 1.0);
    QIcon fittedTinted(const QString& name, int size, qreal fill, qreal dpr = 1.0);

    // Bounds of what an image actually draws, ignoring transparent padding.
    static QRect inkBounds(const QImage &img);

    static Icons* instance();

private:
    Icons();

    static Icons* m_instance;

    QPixmap fittedPixmap(const QString& name, int size, qreal fill, qreal dpr,
                         const QColor *tint);

    QHash<QString, QIcon> m_iconCache;
    QHash<QString, QIcon> m_tintCache;
    QHash<QString, QIcon> m_fittedCache;
    QColor m_ink{Qt::white};

    Q_DISABLE_COPY(Icons)
};

inline Icons* icons()
{
    return Icons::instance();
}

#endif //BESTWALLET_ICONS_H
