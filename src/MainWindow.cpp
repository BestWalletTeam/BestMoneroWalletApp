// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#include "MainWindow.h"

#include <QProgressBar>
#include "ui_MainWindow.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QCheckBox>
#include <QDesktopServices>
#include <QGraphicsOpacityEffect>
#include <QPainter>

#include "constants.h"
#include "dialog/AddressCheckerIndexDialog.h"
#include "dialog/BalanceDialog.h"
#include "dialog/DebugInfoDialog.h"
#include "dialog/HistoryExportDialog.h"
#include "dialog/PasswordDialog.h"
#include "dialog/TxBroadcastDialog.h"
#include "dialog/TxConfAdvDialog.h"
#include "dialog/TxConfDialog.h"
#include "dialog/TxImportDialog.h"
#include "dialog/TxInfoDialog.h"
#include "dialog/TxPoolViewerDialog.h"
#include "dialog/ViewOnlyDialog.h"
#include "dialog/WalletInfoDialog.h"
#include "dialog/WalletCacheDebugDialog.h"
#include "libwalletqt/AddressBook.h"
#include "libwalletqt/rows/CoinsInfo.h"
#include "libwalletqt/rows/Output.h"
#include "libwalletqt/TransactionHistory.h"
#include "model/AddressBookModel.h"
#include "plugins/PluginRegistry.h"
#include "utils/AppData.h"
#include "utils/AsyncTask.h"
#include "utils/ColorScheme.h"
#include "utils/Icons.h"
#include "utils/TorManager.h"
#include "utils/WebsocketNotifier.h"
#include "widgets/PageTransition.h"

#include "wallet/wallet_errors.h"

#ifdef WITH_SCANNER
#include "wizard/offline_tx_signing/OfflineTxSigningWizard.h"
#include "qrcode/scanner/URDialog.h"
#endif

#ifdef CHECK_UPDATES
#include "utils/updater/UpdateDialog.h"
#endif

