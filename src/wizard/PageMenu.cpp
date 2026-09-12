// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include "PageMenu.h"
#include "ui_PageMenu.h"

#include <QTimer>
#include <QPainter>

#include "WalletWizard.h"
#include "constants.h"
#include "utils/config.h"
#include "utils/Icons.h"

PageMenu::PageMenu(WizardFields *fields, WalletKeysFilesModel *wallets, QWidget *parent)
        : QWizardPage(parent)
        , ui(new Ui::PageMenu)
        , m_walletKeysFilesModel(wallets)
        , m_fields(fields)
{
    ui->setupUi(this);
    this->setButtonText(QWizard::FinishButton, "Open recent wallet");

    ui->label_version->setText(QString("Best Wallet %1 — by BestMoneroWallet.app").arg(BESTWALLET_VERSION));

    this->applyBrandMark();

    // Only the last word is accented, so it can't be a stylesheet colour.
    ui->brandSubtitle->setText(
        QString("Private. Secure. <span style='color:%1;'>Monero.</span>")
            .arg(constants::accentColor));
}

void PageMenu::applyBrandMark()
{
    ui->brandLogo->setPixmap(this->brandMark(ui->brandLogo->width(),
                                             this->devicePixelRatioF()));
}

QPixmap PageMenu::brandMark(int size, qreal dpr) const
{
    // Rendered here rather than set on the label, so the mark is scaled at
    // device resolution instead of by QLabel.
    QPixmap out{QSize(size, size) * dpr};
    out.setDevicePixelRatio(dpr);
    out.fill(Qt::transparent);

    QPainter painter{&out};
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // The variant that reads on this skin's background: the primary artwork's
    // lower half is near-white, which the light skin's wizard would swallow.
    const bool light =
        conf()->get(Config::skin).toString() == constants::skinNativeWhite;
    const QPixmap mark{light ? ":/assets/images/appicons/app_logo_light.png"
                             : ":/assets/images/appicons/app_logo.png"};
    if (mark.isNull()) {
        painter.end();
        return out;
    }

    // The source carries its own transparent margin, so sizing by the canvas
    // would leave the mark small and off-centre.
    const QRect ink =
        Icons::inkBounds(mark.toImage().convertToFormat(QImage::Format_ARGB32));
    const QRect region = ink.isEmpty() ? mark.rect() : ink;

    // Fitted rather than filled, so a non-square asset is not stretched.
    const qreal box = size * 0.86;
    const QSizeF fitted = QSizeF(region.size()).scaled(box, box, Qt::KeepAspectRatio);
    const QRectF dest{(size - fitted.width()) / 2, (size - fitted.height()) / 2,
                      fitted.width(), fitted.height()};

    painter.drawPixmap(dest, mark, QRectF(region));
    painter.end();

    return out;
}

void PageMenu::initializePage() {
    if (m_walletKeysFilesModel->rowCount() > 0) {
        ui->radioOpen->setChecked(true);
    } else {
        ui->radioCreate->setChecked(true);
    }

    QTimer::singleShot(0, [this]{
        wizard()->button(QWizard::NextButton)->setFocus();
    });

    // Don't show setup wizard again
    conf()->set(Config::firstRun, false);
}

int PageMenu::nextId() const {
    if (ui->radioCreate->isChecked())
        return WalletWizard::Page_CreateWalletSeed;
    if (ui->radioOpen->isChecked())
        return WalletWizard::Page_OpenWallet;
    if (ui->radioSeed->isChecked())
        return WalletWizard::Page_WalletRestoreSeed;
    if (ui->radioViewOnly->isChecked())
        return WalletWizard::Page_WalletRestoreKeys;
    if (ui->radioCreateFromDevice->isChecked())
        return WalletWizard::Page_HardwareDevice;
    return 0;
}

bool PageMenu::validatePage() {
    m_fields->clearFields();

    if (ui->radioCreate->isChecked()) {
        m_fields->mode = WizardMode::CreateWallet;
        m_fields->modeText = "Create wallet";
    }
    if (ui->radioOpen->isChecked()) {
        m_fields->mode = WizardMode::OpenWallet;
        m_fields->modeText = "Open wallet";
    }
    if (ui->radioSeed->isChecked()) {
        m_fields->mode = WizardMode::RestoreFromSeed;
        m_fields->modeText = "Restore wallet";
    }
    if (ui->radioViewOnly->isChecked()) {
        m_fields->mode = WizardMode::RestoreFromKeys;
        m_fields->modeText = "Restore wallet";
    }
    if (ui->radioCreateFromDevice->isChecked()) {
        m_fields->mode = WizardMode::CreateWalletFromDevice;
        m_fields->modeText = "Create from hardware device";
    }

    return true;
}
