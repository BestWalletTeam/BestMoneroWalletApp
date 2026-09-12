// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_WALLETWIZARD_H
#define BESTWALLET_WALLETWIZARD_H

#include <QWizard>

#include "model/WalletKeysFilesModel.h"
#include "utils/RestoreHeightLookup.h"
#include "utils/config.h"
#include "utils/Seed.h"
#include "constants.h"

enum WizardMode {
    CreateWallet = 0,
    OpenWallet,
    RestoreFromSeed,
    RestoreFromKeys,
    CreateWalletFromDevice
};

enum DeviceType {
    LEDGER = 0,
    TREZOR
};

struct WizardFields {
    QString walletName;
    QString walletDir;
    Seed seed;
    bool showSetSeedPassphrasePage = false;
    bool showSetRestoreHeightPage = false;
    bool showSetSubaddressLookaheadPage = false;
    QString seedOffsetPassphrase;
    QString seedLanguage = constants::seedLanguage;
    QString password;
    QString modeText;
    QString address;
    QString secretViewKey;
    QString secretSpendKey;
    WizardMode mode;
    int restoreHeight = 0;
    Seed::Type seedType;
    DeviceType deviceType;
    QString subaddressLookahead;

    void clearFields() {
        showSetSeedPassphrasePage = false;
        showSetRestoreHeightPage = false;
        showSetSubaddressLookaheadPage = false;
        seedOffsetPassphrase = "";
        password = "";
        address = "";
        secretViewKey = "";
        secretSpendKey = "";
        restoreHeight = 0;
        subaddressLookahead = "";
    }

    WizardFields(): deviceType(DeviceType::LEDGER), mode(WizardMode::CreateWallet),
                    seedType(Seed::POLYSEED), restoreHeight(0) {}
};

class PageMenu;

class WalletWizard : public QWizard
{
    Q_OBJECT

public:
    // The menu page paints its brand mark, so a stylesheet swap alone
    // cannot repaint it -- WindowManager calls this on a theme change.
    void skinChanged();
    enum Page {
        Page_Menu = 0,
        Page_WalletFile,
        Page_CreateWalletSeed,
        Page_SetSeedPassphrase,
        Page_SetPasswordPage,
        Page_SetSubaddressLookahead,
        Page_OpenWallet,
        Page_Network,
        Page_WalletRestoreSeed,
        Page_WalletRestoreKeys,
        Page_SetRestoreHeight,
        Page_HardwareDevice,
        Page_NetworkProxy,
        Page_NetworkWebsocket,
        Page_Plugins
    };

    explicit WalletWizard(QWidget *parent = nullptr);
    void resetFields();

signals:
    void initialNetworkConfigured();
    void showSettings();
    void openWallet(QString path, QString password);

    void createWalletFromDevice(const QString &path, const QString &password, const QString &deviceName, int restoreHeight, const QString &subaddressLookahead);
    void createWalletFromKeys(const QString &path, const QString &password, const QString &address, const QString &viewkey, const QString &spendkey, quint64 restoreHeight, const QString subaddressLookahead = "");
    void createWallet(Seed seed, const QString &path, const QString &password, const QString &seedLanguage, const QString &seedOffset, const QString &subaddressLookahead, bool newWallet);

private slots:
    void onCreateWallet();
    QString helpPage();
    void showHelp();

private:
    WalletKeysFilesModel *m_walletKeysFilesModel;
    PageMenu *m_menuPage = nullptr;
    WizardFields m_wizardFields;
};

#endif // BESTWALLET_WALLETWIZARD_H