MainWindow::MainWindow(WindowManager *windowManager, Wallet *wallet, QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_windowManager(windowManager)
    , m_wallet(wallet)
    , m_nodes(new Nodes(this, wallet))
    , m_rpc(new DaemonRpc(this, ""))
{
    ui->setupUi(this);

    // Ensure the destructor is called after closeEvent()
    setAttribute(Qt::WA_DeleteOnClose);

    m_splashDialog = new SplashDialog(this);
    m_accountSwitcherDialog = new AccountSwitcherDialog(m_wallet, this);

#ifdef CHECK_UPDATES
    m_updater = QSharedPointer<Updater>(new Updater(this));
#endif

    this->restoreGeo();

    this->initStatusBar();
    this->initLeftbar();
    this->initPlugins();
    this->initWidgets();
    this->initMenu();
    this->initOffline();
    this->initWalletContext();
    emit uiSetup();

    this->onOfflineMode(conf()->get(Config::offlineMode).toBool());
    conf()->set(Config::restartRequired, false);
    
    // Websocket notifier
#ifdef CHECK_UPDATES
    connect(websocketNotifier(), &WebsocketNotifier::UpdatesReceived, m_updater.data(), &Updater::wsUpdatesReceived);
#endif

    websocketNotifier()->emitCache(); // Get cached data

    connect(m_windowManager, &WindowManager::websocketStatusChanged, this, &MainWindow::onWebsocketStatusChanged);
    this->onWebsocketStatusChanged(!conf()->get(Config::disableWebsocket).toBool());

    connect(m_windowManager, &WindowManager::proxySettingsChanged, this, &MainWindow::onProxySettingsChangedConnect);
    connect(m_windowManager, &WindowManager::updateBalance, m_wallet, &Wallet::updateBalance);
    connect(m_windowManager, &WindowManager::offlineMode, this, &MainWindow::onOfflineMode);
    connect(m_windowManager, &WindowManager::manualFeeSelectionEnabled, this, &MainWindow::onManualFeeSelectionEnabled);
    connect(m_windowManager, &WindowManager::subtractFeeFromAmountEnabled, this, &MainWindow::onSubtractFeeFromAmountEnabled);

    connect(torManager(), &TorManager::connectionStateChanged, this, &MainWindow::onTorConnectionStateChanged);
    this->onTorConnectionStateChanged(torManager()->torConnected);


#ifdef CHECK_UPDATES
    connect(m_updater.data(), &Updater::updateAvailable, this, &MainWindow::showUpdateNotification);
#endif

    ColorScheme::updateFromWidget(this);
    QTimer::singleShot(1, [this]{this->updateWidgetIcons();});

    // Timers
    connect(&m_updateBytes, &QTimer::timeout, this, &MainWindow::updateNetStats);
    connect(&m_txTimer, &QTimer::timeout, [this]{
        m_statusLabelStatus->setText("Constructing transaction" + this->statusDots());
    });

    conf()->set(Config::firstRun, false);

    this->onWalletOpened();

    connect(&appData()->prices, &Prices::fiatPricesUpdated, this, &MainWindow::updateBalance);
    connect(&appData()->prices, &Prices::cryptoPricesUpdated, this, &MainWindow::updateBalance);

    connect(m_windowManager->eventFilter, &EventFilter::userActivity, this, &MainWindow::userActivity);
    connect(&m_checkUserActivity, &QTimer::timeout, this, &MainWindow::checkUserActivity);
    m_checkUserActivity.setInterval(5000);
    m_checkUserActivity.start();
}

void MainWindow::initStatusBar() {
#if defined(Q_OS_WIN)
    // No separators between statusbar widgets
    this->statusBar()->setStyleSheet("QStatusBar::item {border: None;}");
#endif

    this->statusBar()->setFixedHeight(38);
    this->statusBar()->setContentsMargins(18, 0, 14, 0);
    this->statusBar()->setSizeGripEnabled(false);
    if (QLayout *bar = this->statusBar()->layout()) {
        bar->setSpacing(6);
    }

    // Hidden until there is something to report.
    m_statusSyncBar = new QProgressBar(this);
    m_statusSyncBar->setObjectName("statusSyncBar");
    m_statusSyncBar->setRange(0, 100);
    m_statusSyncBar->setTextVisible(false);
    m_statusSyncBar->setFixedSize(120, 8);
    m_statusSyncBar->hide();
    this->statusBar()->addWidget(m_statusSyncBar);

    m_statusLabelStatus = new QLabel("Ready", this);
    m_statusLabelStatus->setObjectName("statusText");
    m_statusLabelStatus->setTextInteractionFlags(Qt::TextSelectableByMouse);
    this->statusBar()->addWidget(m_statusLabelStatus);

    m_statusLabelNetStats = new QLabel("", this);
    m_statusLabelNetStats->setObjectName("statusNetStats");
    m_statusLabelNetStats->setTextInteractionFlags(Qt::TextSelectableByMouse);
    this->statusBar()->addWidget(m_statusLabelNetStats);

    m_statusUpdateAvailable = new QPushButton(this);
    m_statusUpdateAvailable->setFlat(true);
    m_statusUpdateAvailable->setCursor(Qt::PointingHandCursor);
    m_statusUpdateAvailable->setIcon(this->statusGlyph("tab_party.png", false));
    m_statusUpdateAvailable->setIconSize(QSize(kStatusBarIconSize, kStatusBarIconSize));
    m_statusUpdateAvailable->hide();
    this->statusBar()->addPermanentWidget(m_statusUpdateAvailable);

    m_statusLabelBalance = new ClickableLabel(this);
    m_statusLabelBalance->setObjectName("statusBalance");
    m_statusLabelBalance->setText(this->formatStatusBalance("0", "XMR"));
    m_statusLabelBalance->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_statusLabelBalance->setCursor(Qt::PointingHandCursor);
    this->statusBar()->addPermanentWidget(m_statusLabelBalance);
    connect(m_statusLabelBalance, &ClickableLabel::clicked, this, &MainWindow::showBalanceDialog);

    // Separates the wallet's headline figure from the action icons.
    auto *statusSeparator = new QFrame(this);
    statusSeparator->setObjectName("statusSeparator");
    statusSeparator->setFrameShape(QFrame::VLine);
    statusSeparator->setFixedSize(1, 18);
    this->statusBar()->addPermanentWidget(statusSeparator);

    m_statusBtnConnectionStatusIndicator = new StatusBarButton(this->statusDot("red-circle.svg"), "Connection status", this);
    connect(m_statusBtnConnectionStatusIndicator, &StatusBarButton::clicked, [this](){
        this->onShowSettingsPage(Settings::Pages::NETWORK);
    });
    this->statusBar()->addPermanentWidget(m_statusBtnConnectionStatusIndicator);
    this->onConnectionStatusChanged(Wallet::ConnectionStatus_Disconnected);

    m_statusAccountSwitcher = new StatusBarButton(this->statusGlyph("status_user.svg", true), "Account switcher", this);
    connect(m_statusAccountSwitcher, &StatusBarButton::clicked, this, &MainWindow::showAccountSwitcherDialog);
    this->statusBar()->addPermanentWidget(m_statusAccountSwitcher);

    m_statusBtnPassword = new StatusBarButton(this->statusGlyph("status_lock.svg", true), "Password", this);
    connect(m_statusBtnPassword, &StatusBarButton::clicked, this, &MainWindow::showPasswordDialog);
    this->statusBar()->addPermanentWidget(m_statusBtnPassword);

    m_statusBtnPreferences = new StatusBarButton(this->statusGlyph("status_settings.svg", true), "Settings", this);
    connect(m_statusBtnPreferences, &StatusBarButton::clicked, this, &MainWindow::menuSettingsClicked);
    this->statusBar()->addPermanentWidget(m_statusBtnPreferences);

    m_statusBtnSeed = new StatusBarButton(this->statusGlyph("status_warning.svg", true), "Seed", this);
    connect(m_statusBtnSeed, &StatusBarButton::clicked, this, &MainWindow::showSeedDialog);
    this->statusBar()->addPermanentWidget(m_statusBtnSeed);

    m_statusBtnProxySettings = new StatusBarButton(this->statusGlyph("status_tor.svg", true), "Proxy settings", this);
    connect(m_statusBtnProxySettings, &StatusBarButton::clicked, this, &MainWindow::menuProxySettingsClicked);
    this->statusBar()->addPermanentWidget(m_statusBtnProxySettings);
    this->onProxySettingsChanged();

    m_statusBtnHwDevice = new StatusBarButton(this->hardwareDevicePairedIcon(), this->getHardwareDevice(), this);
    connect(m_statusBtnHwDevice, &StatusBarButton::clicked, this, &MainWindow::menuHwDeviceClicked);
    this->statusBar()->addPermanentWidget(m_statusBtnHwDevice);
    m_statusBtnHwDevice->hide();

    this->applyThemedIcons();
}

void MainWindow::initPlugins() {
    const QStringList enabledPlugins = conf()->get(Config::enabledPlugins).toStringList();

    for (const auto& plugin_creator : PluginRegistry::getPluginCreators()) {
        Plugin* plugin = plugin_creator();

        if (!PluginRegistry::getInstance().isPluginEnabled(plugin->id())) {
            continue;
        }

        // Skip crowdfunding plugin
        if (plugin->id() == "crowdfunding")
            continue;

        qDebug() << "Initializing plugin: " << plugin->id();
        plugin->initialize(m_wallet, this);
        connect(plugin, &Plugin::setStatusText, this, &MainWindow::setStatusText);
        connect(plugin, &Plugin::fillSendTab, this, &MainWindow::fillSendTab);
        connect(plugin, &Plugin::showHistoryTab, this, [this]{
            ui->navigationStack->setCurrentWidget(m_historyWidget);
        });
        connect(this, &MainWindow::updateIcons, plugin, &Plugin::skinChanged);
        connect(this, &MainWindow::aboutToQuit, plugin, &Plugin::aboutToQuit);
        connect(this, &MainWindow::uiSetup, plugin, &Plugin::uiSetup);

        m_plugins.append(plugin);
    }

    std::sort(m_plugins.begin(), m_plugins.end(), [](Plugin *a, Plugin *b) {
        return a->idx() < b->idx();
    });
}

void MainWindow::initWidgets() {
    // [History]
    m_historyWidget = new HistoryWidget(m_wallet, this);
    //ui->historyWidgetLayout->addWidget(m_historyWidget);
    ui->navigationStack->addWidget(m_historyWidget);
    connect(m_historyWidget, &HistoryWidget::viewOnBlockExplorer, this, &MainWindow::onViewOnBlockExplorer);
    connect(m_historyWidget, &HistoryWidget::resendTransaction, this, &MainWindow::onResendTransaction);

    // [Send]
    m_sendWidget = new SendWidget(m_wallet, this);
    //ui->sendWidgetLayout->addWidget(m_sendWidget);
    ui->navigationStack->addWidget(m_sendWidget);

    // [Receive]
    m_receiveWidget = new ReceiveWidget(m_wallet, this);
    //ui->receiveWidgetLayout->addWidget(m_receiveWidget);
    ui->navigationStack->addWidget(m_receiveWidget);
    connect(m_receiveWidget, &ReceiveWidget::showTransactions, [this](const QString &text) {
        m_historyWidget->setSearchText(text);
        ui->navigationStack->setCurrentWidget(m_historyWidget);
    });

    // [Swap]
    m_swapWidget = new SwapWidget(m_wallet, this);
    ui->navigationStack->addWidget(m_swapWidget);
    connect(m_swapWidget, &SwapWidget::setStatusText, this, &MainWindow::setStatusText);
    // fillSendTab has no room for an amount, which a swap deposit carries.
    connect(m_swapWidget, &SwapWidget::sendFromWallet, this,
            [this](const QString &address, const QString &description, double amount) {
        m_sendWidget->fill(address, description, amount);
        ui->navigationStack->setCurrentWidget(m_sendWidget);
    });

    // [Coins]
    m_coinsWidget = new CoinsWidget(m_wallet, this);
    //ui->coinsWidgetLayout->addWidget(m_coinsWidget);
    ui->navigationStack->addWidget(m_coinsWidget);

    // [Contacts]
    m_contactsWidget = new ContactsWidget(m_wallet, this);
    //ui->contactsWidgetLayout->addWidget(m_contactsWidget);
    ui->navigationStack->addWidget(m_contactsWidget);
    connect(m_contactsWidget, &ContactsWidget::fill, [this](const QString &address, const QString &description){
        m_sendWidget->fill(address, description, 0, true);
        ui->navigationStack->setCurrentWidget(m_contactsWidget);
    });

    // [Notes]
    // ui->notes->setPlainText(m_wallet->getCacheAttribute("wallet.notes"));
    // connect(ui->notes, &QPlainTextEdit::textChanged, [this] {
    //    m_wallet->setCacheAttribute("wallet.notes", ui->notes->toPlainText());
    // });

    // [Plugins..]
    for (auto* plugin : m_plugins) {
        if (!plugin->hasParent()) {
            qDebug() << "Adding widget: " << plugin->displayName();

            if (plugin->insertFirst()) {
                ui->navigationStack->insertWidget(0, plugin->tab());
                //ui->tabWidget->insertTab(0, plugin->tab(), icons()->icon(plugin->icon()), plugin->displayName());
            } else {
                ui->navigationStack->addWidget(plugin->tab());
                //ui->tabWidget->addTab(plugin->tab(), icons()->icon(plugin->icon()), plugin->displayName());
            }

            for (auto* child : m_plugins) {
                if (child->hasParent() && child->parent() == plugin->id()) {
                    plugin->addSubPlugin(child);
                }
            }
        }
    }

    // ui->frame_coinControl->setVisible(false);
    // connect(ui->btn_resetCoinControl, &QPushButton::clicked, [this]{
    //    m_wallet->setSelectedInputs({});
    // });

    m_walletUnlockWidget = new WalletUnlockWidget(this, m_wallet);
    m_walletUnlockWidget->setWalletName(this->walletName());
    ui->walletUnlockLayout->addWidget(m_walletUnlockWidget);

    connect(m_walletUnlockWidget, &WalletUnlockWidget::closeWallet, this, &MainWindow::close);
    connect(m_walletUnlockWidget, &WalletUnlockWidget::unlockWallet, this, &MainWindow::unlockWallet);

    ui->navigationStack->setCurrentIndex(0);
    ui->stackedWidget->setCurrentIndex(0);

    // setCurrentIndex only emits on a real change, and the pages shuffle as they
    // are added, so the opening page may never reach onNavigationChanged.
    this->onNavigationChanged(ui->navigationStack->currentIndex());
}

void MainWindow::initLeftbar()
{
    ui->portfolioWidget->installEventFilter(this);
    ui->sendWidget->installEventFilter(this);
    ui->receiveWidget->installEventFilter(this);
    ui->swapWidget->installEventFilter(this);
    ui->historyWidget->installEventFilter(this);

    ui->portfolioButton->installEventFilter(this);
    ui->sendButton->installEventFilter(this);
    ui->receiveButton->installEventFilter(this);
    ui->swapButton->installEventFilter(this);
    ui->historyButton->installEventFilter(this);

    // Plain QWidgets ignore a stylesheet background unless this is set.
    for (QWidget *row : {static_cast<QWidget*>(ui->portfolioWidget),
                         static_cast<QWidget*>(ui->sendWidget),
                         static_cast<QWidget*>(ui->receiveWidget),
                         static_cast<QWidget*>(ui->swapWidget),
                         static_cast<QWidget*>(ui->historyWidget)}) {
        row->setAttribute(Qt::WA_StyledBackground, true);
    }

    this->updateLeftbarLogo();

    connect(ui->navigationStack, &QStackedWidget::currentChanged, this, &MainWindow::onNavigationChanged);
    new PageTransition(ui->navigationStack, this);
}

void MainWindow::initMenu() {
    // TODO: Rename actions to follow style
    // [File]
    connect(ui->actionOpen,        &QAction::triggered, this, &MainWindow::menuOpenClicked);
    connect(ui->actionNew_Restore, &QAction::triggered, this, &MainWindow::menuNewRestoreClicked);
    connect(ui->actionLock,        &QAction::triggered, this, &MainWindow::lockWallet);
    connect(ui->actionClose,       &QAction::triggered, this, &MainWindow::menuWalletCloseClicked); // Close current wallet
    connect(ui->actionQuit,        &QAction::triggered, this, &MainWindow::menuQuitClicked);        // Quit application
    connect(ui->actionSettings,    &QAction::triggered, this, &MainWindow::menuSettingsClicked);

    // [File] -> [Recently open]
    m_clearRecentlyOpenAction = new QAction("Clear history", ui->menuFile);
    connect(m_clearRecentlyOpenAction, &QAction::triggered, this, &MainWindow::menuClearHistoryClicked);

    // [Wallet]
    connect(ui->actionInformation,  &QAction::triggered, this, &MainWindow::showWalletInfoDialog);
    connect(ui->actionAccount,      &QAction::triggered, this, &MainWindow::showAccountSwitcherDialog);
    connect(ui->actionPassword,     &QAction::triggered, this, &MainWindow::showPasswordDialog);
    connect(ui->actionSeed,         &QAction::triggered, this, &MainWindow::showSeedDialog);
    connect(ui->actionKeys,         &QAction::triggered, this, &MainWindow::showKeysDialog);
    connect(ui->actionViewOnly,     &QAction::triggered, this, &MainWindow::showViewOnlyDialog);

    // [Wallet] -> [Advanced]
    connect(ui->actionStore_wallet,          &QAction::triggered, this, &MainWindow::tryStoreWallet);
    connect(ui->actionUpdate_balance,        &QAction::triggered, [this]{m_wallet->updateBalance();});
    connect(ui->actionRefresh_tabs,          &QAction::triggered, [this]{m_wallet->refreshModels();});
    connect(ui->actionRescan_spent,          &QAction::triggered, this, &MainWindow::rescanSpent);
    connect(ui->actionWallet_cache_debug,    &QAction::triggered, this, &MainWindow::showWalletCacheDebugDialog);
    connect(ui->actionTxPoolViewer,          &QAction::triggered, this, &MainWindow::showTxPoolViewerDialog);

    // [Wallet] -> [History]
    connect(ui->actionExport_CSV, &QAction::triggered, this, &MainWindow::onExportHistoryCSV);
    connect(ui->actionImportHistoryCSV, &QAction::triggered, this, &MainWindow::onImportHistoryDescriptionsCSV);

    // [View]
    m_tabShowHideSignalMapper = new QSignalMapper(this);
    connect(ui->actionShow_Searchbar, &QAction::toggled, this, &MainWindow::toggleSearchbar);
    ui->actionShow_Searchbar->setChecked(conf()->get(Config::showSearchbar).toBool());

    // Show/Hide Coins
    // connect(ui->actionShow_Coins, &QAction::triggered, m_tabShowHideSignalMapper, QOverload<>::of(&QSignalMapper::map));
    // m_tabShowHideMapper["Coins"] = new ToggleTab(ui->tabCoins, "Coins", "Coins", ui->actionShow_Coins, this);
    // m_tabShowHideSignalMapper->setMapping(ui->actionShow_Coins, "Coins");

    // // Show/Hide Contacts
    // connect(ui->actionShow_Contacts, &QAction::triggered, m_tabShowHideSignalMapper, QOverload<>::of(&QSignalMapper::map));
    // m_tabShowHideMapper["Contacts"] = new ToggleTab(ui->tabContacts, "Contacts", "Contacts", ui->actionShow_Contacts, this);
    // m_tabShowHideSignalMapper->setMapping(ui->actionShow_Contacts, "Contacts");

    // Show/Hide Notes
    // connect(ui->actionShow_Notes, &QAction::triggered, m_tabShowHideSignalMapper, QOverload<>::of(&QSignalMapper::map));
    // m_tabShowHideMapper["Notes"] = new ToggleTab(ui->tabNotes, "Notes", "Notes", ui->actionShow_Notes, this);
    // m_tabShowHideSignalMapper->setMapping(ui->actionShow_Notes, "Notes");

    // Show/Hide Plugins..
    for (const auto &plugin : m_plugins) {
        if (plugin->parent() != "") {
            continue;
        }

        auto* pluginAction = new QAction(plugin->displayName(), this);
        ui->menuView->insertAction(plugin->insertFirst() ? ui->actionPlaceholderBegin : ui->actionPlaceholderEnd, pluginAction);
        connect(pluginAction, &QAction::triggered, m_tabShowHideSignalMapper, QOverload<>::of(&QSignalMapper::map));
        m_tabShowHideMapper[plugin->displayName()] = new ToggleTab(plugin->tab(), plugin->displayName(), plugin->displayName(), pluginAction, this);
        m_tabShowHideSignalMapper->setMapping(pluginAction, plugin->displayName());
    }
    ui->actionPlaceholderBegin->setVisible(false);
    ui->actionPlaceholderEnd->setVisible(false);

    QStringList enabledTabs = conf()->get(Config::enabledTabs).toStringList();
    for (const auto &key: m_tabShowHideMapper.keys()) {
        const auto toggleTab = m_tabShowHideMapper.value(key);
        bool show = enabledTabs.contains(key);

        toggleTab->menuAction->setText(toggleTab->name);
        toggleTab->menuAction->setCheckable(true);
        toggleTab->menuAction->setChecked(show);
        //ui->tabWidget->setTabVisible(ui->tabWidget->indexOf(toggleTab->tab), show);
    }
    connect(m_tabShowHideSignalMapper, &QSignalMapper::mappedString, this, &MainWindow::menuToggleTabVisible);

    // [Tools]
    connect(ui->actionSignVerify,                  &QAction::triggered, this, &MainWindow::menuSignVerifyClicked);
    connect(ui->actionVerifyTxProof,               &QAction::triggered, this, &MainWindow::menuVerifyTxProof);
    connect(ui->actionKeyImageSync,                &QAction::triggered, this, &MainWindow::showKeyImageSyncWizard);
    connect(ui->actionLoadSignedTxFromFile,        &QAction::triggered, this, &MainWindow::loadSignedTx);
    connect(ui->actionLoadSignedTxFromText,        &QAction::triggered, this, &MainWindow::loadSignedTxFromText);
    connect(ui->actionImport_transaction,          &QAction::triggered, this, &MainWindow::importTransaction);
    connect(ui->actionTransmitOverUR,              &QAction::triggered, this, &MainWindow::showURDialog);
    connect(ui->actionPay_to_many,                 &QAction::triggered, this, &MainWindow::payToMany);
    connect(ui->actionAddress_checker,             &QAction::triggered, this, &MainWindow::showAddressChecker);
    connect(ui->actionCreateDesktopEntry,          &QAction::triggered, this, &MainWindow::onCreateDesktopEntry);

    if (m_wallet->viewOnly()) {
        ui->actionKeyImageSync->setText("Key image sync");
    }

    // TODO: Allow creating desktop entry on Windows and Mac
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
    ui->actionCreateDesktopEntry->setDisabled(true);
#endif

#ifndef SELF_CONTAINED
    ui->actionCreateDesktopEntry->setVisible(false);
#endif

    // [Help]
    connect(ui->actionAbout,             &QAction::triggered, this, &MainWindow::menuAboutClicked);
#if defined(CHECK_UPDATES)
    connect(ui->actionCheckForUpdates,   &QAction::triggered, this, &MainWindow::showUpdateDialog);
#else
    ui->actionCheckForUpdates->setVisible(false);
#endif

    connect(ui->actionOfficialWebsite,   &QAction::triggered, [this](){QDesktopServices::openUrl(QUrl(constants::websiteUrl));});
    // connect(ui->actionDocumentation,     &QAction::triggered, this, &MainWindow::onShowDocumentation);
    // connect(ui->actionReport_bug,        &QAction::triggered, this, &MainWindow::onReportBug);
    connect(ui->actionShow_debug_info,   &QAction::triggered, this, &MainWindow::showDebugInfo);


    // Setup shortcuts
    ui->actionStore_wallet->setShortcut(QKeySequence("Ctrl+S"));
    ui->actionRefresh_tabs->setShortcut(QKeySequence("Ctrl+R"));
    ui->actionOpen->setShortcut(QKeySequence("Ctrl+O"));
    ui->actionNew_Restore->setShortcut(QKeySequence("Ctrl+N"));
    ui->actionLock->setShortcut(QKeySequence("Ctrl+L"));
    ui->actionClose->setShortcut(QKeySequence("Ctrl+W"));
    ui->actionShow_debug_info->setShortcut(QKeySequence("Ctrl+D"));
    ui->actionSettings->setShortcut(QKeySequence("Ctrl+Alt+S"));
    ui->actionUpdate_balance->setShortcut(QKeySequence("Ctrl+U"));
    ui->actionShow_Searchbar->setShortcut(QKeySequence("Ctrl+F"));
    ui->actionDocumentation->setShortcut(QKeySequence("F1"));
}

void MainWindow::initOffline() {
    // TODO: check if we have any cameras available

    ui->btn_help->setFocusPolicy(Qt::NoFocus);
    ui->btn_viewOnlyDetails->setFocusPolicy(Qt::NoFocus);
    ui->btn_checkAddress->setFocusPolicy(Qt::NoFocus);
    ui->btn_signTransaction->setFocusPolicy(Qt::StrongFocus);
    ui->btn_signTransaction->setFocus();

    connect(ui->btn_help, &QPushButton::clicked, [this] {
        windowManager()->showDocs(this, "offline_tx_signing");
    });
    connect(ui->btn_viewOnlyDetails, &QPushButton::clicked, [this] {
         this->showViewOnlyDialog();
    });
    connect(ui->btn_checkAddress, &QPushButton::clicked, [this]{
        AddressCheckerIndexDialog dialog{m_wallet, this};
        dialog.exec();
    });
    connect(ui->btn_signTransaction, &QPushButton::clicked, [this] {
        this->showKeyImageSyncWizard();
    });

    switch (conf()->get(Config::offlineTxSigningMethod).toInt()) {
        case Config::OTSMethod::FileTransfer:
            ui->radio_airgapFiles->setChecked(true);
            break;
        default:
            ui->radio_airgapUR->setChecked(true);
    }

    // We can't use rich text for radio buttons
    connect(ui->label_airgapUR, &ClickableLabel::clicked, [this] {
        ui->radio_airgapUR->setChecked(true);
    });
    connect(ui->label_airgapFiles, &ClickableLabel::clicked, [this] {
        ui->radio_airgapFiles->setChecked(true);
    });

    connect(ui->radio_airgapFiles, &QCheckBox::toggled, [this] (bool checked){
        if (checked) {
            conf()->set(Config::offlineTxSigningMethod, Config::OTSMethod::FileTransfer);
        }
    });
    connect(ui->radio_airgapUR, &QCheckBox::toggled, [this](bool checked) {
        if (checked) {
            conf()->set(Config::offlineTxSigningMethod, Config::OTSMethod::UnifiedResources);
        }
    });
}

void MainWindow::initWalletContext() {
    connect(m_wallet, &Wallet::balanceUpdated,           this, &MainWindow::onBalanceUpdated);
    connect(m_wallet, &Wallet::syncStatus,               this, &MainWindow::onSyncStatus);
    connect(m_wallet, &Wallet::transactionCreated,       this, &MainWindow::onTransactionCreated);
    connect(m_wallet, &Wallet::transactionCommitted,     this, &MainWindow::onTransactionCommitted);
    connect(m_wallet, &Wallet::initiateTransaction,      this, &MainWindow::onInitiateTransaction);
    connect(m_wallet, &Wallet::keysCorrupted,            this, &MainWindow::onKeysCorrupted);
    connect(m_wallet, &Wallet::selectedInputsChanged,    this, &MainWindow::onSelectedInputsChanged);
    connect(m_wallet, &Wallet::txPoolBacklog,            this, &MainWindow::onTxPoolBacklog);

    // Wallet
    connect(m_wallet, &Wallet::connectionStatusChanged, [this](int status){
        // Order is important, first inform UI about a potential disconnect, then reconnect
        this->onConnectionStatusChanged(status);
        m_nodes->autoConnect();
    });
    connect(m_wallet, &Wallet::currentSubaddressAccountChanged, this, &MainWindow::updateTitle);
    connect(m_wallet, &Wallet::walletPassphraseNeeded, this, &MainWindow::onWalletPassphraseNeeded);

    connect(m_wallet, &Wallet::unconfirmedMoneyReceived, this, [this](const QString &txId, uint64_t amount){
       if (m_wallet->isSynchronized() && !m_locked) {
           auto notify = QString("%1 XMR (pending)").arg(WalletManager::displayAmount(amount, false));
           m_windowManager->notify("Payment received", notify, 5000);
       }
    });
    connect(m_wallet, &Wallet::moneyReceived, this, [this](const QString &txId, uint64_t amount, bool coinbase){
        if (m_wallet->isSynchronized() && !m_locked && coinbase) {
            auto notify = QString("%1 XMR").arg(WalletManager::displayAmount(amount, false));
            m_windowManager->notify("Mining payment received", notify, 5000);
        }
    });

    // Device
    connect(m_wallet, &Wallet::deviceButtonRequest, this, &MainWindow::onDeviceButtonRequest);
    connect(m_wallet, &Wallet::deviceButtonPressed, this, &MainWindow::onDeviceButtonPressed);
    connect(m_wallet, &Wallet::deviceError,         this, &MainWindow::onDeviceError);
    
    connect(m_wallet, &Wallet::multiBroadcast,      this, &MainWindow::onMultiBroadcast);
}

void MainWindow::menuToggleTabVisible(const QString &key){
    const auto toggleTab = m_tabShowHideMapper[key];

    QStringList enabledTabs = conf()->get(Config::enabledTabs).toStringList();
    bool show = enabledTabs.contains(key);
    show = !show;

    if (show) {
        enabledTabs.append(key);
    } else {
        enabledTabs.removeAll(key);
    }

    conf()->set(Config::enabledTabs, enabledTabs);
    //ui->tabWidget->setTabVisible(ui->tabWidget->indexOf(toggleTab->tab), show);
    toggleTab->menuAction->setText(toggleTab->name);
}

void MainWindow::menuClearHistoryClicked() {
    conf()->remove(Config::recentlyOpenedWallets);
    this->updateRecentlyOpenedMenu();
}

QString MainWindow::walletName() {
    return QFileInfo(m_wallet->cachePath()).fileName();
}

QString MainWindow::walletCachePath() {
    return m_wallet->cachePath();
}

QString MainWindow::walletKeysPath() {
    return m_wallet->keysPath();
}

void MainWindow::onWalletOpened() {
    qDebug() << Q_FUNC_INFO;
    m_splashDialog->hide();

    m_wallet->setRingDatabase(Utils::ringDatabasePath());

    m_wallet->updateBalance();
    if (m_wallet->isHwBacked()) {
        m_statusBtnHwDevice->show();
    }

    this->bringToFront();
    this->setEnabled(true);

    // receive page
    m_wallet->subaddress()->refresh();
    if (m_wallet->subaddress()->count() == 1) {
        for (int i = 0; i < 10; i++) {
            m_wallet->subaddress()->addRow("");
        }
    }

    // history page
    m_wallet->history()->refresh();

    // coins page
    m_wallet->coins()->refresh();
    m_coinsWidget->setModel(m_wallet->coinsModel(), m_wallet->coins());
    m_wallet->coinsModel()->setCurrentSubaddressAccount(m_wallet->currentSubaddressAccount());

    // Coin labeling uses set_tx_note, so we need to refresh history too
    connect(m_wallet->coins(), &Coins::descriptionChanged, [this] {
        m_wallet->history()->refresh();
    });
    // Vice versa
    connect(m_wallet->transactionHistoryModel(), &TransactionHistoryModel::transactionDescriptionChanged, [this] {
        m_wallet->coins()->refresh();
    });

    this->updatePasswordIcon();
    this->updateTitle();
    m_nodes->allowConnection();
    m_nodes->connectToNode();
    m_updateBytes.start(250);

    if (conf()->get(Config::writeRecentlyOpenedWallets).toBool()) {
        this->addToRecentlyOpened(m_wallet->cachePath());
    }
}

void MainWindow::updateBalance() {
    this->onBalanceUpdated(m_wallet->balance(), m_wallet->unlockedBalance());
}

void MainWindow::onBalanceUpdated(quint64 balance, quint64 spendable) {
    bool hide = conf()->get(Config::hideBalance).toBool();
    int displaySetting = conf()->get(Config::balanceDisplay).toInt();

    // amountPrecision defaults to 12, which is far more than the status bar
    // wants; a user who has asked for fewer still gets fewer.
    constexpr int kStatusDecimals = 5;
    const int decimals = qMin(conf()->get(Config::amountPrecision).toInt(), kStatusDecimals);

    // displayAmount truncates, so an amount below the rounding step comes back a
    // flat "0"; fall back to full precision rather than show nothing.
    auto format = [](quint64 atomic, int dp) {
        QString out = WalletManager::displayAmount(atomic, false, dp);
        if (atomic > 0 && out.toDouble() == 0.0) {
            out = WalletManager::displayAmount(atomic, false, 12);
        }
        return out;
    };

    QString amount;
    QString suffix;

    if (hide) {
        amount = "Hidden";
    }
    else if (displaySetting == Config::totalBalance) {
        amount = format(balance, decimals);
        suffix = "XMR";
    }
    else if (displaySetting == Config::spendable || displaySetting == Config::spendablePlusUnconfirmed) {
        amount = format(spendable, decimals);
        suffix = "XMR";

        if (displaySetting == Config::spendablePlusUnconfirmed && balance > spendable) {
            suffix += QString(" · +%1 pending").arg(format(balance - spendable, decimals));
        }
    }

    if (conf()->get(Config::balanceShowFiat).toBool() && !hide) {
        QString fiatCurrency = conf()->get(Config::preferredFiatCurrency).toString();
        double balanceFiatAmount = appData()->prices.convert("XMR", fiatCurrency, balance / constants::cdiv);

        // convert() returns 0 when it holds no rate, which is normal whenever
        // the feed is unreachable; "$0.00" would read as a real valuation.
        if (balanceFiatAmount > 0) {
            suffix += QString("%1%2")
                          .arg(suffix.isEmpty() ? "" : " · ",
                               Utils::amountToCurrencyString(balanceFiatAmount, fiatCurrency));
        }
    }

    m_statusLabelBalance->setToolTip("Click for a full balance breakdown");
    m_statusLabelBalance->setText(this->formatStatusBalance(amount, suffix));
}

QString MainWindow::formatStatusBalance(const QString &amount, const QString &suffix) {
    // A stylesheet cannot colour parts of one label, so the figure and its unit
    // are split inline here, which means resolving the theme here too.
    m_statusBalanceAmount = amount;
    m_statusBalanceSuffix = suffix;

    const bool light =
        conf()->get(Config::skin).toString() == constants::skinNativeWhite;

    // Same ink as #HomeWidget QLabel#viewHistory in each skin's stylesheet.
    const QString labelColour = light ? "#5A6472" : "#979696";
    // Each skin's primary text: the light skin's balance pill is #FFFFFF, so a
    // fixed white figure would be invisible on it.
    const QString amountColour = light ? "#16191D" : "#FFFFFF";
    const QString suffixColour = light ? "#6B7480" : "#8A93A3";

    QString text = QString("<span style='color:%1;'>Balance</span>&nbsp;&nbsp;"
                           "<span style='color:%2; font-weight:600;'>%3</span>")
                       .arg(labelColour, amountColour, amount.toHtmlEscaped());

    if (!suffix.isEmpty()) {
        text += QString("&nbsp;<span style='color:%1;'>%2</span>")
                    .arg(suffixColour, suffix.toHtmlEscaped());
    }

    return text;
}

void MainWindow::setStatusText(const QString &text, bool override, int timeout) {
    if (override) {
        m_statusOverrideActive = true;
        m_statusLabelStatus->setText(text);
        QTimer::singleShot(timeout, [this]{
            m_statusOverrideActive = false;
            this->setStatusText(m_statusText);
        });
        return;
    }

    m_statusText = text;

    if (!m_statusOverrideActive && !m_constructingTransaction) {
        m_statusLabelStatus->setText(text);
    }
}

void MainWindow::tryStoreWallet() {
    if (m_wallet->connectionStatus() == Wallet::ConnectionStatus::ConnectionStatus_Synchronizing) {
        Utils::showError(this, "Unable to save wallet", "Can't save wallet during synchronization", {"Wait until synchronization is finished and try again"}, "synchronization");
        return;
    }

    m_wallet->store();
}

void MainWindow::onWebsocketStatusChanged(bool enabled) {
    ui->actionShow_Home->setVisible(enabled);

    QStringList enabledTabs = conf()->get(Config::enabledTabs).toStringList();

    for (const auto &plugin : m_plugins) {
        if (plugin->hasParent()) {
            continue;
        }

        if (plugin->requiresWebsocket()) {
            // TODO: unload plugins
            //ui->tabWidget->setTabVisible(this->findTab(plugin->displayName()), enabled && enabledTabs.contains(plugin->displayName()));
        }
    }

    m_sendWidget->setWebsocketEnabled(enabled);
}

void MainWindow::onProxySettingsChangedConnect() {
    m_nodes->connectToNode();
    this->onProxySettingsChanged();
}

void MainWindow::onProxySettingsChanged() {
    int proxy = conf()->get(Config::proxy).toInt();

    if (proxy == Config::Proxy::Tor) {
        this->onTorConnectionStateChanged(torManager()->torConnected);
        m_statusBtnProxySettings->show();
        return;
    }

    if (proxy == Config::Proxy::i2p) {
        m_statusBtnProxySettings->setIcon(this->statusGlyph("i2p.png", true));
        m_statusBtnProxySettings->show();
        return;
    }

    m_statusBtnProxySettings->hide();
}

void MainWindow::onOfflineMode(bool offline) {
    m_wallet->setOffline(offline);

    if (m_wallet->viewOnly()) {
        return;
    }

    if (ui->stackedWidget->currentIndex() != Stack::LOCKED) {
        ui->stackedWidget->setCurrentIndex(offline ? Stack::OFFLINE: Stack::WALLET);
    }

    ui->actionPay_to_many->setVisible(!offline);
    ui->menuView->setDisabled(offline);

    m_statusLabelBalance->setVisible(!offline);
    m_statusBtnProxySettings->setVisible(!offline);
}

void MainWindow::onManualFeeSelectionEnabled(bool enabled) {
    m_sendWidget->setManualFeeSelectionEnabled(enabled);
}

void MainWindow::onSubtractFeeFromAmountEnabled(bool enabled) {
    m_sendWidget->setSubtractFeeFromAmountEnabled(enabled);
}

void MainWindow::onMultiBroadcast(const QMap<QString, QString> &txHexMap) {
    QMapIterator<QString, QString> i(txHexMap);
    while (i.hasNext()) {
        i.next();
        for (const auto& node: m_nodes->nodes()) {
            QString address = node.toURL();
            qDebug() << QString("Relaying %1 to: %2").arg(i.key(), address);
            m_rpc->setDaemonAddress(address);
            m_rpc->sendRawTransaction(i.value());
        }
    }
}

void MainWindow::onNavigationChanged(int index)
{
    QWidget *page = ui->navigationStack->widget(index);
    if (!page) {
        return;
    }

    if (page == m_sendWidget) {
        updateNavigationState(ui->sendWidget, ui->sendButton);
        return;
    }
    if (page == m_receiveWidget) {
        updateNavigationState(ui->receiveWidget, ui->receiveButton);
        return;
    }
    if (page == m_swapWidget) {
        updateNavigationState(ui->swapWidget, ui->swapButton);
        return;
    }
    if (page == m_historyWidget) {
        updateNavigationState(ui->historyWidget, ui->historyButton);
        return;
    }

    // Portfolio is a plugin, so it has no member to compare against.
    for (auto *plugin : m_plugins) {
        if (plugin->tab() == page && plugin->displayName() == "Portfolio") {
            updateNavigationState(ui->portfolioWidget, ui->portfolioButton);
            return;
        }
    }
}

void MainWindow::onSyncStatus(quint64 height, quint64 target, bool daemonSync) {
    if (height >= (target - 1)) {
        this->updateNetStats();
    }

    m_syncProgress.update(height, target, daemonSync);

    // Both ends of the job, the way a download states them: where it has got
    // to, where it is going, and how long that leaves. formatSyncStatus still
    // supplies the settled wording, including "Synchronized".
    QString status = Utils::formatSyncStatus(height, target, daemonSync);

    const QString progress = m_syncProgress.progressText();
    if (!progress.isEmpty()) {
        status = QString("%1 sync: %2").arg(daemonSync ? "Blockchain" : "Wallet", progress);
    }

    const QString estimate = m_syncProgress.estimateText();
    if (!estimate.isEmpty()) {
        status += QString(" · %1").arg(estimate);
    }

    m_statusSyncBar->setVisible(m_syncProgress.isSyncing());
    if (m_syncProgress.isSyncing()) {
        // Only on a change: syncStatus arrives once per scanned block, and the
        // percentage moves about a hundred times over an entire sync.
        const int percent = m_syncProgress.percent();
        if (percent != m_statusSyncBar->value()) {
            m_statusSyncBar->setValue(percent);
            m_statusSyncBar->setToolTip(QString("%1% · %2 blocks remaining")
                                            .arg(percent)
                                            .arg(m_syncProgress.remaining()));
        }
    }

    this->setStatusText(status);
    m_statusLabelStatus->setToolTip(QString("Wallet height: %1").arg(QString::number(height)));
}

void MainWindow::onConnectionStatusChanged(int status)
{
    // Note: Wallet does not emit this signal unless status is changed, so calling this function from MainWindow may
    // result in the wrong connection status being displayed.
    qDebug() << "Wallet connection status changed:" << Utils::QtEnumToString(static_cast<Wallet::ConnectionStatus>(status));

    // Update connection info in status bar
    QIcon icon;
    if (conf()->get(Config::offlineMode).toBool()) {
        icon = this->statusDot("red-circle.svg");
        this->setStatusText("Offline mode");
    } else {
        switch(status){
            case Wallet::ConnectionStatus_Disconnected:
                icon = this->statusDot("red-circle.svg");
                this->setStatusText("Disconnected");
                break;
            case Wallet::ConnectionStatus_Connecting:
                icon = this->statusDot("yellow-circle.svg");
                this->setStatusText("Connecting to node");
                break;
            case Wallet::ConnectionStatus_WrongVersion:
                icon = this->statusDot("red-circle.svg");
                this->setStatusText("Incompatible node");
                break;
            case Wallet::ConnectionStatus_Synchronizing:
                // Matches the portfolio pill's synchronizing indicator.
                icon = this->statusDot("blue-circle.svg");
                break;
            case Wallet::ConnectionStatus_Synchronized:
                icon = this->statusDot("green-circle.svg");
                break;
            default:
                icon = this->statusDot("red-circle.svg");
                break;
        }
    }

    m_statusBtnConnectionStatusIndicator->setIcon(icon);
}

void MainWindow::onTransactionCreated(PendingTransaction *tx, const QVector<QString> &address) {
    // Clean up some UI
    m_constructingTransaction = false;
    m_txTimer.stop();
    this->setStatusText(m_statusText);

    if (m_wallet->isHwBacked()) {
        m_splashDialog->hide();
    }

    if (tx->status() != PendingTransaction::Status_Ok) {
        if (m_showDeviceError) {
            // The hardware devices has disconnected during tx construction.
            // Due to a macOS-specific Qt bug, we have to prevent it from stacking two QMessageBoxes, otherwise
            // the UI becomes unresponsive. The reconnect dialog should take priority.
            m_wallet->disposeTransaction(tx);
            return;
        }

        QString errMsg = tx->errorString();

        Utils::Message message{this, Utils::ERROR, "Failed to construct transaction", errMsg};

        if (tx->getException()) {
            try
            {
                std::rethrow_exception(tx->getException());
            }
            catch (const tools::error::daemon_busy &e) {
                message.description = QString("Node was unable to respond. Failed request: %1").arg(QString::fromStdString(e.request()));
                message.helpItems = {"Try sending the transaction again.", "If this keeps happening, connect to a different node."};
            }
            catch (const tools::error::no_connection_to_daemon &e) {
                message.description = QString("Connection to node lost. Failed request: %1").arg(QString::fromStdString(e.request()));
                message.helpItems = {"Try sending the transaction again.", "If this keeps happening, connect to a different node."};
            }
            catch (const tools::error::wallet_rpc_error &e) {
                message.description = QString("RPC error: %1").arg(QString::fromStdString(e.to_string()));
                message.helpItems = {"Try sending the transaction again.", "If this keeps happening, connect to a different node."};
            }
            catch (const tools::error::get_outs_error &e) {
                message.description = "Failed to get enough decoy outputs from node";
                message.helpItems = {"Your transaction has too many inputs. Try sending a lower amount."};
            }
            catch (const tools::error::not_enough_unlocked_money &e) {
                QString error;
                if (e.fee() > e.available()) {
                    error = QString("Transaction fee exceeds spendable balance.\n\nSpendable balance: %1\nTransaction fee: %2").arg(WalletManager::displayAmount(e.available()), WalletManager::displayAmount(e.fee()));
                }
                else {
                    error = QString("Spendable balance insufficient to pay for transaction.\n\nSpendable balance: %1\nTransaction needs: %2").arg(WalletManager::displayAmount(e.available()), WalletManager::displayAmount(e.tx_amount() + e.fee()));
                }
                message.description = error;
                message.helpItems = {"Wait for more balance to unlock.", "Click 'Help' to learn more about how balance works."};
                message.doc = "balance";
            }
            catch (const tools::error::not_enough_money &e) {
                message.description = QString("Not enough money to transfer\n\nTotal balance: %1\nTransaction amount: %2").arg(WalletManager::displayAmount(e.available()), WalletManager::displayAmount(e.tx_amount()));
                message.helpItems = {"If you are trying to send your entire balance, click 'Max'."};
                message.doc = "balance";
            }
            catch (const tools::error::tx_not_possible &e) {
                message.description = QString("Not enough money to transfer. Transaction amount + fee exceeds available balance.\n\n"
                                              "Spendable balance: %1\n"
                                              "Transaction needs: %2").arg(WalletManager::displayAmount(e.available()), WalletManager::displayAmount(e.tx_amount() + e.fee()));
                message.helpItems = {"If you're trying to send your entire balance, click 'Max'."};
                message.doc = "balance";
            }
            catch (const tools::error::not_enough_outs_to_mix &e) {
                message.description = "Not enough outputs for specified ring size.";
            }
            catch (const tools::error::tx_not_constructed&) {
                message.description = "Transaction was not constructed";
                message.helpItems = {"You have found a bug. Please contact the developers."};
                message.doc = "report_an_issue";
            }
            catch (const tools::error::tx_rejected &e) {
                // TODO: provide helptext
                message.description = QString("Transaction was rejected by node. Reason: %1.").arg(QString::fromStdString(e.status()));
            }
            catch (const tools::error::tx_sum_overflow &e) {
                message.description = "Transaction tries to spend an unrealistic amount of XMR";
                message.helpItems = {"You have found a bug. Please contact the developers."};
                message.doc = "report_an_issue";
            }
            catch (const tools::error::zero_amount&) {
                message.description = "Destination amount is zero";
                message.helpItems = {"You have found a bug. Please contact the developers."};
                message.doc = "report_an_issue";
            }
            catch (const tools::error::zero_destination&) {
                message.description = "Transaction has no destination";
                message.helpItems = {"You have found a bug. Please contact the developers."};
                message.doc = "report_an_issue";
            }
            catch (const tools::error::tx_too_big &e) {
                message.description = "Transaction too big";
                message.helpItems = {"Try sending a smaller amount."};
            }
            catch (const tools::error::transfer_error &e) {
                message.description = QString("Unknown transfer error: %1").arg(QString::fromStdString(e.what()));
                message.helpItems = {"You have found a bug. Please contact the developers."};
                message.doc = "report_an_issue";
            }
            catch (const tools::error::wallet_internal_error &e) {
                bool bug = true;
                QString msg = e.what();
                message.description = QString("Internal error: %1").arg(QString::fromStdString(e.what()));

                if (msg.contains("Daemon response did not include the requested real output")) {
                    QString currentNode = m_nodes->connection().toAddress();
                    message.description += QString("\nYou are currently connected to: %1\n\n"
                                                   "This node may be acting maliciously. You are strongly recommended to disconnect from this node."
                                                   "Please report this incident to the developers.").arg(currentNode);
                    message.doc = "report_an_issue";
                }
                if (msg.startsWith("No unlocked balance")) {
                    // TODO: We're sending ALL, but fractional outputs got ignored
                    message.description = "Spendable balance insufficient to pay for transaction fee.";
                    bug = false;
                }
                if (msg.contains("Failed to get height") || msg.contains("Failed to get earliest fork height")) {
                    message.description = QString("RPC error: %1").arg(QString::fromStdString(e.to_string()));
                    message.helpItems = {"Try sending the transaction again.", "If this keeps happening, connect to a different node."};
                    bug = false;
                }

                if (bug) {
                    message.helpItems = {"You have found a bug. Please contact the developers."};
                    message.doc = "report_an_issue";
                }
            }
            catch (const std::exception &e) {
                message.description = QString::fromStdString(e.what());
            }
        }

        Utils::showMsg(message);
        m_wallet->disposeTransaction(tx);
        return;
    }
    if (tx->txCount() == 0) {
        Utils::showError(this, "Failed to construct transaction", "No transactions were constructed", {"You have found a bug. Please contact the developers."}, "report_an_issue");
        m_wallet->disposeTransaction(tx);
        return;
    }
    if (tx->txCount() > 1) {
        Utils::showError(this, "Failed to construct transaction", "Transaction tries to spend too many inputs", {"Send a smaller amount of XMR to yourself first."});
        m_wallet->disposeTransaction(tx);
        return;
    }

    // This is a weak check to see if we send to all specified destination addresses
    // This is here to catch rare memory corruption errors during transaction construction
    // TODO: also check that amounts match
    tx->refresh();
    QSet<QString> outputAddresses;
    for (const auto &output : tx->transaction(0).outputs) {
        outputAddresses.insert(WalletManager::baseAddressFromIntegratedAddress(output.address, constants::networkType));
    }
    QSet<QString> destAddresses;
    for (const auto &addr : address) {
        // TODO: Monero core bug, integrated address is not added to dests for transactions spending ALL
        destAddresses.insert(WalletManager::baseAddressFromIntegratedAddress(addr, constants::networkType));
    }
    if (!outputAddresses.contains(destAddresses)) {
        Utils::showError(this, "Transaction fails sanity check", "Constructed transaction doesn't appear to send to (all) specified destination address(es). Try creating the transaction again.");
        m_wallet->disposeTransaction(tx);
        return;
    }

    // Offline transaction signing
    if (m_wallet->viewOnly()) {
#ifdef WITH_SCANNER
        OfflineTxSigningWizard wizard(this, m_wallet, tx);
        wizard.exec();
        
        if (!wizard.readyToCommit()) {
            return;
        } else {
            tx = wizard.signedTx();
        }

        if (tx->txCount() == 0) {
            Utils::showError(this, "Failed to load transaction", "No transactions were found", {"You have found a bug. Please contact the developers."}, "report_an_issue");
            m_wallet->disposeTransaction(tx);
            return;
        }
#else
        Utils::showError(this, "Can't open offline transaction signing wizard", "BestWallet was built without webcam QR scanner support");
        return;
#endif
    }

    m_wallet->addCacheTransaction(tx->txid()[0], tx->signedTxToHex(0));

    // Show advanced dialog on multi-destination transactions
    if (address.size() > 1) {
        TxConfAdvDialog dialog_adv{m_wallet, m_wallet->tmpTxDescription, this};
        dialog_adv.setTransaction(tx, !m_wallet->viewOnly());
        dialog_adv.exec();
        return;
    }

    TxConfDialog dialog{m_wallet, tx, address[0], m_wallet->tmpTxDescription, this};
    switch (dialog.exec()) {
        case QDialog::Rejected:
        {
            if (!dialog.showAdvanced) {
                m_wallet->disposeTransaction(tx);
            }
            break;
        }
        case QDialog::Accepted:
            m_wallet->commitTransaction(tx, m_wallet->tmpTxDescription);
            break;
    }

    if (dialog.showAdvanced) {
        TxConfAdvDialog dialog_adv{m_wallet, m_wallet->tmpTxDescription, this};
        dialog_adv.setTransaction(tx);
        dialog_adv.exec();
    }
}

void MainWindow::onTransactionCommitted(bool success, PendingTransaction *tx, const QStringList& txid) {
    if (!success) {
        QString error = tx->errorString();
        if (m_wallet->viewOnly() && error.contains("double spend")) {
            m_wallet->setForceKeyImageSync(true);
        }
        if (error.contains("no connection to daemon")) {
            QMessageBox box(this);
            box.setWindowTitle("Question");
            box.setText("Unable to send transaction");
            box.setInformativeText("No connection to node. Retry sending transaction?");
            QPushButton *manual = box.addButton("Broadcast manually", QMessageBox::HelpRole);
            box.addButton(QMessageBox::No);
            box.addButton(QMessageBox::Yes);

            box.exec();

            if (box.clickedButton() == manual) {
                if (txid.empty()) {
                    Utils::showError(this, "Unable to open tx broadcaster", "Cached transaction not found");
                    return;
                }
                this->onResendTransaction(txid[0]);
            }
            else if (box.result() == QMessageBox::Yes) {
                m_wallet->commitTransaction(tx, m_wallet->tmpTxDescription);
            }
            return;
        }
        Utils::showError(this, "Failed to send transaction", error);
        return;
    }

    QMessageBox msgBox{this};
    QPushButton *showDetailsButton = msgBox.addButton("Show details", QMessageBox::ActionRole);
    msgBox.addButton(QMessageBox::Ok);
    QString body = QString("Successfully sent %1 transaction(s).").arg(txid.count());
    msgBox.setText(body);
    msgBox.setWindowTitle("Transaction sent");
    msgBox.setIcon(QMessageBox::Icon::Information);
    msgBox.exec();
    if (msgBox.clickedButton() == showDetailsButton) {
        this->showHistoryTab();

        const auto& rows = m_wallet->history()->getRows();
        auto itr = std::find_if(rows.begin(), rows.end(),
                [&](const TransactionRow& ti) {
            return ti.hash == txid.first();
        });
        if (itr == rows.end()) {
            return;
        }

        auto *dialog = new TxInfoDialog(m_wallet, *itr, this);
        connect(dialog, &TxInfoDialog::resendTranscation, this, &MainWindow::onResendTransaction);
        dialog->show();
        dialog->setAttribute(Qt::WA_DeleteOnClose);
    }

    m_sendWidget->clearFields();
}

void MainWindow::showWalletInfoDialog() {
    WalletInfoDialog dialog{m_wallet, this};
    dialog.exec();
}

void MainWindow::showSeedDialog() {
    if (m_wallet->isHwBacked()) {
        Utils::showInfo(this, "Seed unavailable", "Wallet keys are stored on a hardware device", {}, "show_wallet_seed");
        return;
    }

    if (m_wallet->viewOnly()) {
        Utils::showInfo(this, "Seed unavailable", "Wallet is view-only", {"To obtain your private spendkey go to Wallet -> Keys"}, "show_wallet_seed");
        return;
    }

    if (!m_wallet->isDeterministic()) {
        Utils::showInfo(this, "Seed unavailable", "Wallet is non-deterministic and has no seed",
                        {"To obtain wallet keys go to Wallet -> Keys"}, "show_wallet_seed");
        return;
    }

    if (!this->verifyPassword()) {
        return;
    }

    SeedDialog dialog{m_wallet, this};
    dialog.exec();
}

void MainWindow::showPasswordDialog() {
    PasswordChangeDialog dialog{this, m_wallet};
    dialog.exec();
    this->updatePasswordIcon();
}

void MainWindow::updatePasswordIcon() {
    bool emptyPassword = m_wallet->verifyPassword("");
    QIcon icon = this->statusGlyph(emptyPassword ? "status_unlock.svg" : "status_lock.svg", true);
    m_statusBtnPassword->setIcon(icon);
}

void MainWindow::showKeysDialog() {
    if (!this->verifyPassword()) {
        return;
    }

    KeysDialog dialog{m_wallet, this};
    dialog.exec();
}

void MainWindow::showViewOnlyDialog() {
    if (!this->verifyPassword()) {
        return;
    }

    ViewOnlyDialog dialog{m_wallet, this};
    dialog.exec();
}

void MainWindow::showKeyImageSyncWizard() {
#ifdef WITH_SCANNER
    OfflineTxSigningWizard wizard{this, m_wallet};
    wizard.exec();

    if (wizard.readyToSign()) {
        if (wizard.unsignedTransaction()->txCount() == 0) {
            Utils::showError(this, "Unable to load unsigned transaction", "Unsigned transaction set contains no transactions");
            return;
        }

        if (wizard.unsignedTransaction()->txCount() > 1) {
            Utils::showError(this, "Unable to load unsigned transaction", "Unsigned transaction set contains more than one transaction");
            return;
        }

        TxConfAdvDialog dialog{m_wallet, "", this, true};
        dialog.setUnsignedTransaction(wizard.unsignedTransaction());
        auto r = dialog.exec();

        if (r != QDialog::Accepted) {
            return;
        }

        wizard.setStartId(OfflineTxSigningWizard::Page_ExportSignedTx);
        wizard.restart();
        wizard.exec();
    }
#else
    Utils::showError(this, "Can't open offline transaction signing wizard", "Best Wallet was built without webcam QR scanner support");
#endif
}

void MainWindow::menuHwDeviceClicked() {
    Utils::showInfo(this, "Hardware device", QString("This wallet is backed by a %1 hardware device.").arg(this->getHardwareDevice()));
}

void MainWindow::menuOpenClicked() {
    m_windowManager->wizardOpenWallet();
}

void MainWindow::menuNewRestoreClicked() {
    m_windowManager->showWizard(WalletWizard::Page_Menu);
}

void MainWindow::menuQuitClicked() {
    this->close();
}

void MainWindow::menuWalletCloseClicked() {
    m_windowManager->showWizard(WalletWizard::Page_Menu);
    this->close();
}

void MainWindow::menuProxySettingsClicked() {
    this->menuSettingsClicked(true);
}

void MainWindow::menuAboutClicked() {
    AboutDialog dialog{this};
    dialog.exec();
}

void MainWindow::menuSettingsClicked(bool showProxyTab) {
    m_windowManager->showSettings(m_nodes, this, showProxyTab);
}

void MainWindow::menuSignVerifyClicked() {
    SignVerifyDialog dialog{m_wallet, this};
    dialog.exec();
}

void MainWindow::menuVerifyTxProof() {
    VerifyProofDialog dialog{m_wallet, this};
    dialog.exec();
}

void MainWindow::onShowSettingsPage(int page) {
    conf()->set(Config::lastSettingsPage, page);
    this->menuSettingsClicked();
}

void MainWindow::skinChanged(const QString &skinName) {
    ColorScheme::updateFromWidget(this);
    this->updateWidgetIcons();
    this->updateLeftbarLogo();

    // The balance label's colours are inline HTML, so a stylesheet swap alone
    // leaves it painted for the previous theme.
    m_statusLabelBalance->setText(
        this->formatStatusBalance(m_statusBalanceAmount, m_statusBalanceSuffix));
}

void MainWindow::updateWidgetIcons() {
    this->applyThemedIcons();
    m_sendWidget->skinChanged();
    m_swapWidget->skinChanged();
    emit updateIcons();

    m_statusBtnHwDevice->setIcon(this->hardwareDevicePairedIcon());

    // applyThemedIcons has just repainted every nav glyph in the theme's ink,
    // dropping the accent from the selected row.
    this->onNavigationChanged(ui->navigationStack->currentIndex());
}

QIcon MainWindow::hardwareDevicePairedIcon() {
    QString filename;
    if (m_wallet->isLedger())
        filename = "ledger.png";
    else if (m_wallet->isTrezor())
        filename = ColorScheme::darkScheme ? "trezor_white.png" : "trezor.png";
    return this->statusGlyph(filename, false);
}

QIcon MainWindow::hardwareDeviceUnpairedIcon() {
    QString filename;
    if (m_wallet->isLedger())
        filename = "ledger_unpaired.png";
    else if (m_wallet->isTrezor())
        filename = ColorScheme::darkScheme ? "trezor_unpaired_white.png" : "trezor_unpaired.png";
    return this->statusGlyph(filename, false);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    bool isMousePress = (event->type() == QEvent::MouseButtonPress);

    // Navigate to portfolio
    for (auto* plugin : m_plugins) {
        if (plugin->displayName() == "Portfolio") {
            if (obj == ui->portfolioWidget && isMousePress
                || obj == ui->portfolioButton && isMousePress) {
                ui->navigationStack->setCurrentWidget(plugin->tab());
                return true;
            }
        }
    }

    if (obj == ui->sendWidget && isMousePress
        || obj == ui->sendButton && isMousePress) {
        ui->navigationStack->setCurrentWidget(m_sendWidget);
        return true;
    } else if (obj == ui->receiveWidget && isMousePress
               || obj == ui->receiveButton && isMousePress) {
        ui->navigationStack->setCurrentWidget(m_receiveWidget);
        return true;
    } else if (obj == ui->swapWidget && isMousePress
               || obj == ui->swapButton && isMousePress) {
        ui->navigationStack->setCurrentWidget(m_swapWidget);
        return true;
    } else if (obj == ui->historyWidget && isMousePress
               || obj == ui->historyButton && isMousePress) {
        ui->navigationStack->setCurrentWidget(m_historyWidget);
        return true;
    }

    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    qDebug() << Q_FUNC_INFO;

    if (!this->cleanedUp) {
        qDebug() << "MainWindow: cleaning up";
        this->cleanedUp = true;

        emit aboutToQuit();

        m_historyWidget->resetModel();

        m_updateBytes.stop();
        m_txTimer.stop();

        // Wallet signal may fire after AppContext is gone, causing segv
        m_wallet->disconnect();
        this->disconnect();

        this->saveGeo();
        m_windowManager->closeWindow(this);
    }

    event->accept();
}

void MainWindow::changeEvent(QEvent* event)
{
    if ((event->type() == QEvent::WindowStateChange) && this->isMinimized()) {
        if (conf()->get(Config::lockOnMinimize).toBool()) {
            this->lockWallet();
        }
        if (conf()->get(Config::showTrayIcon).toBool() && conf()->get(Config::minimizeToTray).toBool()) {
            this->hide();
        }
    } else {
        QMainWindow::changeEvent(event);
    }
}

void MainWindow::showHistoryTab() {
    this->raise();
    ui->navigationStack->setCurrentWidget(m_historyWidget);
}

void MainWindow::fillSendTab(const QString &address, const QString &description) {
    m_sendWidget->fill(address, description);
    ui->navigationStack->setCurrentWidget(m_sendWidget);
}

void MainWindow::payToMany() {
    ui->navigationStack->setCurrentWidget(m_sendWidget);
    m_sendWidget->payToMany();
    Utils::showInfo(this, "Pay to many", "Enter a list of outputs in the 'Pay to' field.\n"
                                         "One output per line.\n"
                                         "Format: address, amount\n"
                                         "A maximum of 16 addresses may be specified.");
}

void MainWindow::onViewOnBlockExplorer(const QString &txid) {
    QString blockExplorerLink = Utils::blockExplorerLink(txid);
    if (blockExplorerLink.isEmpty()) {
        Utils::showError(this, "Unable to open block explorer", "No block explorer configured", {"Go to Settings -> Misc -> Block explorer"});
        return;
    }
    Utils::externalLinkWarning(this, blockExplorerLink);
}

void MainWindow::onResendTransaction(const QString &txid) {
    QString txHex = m_wallet->getCacheTransaction(txid);
    if (txHex.isEmpty()) {
        Utils::showError(this, "Unable to resend transaction", "Transaction was not found in the transaction cache.");
        return;
    }

    // Connect to a different node so chances of successful relay are higher
    m_nodes->autoConnect(true);

    TxBroadcastDialog dialog{this, m_nodes, txHex};
    dialog.exec();
}

void MainWindow::saveGeo() {
    conf()->set(Config::geometry, QString(saveGeometry().toBase64()));
    conf()->set(Config::windowState, QString(saveState().toBase64()));
}

void MainWindow::restoreGeo() {
    bool geo = this->restoreGeometry(QByteArray::fromBase64(conf()->get(Config::geometry).toByteArray()));
    bool windowState = this->restoreState(QByteArray::fromBase64(conf()->get(Config::windowState).toByteArray()));
    qDebug() << "Restored window state: " << geo << " " << windowState;
}

void MainWindow::showDebugInfo() {
    DebugInfoDialog dialog{m_wallet, m_nodes, this};
    dialog.exec();
}

void MainWindow::showWalletCacheDebugDialog() {
    if (!this->verifyPassword()) {
        return;
    }

    WalletCacheDebugDialog dialog{m_wallet, this};
    dialog.exec();
}

void MainWindow::showTxPoolViewerDialog() {
    if (!m_txPoolViewerDialog) {
        m_txPoolViewerDialog = new TxPoolViewerDialog{this, m_wallet};
    }

    m_txPoolViewerDialog->show();
}

void MainWindow::showAccountSwitcherDialog() {
    m_accountSwitcherDialog->show();
    m_accountSwitcherDialog->update();
}

void MainWindow::showAddressChecker() {
    QString address = QInputDialog::getText(this, "Address Checker", "Address:                                      ");
    if (address.isEmpty()) {
        return;
    }

    if (!WalletManager::addressValid(address, constants::networkType)) {
        Utils::showInfo(this, "Invalid address", "The address you entered is not a valid XMR address for the current network type.");
        return;
    }

    if (!m_wallet->isAddressTorsionFree(address)) {
        Utils::showWarning(this, "Address is not torsion-free", "This address is not compatible with reference wallets.\n\nSending to this address MAY RESULT IN A LOSS OF FUNDS.");
        return;
    }

    SubaddressIndex index = m_wallet->subaddressIndex(address);
    if (!index.isValid()) {
        // TODO: probably mention lookahead here
        Utils::showInfo(this, "This address does not belong to this wallet", "");
        return;
    } else {
        Utils::showInfo(this, QString("This address belongs to Account #%1").arg(index.major));
    }
}

void MainWindow::showURDialog() {
#ifdef WITH_SCANNER
    URDialog dialog{this};
    dialog.exec();
#else
    Utils::showError(this, "Unable to open UR dialog", "Best Wallet was built without webcam scanner support");
#endif
}

void MainWindow::loadSignedTx() {
    QString fn = QFileDialog::getOpenFileName(this, "Select transaction to load", QDir::homePath(), "Transaction (*signed_monero_tx);;All Files (*)");
    if (fn.isEmpty()) return;
    PendingTransaction *tx = m_wallet->loadSignedTxFile(fn);
    auto err = m_wallet->errorString();
    if (!err.isEmpty()) {
        Utils::showError(this, "Unable to load signed transaction", err);
        return;
    }

    if (tx->txCount() == 0) {
        Utils::showError(this, "Unable to load signed transaction", "Signed transaction set contains no transactions");
        m_wallet->disposeTransaction(tx);
        return;
    }

    if (tx->txCount() > 1) {
        Utils::showError(this, "Unable to load signed transaction", "Signed transaction set contains more than one transaction");
        m_wallet->disposeTransaction(tx);
        return;
    }

    TxConfAdvDialog dialog{m_wallet, "", this, true};
    dialog.setTransaction(tx);
    dialog.exec();
}

void MainWindow::loadSignedTxFromText() {
    TxBroadcastDialog dialog{this, m_nodes};
    dialog.exec();
}

void MainWindow::importTransaction() {
    if (conf()->get(Config::torPrivacyLevel).toInt() == Config::allTorExceptNode) {
        // TODO: don't show if connected to local node

        auto result = QMessageBox::warning(this, "Warning", "Using this feature may allow a remote node to associate the transaction with your IP address.\n"
                                                            "\n"
                                                            "Connect to a trusted node or run Best Wallet over Tor if network level metadata leakage is included in your threat model.",
                                           QMessageBox::Ok | QMessageBox::Cancel);
        if (result != QMessageBox::Ok) {
            return;
        }
    }

    TxImportDialog dialog(this, m_wallet);
    dialog.exec();
}

void MainWindow::onDeviceError(const QString &error, quint64 errorCode) {
    qCritical() << "Device error: " << error;

    if (m_showDeviceError) {
        return;
    }

    m_statusBtnHwDevice->setIcon(this->hardwareDeviceUnpairedIcon());
    while (true) {
        m_showDeviceError = true;

        QString prompt = "Lost connection to hardware device. Attempt to reconnect?";
        if (errorCode == 0x5515) {
            prompt = QString("Device must be unlocked to continue scanning. Attempt to continue?");
        }

        auto result = QMessageBox::question(this, "Hardware device", prompt);
        if (result == QMessageBox::Yes) {
            bool r = m_wallet->reconnectDevice();
            if (r) {
                break;
            }
        }
        if (result == QMessageBox::No) {
            this->menuWalletCloseClicked();
            return;
        }
    }
    m_statusBtnHwDevice->setIcon(this->hardwareDevicePairedIcon());
    m_wallet->startRefresh();
    m_showDeviceError = false;
}

void MainWindow::onDeviceButtonRequest(quint64 code) {
    qDebug() << "DeviceButtonRequest, code: " << code;

    if (m_wallet->isTrezor()) {
        switch (code) {
            case 1:
            {
                m_splashDialog->setMessage("Action required on device: Enter your PIN to continue");
                m_splashDialog->setIcon(QPixmap(":/assets/images/key.png"));
                m_splashDialog->show();
                m_splashDialog->setEnabled(true);
                break;
            }
            case 8:
            default:
            {
                // Annoyingly, this code is used for a variety of actions, including:
                // Confirm refresh: Do you really want to start refresh?
                // Confirm export: Do you really want to export tx_key?

                if (m_constructingTransaction) { // This code is also used when signing a tx, we handle this elsewhere
                    break;
                }

                m_splashDialog->setMessage("Confirm action on device to proceed");
                m_splashDialog->setIcon(QPixmap(":/assets/images/confirmed.svg"));
                m_splashDialog->show();
                m_splashDialog->setEnabled(true);
                break;
            }
        }
    }
}

void MainWindow::onDeviceButtonPressed() {
    if (m_constructingTransaction) {
        return;
    }

    m_splashDialog->hide();
}

void MainWindow::onWalletPassphraseNeeded(bool on_device) {
    auto button = QMessageBox::question(nullptr, "Wallet Passphrase Needed", "Enter passphrase on hardware wallet?\n\n"
                                                                             "It is recommended to enter passphrase on "
                                                                             "the hardware wallet for better security.",
                                        QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
    if (button == QMessageBox::Yes) {
        m_wallet->onPassphraseEntered("", true, false);
        return;
    }

    bool ok;
    QString passphrase = QInputDialog::getText(nullptr, "Wallet Passphrase Needed", "Enter passphrase:", QLineEdit::EchoMode::Password, "", &ok);
    if (ok) {
        m_wallet->onPassphraseEntered(passphrase, false, false);
    } else {
        m_wallet->onPassphraseEntered(passphrase, false, true);
    }
}

void MainWindow::updateNetStats() {
    if (!m_wallet || m_wallet->connectionStatus() == Wallet::ConnectionStatus_Disconnected
                       || m_wallet->connectionStatus() == Wallet::ConnectionStatus_Synchronized)
    {
        m_statusLabelNetStats->hide();
        return;
    }

    m_statusLabelNetStats->show();
    m_statusLabelNetStats->setText(QString("· %1 downloaded").arg(Utils::formatBytes(m_wallet->getBytesReceived())));
}

void MainWindow::rescanSpent() {
    QMessageBox warning{this};
    warning.setWindowTitle("Warning");
    warning.setText("Rescanning spent outputs reveals which outputs you own to the node. "
                    "Make sure you are connected to a trusted node.\n\n"
                    "Do you want to proceed?");
    warning.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    
    auto r = warning.exec();
    if (r == QMessageBox::No) {
        return;
    }
    
    if (!m_wallet->rescanSpent()) {
        Utils::showError(this, "Failed to rescan spent outputs", m_wallet->errorString());
    } else {
        Utils::showInfo(this, "Successfully rescanned spent outputs");
    }
}

void MainWindow::showBalanceDialog() {
    BalanceDialog dialog{this, m_wallet};
    dialog.exec();
}

QString MainWindow::statusDots() {
    m_statusDots++;
    m_statusDots = m_statusDots % 4;
    return QString(".").repeated(m_statusDots);
}

void MainWindow::showOrHide() {
    if (this->isHidden())
        this->bringToFront();
    else
        this->hide();
}

void MainWindow::bringToFront() {
    ensurePolished();
    setWindowState((windowState() & ~Qt::WindowMinimized) | Qt::WindowActive);
    show();
    raise();
    activateWindow();
}

void MainWindow::onPreferredFiatCurrencyChanged() {
    m_sendWidget->onPreferredFiatCurrencyChanged();
}

void MainWindow::onHideUpdateNotifications(bool hidden) {
    if (hidden) {
        m_statusUpdateAvailable->hide();
    }
#ifdef CHECK_UPDATES
    else if (m_updater->state == Updater::State::UPDATE_AVAILABLE) {
        m_statusUpdateAvailable->show();
    }
#endif
}

void MainWindow::onTorConnectionStateChanged(bool connected) {
    if (conf()->get(Config::proxy).toInt() != Config::Proxy::Tor) {
        return;
    }

    m_statusBtnProxySettings->setIcon(this->statusGlyph("status_tor.svg", true));

    // if (connected)
    //     m_statusBtnProxySettings->setIcon(icons()->icon("tor_logo.png"));
    // else
    //     m_statusBtnProxySettings->setIcon(icons()->icon("tor_logo_disabled.png"));
}

void MainWindow::showUpdateNotification() {
#ifdef CHECK_UPDATES
    if (conf()->get(Config::hideUpdateNotifications).toBool()) {
        return;
    }

    QString versionDisplay{m_updater->version};
    versionDisplay.replace("beta", "Beta");
    QString updateText = QString("Update to Best Wallet %1 is available").arg(versionDisplay);
    m_statusUpdateAvailable->setText(updateText);
    m_statusUpdateAvailable->setToolTip("Click to Download update.");
    m_statusUpdateAvailable->show();

    m_statusUpdateAvailable->disconnect();
    connect(m_statusUpdateAvailable, &StatusBarButton::clicked, this, &MainWindow::showUpdateDialog);
#endif
}

void MainWindow::showUpdateDialog() {
#ifdef CHECK_UPDATES
    UpdateDialog updateDialog{this, m_updater};
    connect(&updateDialog, &UpdateDialog::restartWallet, m_windowManager, &WindowManager::restartApplication);
    updateDialog.exec();
#endif
}

void MainWindow::onInitiateTransaction() {
    m_statusDots = 0;
    m_constructingTransaction = true;
    m_txTimer.start(1000);

    if (m_wallet->isHwBacked()) {
        QString message = "Constructing transaction: action may be required on device.";
        m_splashDialog->setMessage(message);
        m_splashDialog->setIcon(QPixmap(":/assets/images/unconfirmed.png"));
        m_splashDialog->show();
        m_splashDialog->setEnabled(true);
    }
}

void MainWindow::onKeysCorrupted() {
    if (!m_criticalWarningShown) {
        m_criticalWarningShown = true;
        Utils::showError(this, "Potential wallet file corruption detected",
                         "WARNING!\n\n"
                         "To prevent LOSS OF FUNDS do NOT continue to use this wallet file.\n\n"
                         "Restore your wallet from seed, keys, or device.\n\n"
                         "Please report this incident to the Best Wallet developers.\n\n"
                         "WARNING!", {}, "report_an_issue");
        m_sendWidget->disallowSending();
    }
}

void MainWindow::onSelectedInputsChanged(const QStringList &selectedInputs) {
    int numInputs = selectedInputs.size();

    // ui->frame_coinControl->setStyleSheet(ColorScheme::GREEN.asStylesheet(true));
    // ui->frame_coinControl->setVisible(numInputs > 0);

    // if (numInputs > 0) {
    //     quint64 totalAmount = m_wallet->coins()->sumAmounts(selectedInputs);

    //     QString text = QString("Coin control active: %1 selected outputs, %2 XMR").arg(QString::number(numInputs), WalletManager::displayAmount(totalAmount));
    //     ui->label_coinControl->setText(text);
    // }
}

void MainWindow::onTxPoolBacklog(const QVector<quint64> &backlog, quint64 originalFeeLevel, quint64 automaticFeeLevel) {
    bool automatic = (originalFeeLevel == 0);

    if (automaticFeeLevel == 0) {
        qWarning() << "Automatic fee level wasn't adjusted";
        automaticFeeLevel = 2;
    }

    quint64 feeLevel = automatic ? automaticFeeLevel : originalFeeLevel;

    for (int i = 0; i < backlog.size(); i++) {
        qDebug() << QString("Fee level: %1, backlog: %2").arg(QString::number(i), QString::number(backlog[i]));
    }

    if (automatic) {
        if (backlog.size() > 1 && backlog[1] >= 2) {
            auto button = QMessageBox::question(this, "Transaction Pool Backlog",
                                                QString("There is a backlog of %1 blocks (≈ %2 minutes) in the transaction pool "
                                                        "at the maximum automatic fee level.\n\n"
                                                        "Do you want to increase the fee for this transaction?")
                                                        .arg(QString::number(backlog[1]), QString::number(backlog[1] * 2)));
            if (button == QMessageBox::Yes) {
                feeLevel = 3;
            }
        }
    }

    m_wallet->confirmPreTransactionChecks(feeLevel);
}

void MainWindow::onExportHistoryCSV() {
    HistoryExportDialog dialog{m_wallet, this};
    dialog.exec();
}

void MainWindow::onImportHistoryDescriptionsCSV() {
    const QString fileName = QFileDialog::getOpenFileName(this, "Import CSV file", QDir::homePath(), "CSV Files (*.csv)");
    if (fileName.isEmpty()) {
        return;
    }

    QString error = m_wallet->history()->importLabelsFromCSV(fileName);
    if (!error.isEmpty()) {
        Utils::showError(this, "Unable to import transaction descriptions from CSV", error);
    }
    else {
        Utils::showInfo(this, "Successfully imported transaction descriptions from CSV");
    }
}

void MainWindow::onCreateDesktopEntry() {
    auto msg = Utils::xdgDesktopEntryRegister() ? "Desktop entry created" : "Desktop entry not created due to an error.";
    QMessageBox::information(this, "Desktop entry", msg);
}

void MainWindow::onShowDocumentation() {
    // TODO: welcome page
    m_windowManager->showDocs(this);
}

void MainWindow::onReportBug() {
    m_windowManager->showDocs(this, "report_an_issue");
}

QString MainWindow::getHardwareDevice() {
    if (!m_wallet->isHwBacked())
        return "";
    if (m_wallet->isTrezor())
        return "Trezor";
    if (m_wallet->isLedger())
        return "Ledger";
    return "Unknown";
}

void MainWindow::updateTitle() {
    QString title = QString("%1 (#%2)").arg(this->walletName(), QString::number(m_wallet->currentSubaddressAccount()));

    if (m_wallet->viewOnly()) {
        title += " [view-only]";
    }

    title += " - Best Wallet";

    this->setWindowTitle(title);
}

void MainWindow::addToRecentlyOpened(QString keysFile) {
    auto recent = conf()->get(Config::recentlyOpenedWallets).toList();

    if (Utils::isPortableMode()) {
        QDir appPath{Utils::applicationPath()};
        keysFile = appPath.relativeFilePath(keysFile);
    }

    if (recent.contains(keysFile)) {
        recent.removeOne(keysFile);
    }
    recent.insert(0, keysFile);

    QList<QVariant> recent_;
    int count = 0;
    for (const auto &file : recent) {
        if (Utils::fileExists(file.toString())) {
            recent_.append(file);
            count++;
        }
        if (count >= 5) {
            break;
        }
    }

    conf()->set(Config::recentlyOpenedWallets, recent_);

    this->updateRecentlyOpenedMenu();
}

void MainWindow::updateRecentlyOpenedMenu() {
    ui->menuRecently_open->clear();
    const QStringList recentWallets = conf()->get(Config::recentlyOpenedWallets).toStringList();
    for (const auto &walletPath : recentWallets) {
        QFileInfo fileInfo{walletPath};
        ui->menuRecently_open->addAction(fileInfo.fileName(), m_windowManager, std::bind(&WindowManager::tryOpenWallet, m_windowManager, fileInfo.absoluteFilePath(), ""));
    }
    ui->menuRecently_open->addSeparator();
    ui->menuRecently_open->addAction(m_clearRecentlyOpenAction);
}

bool MainWindow::verifyPassword(bool sensitive) {
    bool incorrectPassword = false;
    while (true) {
        PasswordDialog passwordDialog{this->walletName(), incorrectPassword, sensitive, this};
        int ret = passwordDialog.exec();
        if (ret == QDialog::Rejected) {
            return false;
        }

        if (!m_wallet->verifyPassword(passwordDialog.password)) {
            incorrectPassword = true;
            continue;
        }
        break;
    }
    return true;
}

void MainWindow::userActivity() {
    m_userLastActive = QDateTime::currentSecsSinceEpoch();
}

void MainWindow::closeQDialogChildren(QObject *object) {
    for (QObject *child : object->children()) {
        if (auto *childDlg = dynamic_cast<QDialog*>(child)) {
            qDebug() << "Closing dialog: " << childDlg->objectName();
            childDlg->close();
        }
        this->closeQDialogChildren(child);
    }
}

void MainWindow::checkUserActivity() {
    if (!conf()->get(Config::inactivityLockEnabled).toBool()) {
        return;
    }

    if (m_constructingTransaction) {
        return;
    }

    if ((m_userLastActive + (conf()->get(Config::inactivityLockTimeout).toInt()*60)) < QDateTime::currentSecsSinceEpoch()) {
        qInfo() << "Locking wallet for inactivity";
        this->lockWallet();
    }
}

void MainWindow::lockWallet() {
    if (m_locked) {
        return;
    }

    if (m_constructingTransaction) {
        Utils::showError(this, "Unable to lock wallet", "Can't lock wallet during transaction construction");
        return;
    }
    m_walletUnlockWidget->reset();

    // Close all open QDialogs
    this->closeQDialogChildren(this);

    ui->navigationStack->hide();
    this->statusBar()->hide();
    this->menuBar()->hide();
    ui->stackedWidget->setCurrentIndex(1);

    m_checkUserActivity.stop();

    m_locked = true;
}

void MainWindow::unlockWallet(const QString &password) {
    if (!m_locked) {
        return;
    }

    if (!m_wallet->verifyPassword(password)) {
        m_walletUnlockWidget->incorrectPassword();
        return;
    }
    m_walletUnlockWidget->reset();

    ui->navigationStack->show();
    this->statusBar()->show();
    this->menuBar()->show();
    ui->stackedWidget->setCurrentIndex(0);
    this->onOfflineMode(conf()->get(Config::offlineMode).toBool());

    m_checkUserActivity.start();

    m_locked = false;
}

void MainWindow::toggleSearchbar(bool visible) {
    conf()->set(Config::showSearchbar, visible);

    m_historyWidget->setSearchbarVisible(visible);
    m_receiveWidget->setSearchbarVisible(visible);
    m_contactsWidget->setSearchbarVisible(visible);
    m_coinsWidget->setSearchbarVisible(visible);

    int currentTab = ui->navigationStack->currentIndex();
    if (currentTab == this->findTab("History"))
        m_historyWidget->focusSearchbar();
    else if (currentTab == this->findTab("Contacts"))
        m_contactsWidget->focusSearchbar();
    else if (currentTab == this->findTab("Receive"))
        m_receiveWidget->focusSearchbar();
    else if (currentTab == this->findTab("Coins"))
        m_coinsWidget->focusSearchbar();
}

int MainWindow::findTab(const QString &title) {
    // for (int i = 0; i < ui->navigationStack->count(); i++) {
    //     if (ui->tabWidget->tabText(i) == title) {
    //         return i;
    //     }
    // }
    return -1;
}

namespace {
    // Share of the label the largest side of a glyph should occupy.
    constexpr qreal kNavGlyphFill = 0.85;

    constexpr qreal kStatusGlyphFill = 0.82;

    // A solid disc reads heavier than the line art beside it at the same
    // extent, so it is drawn smaller to sit at the same optical weight.
    constexpr qreal kStatusDotFill = 0.52;

    // Recolours a single-colour glyph for the theme and the selected state, and
    // fits its drawn extent rather than its canvas: the glyphs pad themselves
    // differently, so scaling the canvas leaves them looking mismatched.
    QPixmap navGlyph(const QString &name, const QColor &colour, int size, qreal dpr)
    {
        const QPixmap glyph{QString(":/qss_icons/svg/%1.svg").arg(name)};
        if (glyph.isNull()) {
            return glyph;
        }

        const QImage img = glyph.toImage().convertToFormat(QImage::Format_ARGB32);
        const QRect ink = Icons::inkBounds(img);
        if (ink.isEmpty()) {
            return glyph;
        }

        QPixmap out{QSize(size, size) * dpr};
        out.setDevicePixelRatio(dpr);
        out.fill(Qt::transparent);

        const qreal scale = size * kNavGlyphFill / qMax(ink.width(), ink.height());
        const QSizeF target = QSizeF(ink.size()) * scale;
        const QRectF dest{(size - target.width()) / 2.0,
                          (size - target.height()) / 2.0,
                          target.width(), target.height()};

        QPainter painter{&out};
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        painter.drawPixmap(dest, glyph, QRectF(ink));
        painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
        painter.fillRect(QRectF(0, 0, size, size), colour);
        painter.end();

        return out;
    }
}

void MainWindow::updateLeftbarLogo() {
    const bool light =
        conf()->get(Config::skin).toString() == constants::skinNativeWhite;

    // One lockup per skin, each drawn for its own background.
    const QPixmap source{light ? ":/assets/images/appicons/light_mode_logo.png"
                               : ":/assets/images/appicons/dark_mode_logo.png"};

    // The lockup spells the name itself, so the title label would repeat it.
    ui->title->hide();

    if (source.isNull()) {
        return;
    }

    // The artwork carries a wide transparent margin, so trim it first or the
    // lockup is sized by its padding.
    const QRect ink =
        Icons::inkBounds(source.toImage().convertToFormat(QImage::Format_ARGB32));
    const QRect region = ink.isEmpty() ? source.rect() : ink;

    // Fits the rail: 250px wide, less the 25px lead-in and the 17px right margin.
    constexpr int kLogoMaxWidth = 176;
    constexpr int kLogoMaxHeight = 44;

    const qreal dpr = this->devicePixelRatioF();
    const QSizeF fitted =
        QSizeF(region.size()).scaled(kLogoMaxWidth, kLogoMaxHeight, Qt::KeepAspectRatio);

    QPixmap out{(fitted * dpr).toSize()};
    out.setDevicePixelRatio(dpr);
    out.fill(Qt::transparent);

    QPainter painter{&out};
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawPixmap(QRectF(QPointF(0, 0), fitted), source, QRectF(region));
    painter.end();

    // Sized here rather than in the .ui, which constrains the label to a square
    // that would crop the lockup. scaledContents off so QLabel does not stretch
    // it back out of proportion.
    ui->icon->setScaledContents(false);
    ui->icon->setFixedSize(fitted.toSize());
    ui->icon->setPixmap(out);
}

QIcon MainWindow::statusGlyph(const QString &name, bool tint) const
{
    const qreal dpr = this->devicePixelRatioF();
    return tint ? icons()->fittedTinted(name, kStatusBarIconSize, kStatusGlyphFill, dpr)
                : icons()->fitted(name, kStatusBarIconSize, kStatusGlyphFill, dpr);
}

QIcon MainWindow::statusDot(const QString &name) const
{
    // Keeps its own colour, since the dot is the status, but is fitted to the
    // dot size rather than filling the button.
    return icons()->fitted(name, kStatusBarIconSize, kStatusDotFill,
                           this->devicePixelRatioF());
}

void MainWindow::applyThemedIcons() {
    // The .ui files carry single-colour artwork; recolour it for the theme.
    const QVector<QPair<QLabel*, QString>> navIcons = {
        {ui->label_3, "portfolio"},  {ui->label_7, "send"},
        {ui->label_8, "receive"},    {ui->label_swap, "swap"},
        {ui->label_4, "history"},
    };
    for (const auto &[label, name] : navIcons) {
        // navGlyph already sizes to the label, and stretching would undo that.
        label->setScaledContents(false);
        label->setAlignment(Qt::AlignCenter);
        label->setPixmap(navGlyph(name, icons()->ink(), label->width(),
                                  label->devicePixelRatioF()));
    }

    // Fitted, not just tinted: these ship with very different padding, so equal
    // button sizes alone still leave them looking mismatched.
    const QVector<QPair<StatusBarButton*, QString>> statusGlyphs = {
        {m_statusAccountSwitcher,  "status_user.svg"},
        {m_statusBtnPassword,      "status_lock.svg"},
        {m_statusBtnPreferences,   "status_settings.svg"},
        {m_statusBtnSeed,          "status_warning.svg"},
        {m_statusBtnProxySettings, "status_tor.svg"},
    };
    for (const auto &[button, name] : statusGlyphs) {
        button->setIcon(this->statusGlyph(name, true));
    }
}

void MainWindow::updateNavigationState(QWidget *active, QPushButton *activeButton)
{
    QList<QWidget*> widgets = {
        ui->portfolioWidget,
        ui->sendWidget,
        ui->receiveWidget,
        ui->swapWidget,
        ui->historyWidget
    };

    QList<QPushButton*> buttons = {
        ui->portfolioButton,
        ui->sendButton,
        ui->receiveButton,
        ui->swapButton,
        ui->historyButton
    };

    const QStringList glyphs = {"portfolio", "send", "receive", "swap", "history"};

    // The dark skin names the selected row in the accent, so its glyph follows
    // the label; the white skin lets the rail carry the accent alone.
    const bool darkSkin =
        conf()->get(Config::skin).toString() == constants::skinNativeDark;
    const QColor activeInk = darkSkin ? QColor(constants::accentColor)
                                      : icons()->ink();

    for (int i = 0; i < widgets.size(); ++i) {
        QWidget *widget = widgets.at(i);
        const bool isActive = widget == active;

        widget->setProperty("active", isActive);

        widget->style()->unpolish(widget);
        widget->style()->polish(widget);
        widget->update();

        // Single-colour pixmaps cannot be recoloured through the stylesheet.
        // The icon is each row's only QLabel.
        if (QLabel *icon = widget->findChild<QLabel*>()) {
            icon->setScaledContents(false);
            icon->setAlignment(Qt::AlignCenter);
            icon->setPixmap(navGlyph(glyphs.at(i), isActive ? activeInk : icons()->ink(),
                                     icon->width(), icon->devicePixelRatioF()));

            auto *fade = qobject_cast<QGraphicsOpacityEffect*>(icon->graphicsEffect());
            if (!fade) {
                fade = new QGraphicsOpacityEffect(icon);
                icon->setGraphicsEffect(fade);
            }
            fade->setOpacity(isActive ? 1.0 : 0.5);
        }
    }

    for (QPushButton* button : buttons) {
        button->setProperty("active", activeButton == button);

        button->style()->unpolish(button);
        button->style()->polish(button);
        button->update();
    }
}

MainWindow::~MainWindow() {
    qDebug() << "~MainWindow" << QThread::currentThreadId();
}
