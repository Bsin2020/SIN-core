// Copyright (c) 2011-2018 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <qt/singui.h>

#include <qt/sinunits.h>
#include <qt/clientmodel.h>
#include <qt/guiconstants.h>
#include <qt/guiutil.h>
#include <qt/modaloverlay.h>
#include <qt/networkstyle.h>
#include <qt/notificator.h>
#include <qt/openuridialog.h>
#include <qt/optionsdialog.h>
#include <qt/optionsmodel.h>
#include <qt/platformstyle.h>
#include <qt/rpcconsole.h>
#include <qt/utilitydialog.h>

#ifdef ENABLE_WALLET
#include <qt/walletframe.h>
#include <qt/walletmodel.h>
#include <qt/walletview.h>
#endif // ENABLE_WALLET

#ifdef Q_OS_MAC
#include <qt/macdockiconhandler.h>
#endif

#include <chainparams.h>
#include <interfaces/handler.h>
#include <interfaces/node.h>
#include <ui_interface.h>
#include <util.h>

// Dash
#include <masternode-sync.h>
#include <qt/masternodelist.h>
//

#include <iostream>

#include <QAction>
#include <QApplication>
#include <QComboBox>
#include <QDateTime>
#include <QDesktopWidget>
#include <QDragEnterEvent>
#include <QListWidget>
#include <QMenuBar>
#include <QMessageBox>
#include <QMimeData>
#include <QProgressDialog>
#include <QProgressBar>
#include <QSettings>
#include <QShortcut>
#include <QStackedWidget>
#include <QStatusBar>
#include <QStyle>
#include <QTimer>
#include <QToolBar>
#include <QVBoxLayout>
#include <QFontDatabase>

#if QT_VERSION < 0x050000
#include <QTextDocument>
#include <QUrl>
#else
#include <QUrlQuery>
#endif

const std::string BitcoinGUI::DEFAULT_UIPLATFORM =
#if defined(Q_OS_MAC)
        "macosx"
#elif defined(Q_OS_WIN)
        "windows"
#else
        "other"
#endif
        ;

BitcoinGUI::BitcoinGUI(interfaces::Node& node, const PlatformStyle *_platformStyle, const NetworkStyle *networkStyle, QWidget *parent) :
    QMainWindow(parent),
    m_node(node),
    platformStyle(_platformStyle)
{
    QSettings settings;
    if (!restoreGeometry(settings.value("MainWindowGeometry").toByteArray())) {
        // Restore failed (perhaps missing setting), center the window
        move(QApplication::desktop()->availableGeometry().center() - frameGeometry().center());
    }

    QString windowTitle = tr("SIN-CORE") + " - ";
#ifdef ENABLE_WALLET
    enableWallet = WalletModel::isWalletEnabled();
#endif // ENABLE_WALLET
    if(enableWallet)
    {
        windowTitle += tr("Wallet");
    } else {
        windowTitle += tr("Node");
    }
    windowTitle += " " + networkStyle->getTitleAddText();
    QApplication::setWindowIcon(networkStyle->getTrayAndWindowIcon());
    setWindowIcon(networkStyle->getTrayAndWindowIcon());
    setWindowTitle(windowTitle);

    rpcConsole = new RPCConsole(node, _platformStyle, 0);
    helpMessageDialog = new HelpMessageDialog(node, this, HelpMessageDialog::cmdline);
#ifdef ENABLE_WALLET
    if(enableWallet)
    {
        /** Create wallet frame and make it the central widget */
        walletFrame = new WalletFrame(_platformStyle, this);
        setCentralWidget(walletFrame);
    } else
#endif // ENABLE_WALLET
    {
        /* When compiled without wallet or -disablewallet is provided,
         * the central widget is the rpc console.
         */
        setCentralWidget(rpcConsole);
    }

    QFontDatabase::addApplicationFont(":/fonts/Hind-Bold");
    QFontDatabase::addApplicationFont(":/fonts/Hind-SemiBold");
    QFontDatabase::addApplicationFont(":/fonts/Hind-Regular");
    QFontDatabase::addApplicationFont(":/fonts/Hind-Light");
    QFontDatabase::addApplicationFont(":/fonts/Montserrat-Bold");
//******** Load CSS ************/
  QFile File(":/css/default");
  File.open(QFile::ReadOnly);
  QString StyleSheet = QLatin1String(File.readAll());
  this->setStyleSheet(StyleSheet);
//******** Load CSS ************/

 // Accept D&D of URIs
    setAcceptDrops(true);

    // Create actions for the toolbar, menu bar and tray/dock icon
    // Needs walletFrame to be initialized
    createActions();

    // Create application menu bar
    createMenuBar();

    // Create the toolbars
    createToolBars();

    // Create system tray icon and notification
    createTrayIcon(networkStyle);

    // Create status bar
    statusBar();

 // Social Media icons
    QFrame* frameSocial = new QFrame();
    frameSocial->setContentsMargins(0, 0, 0, 0);
    frameSocial->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QHBoxLayout* frameSocialLayout = new QHBoxLayout(frameSocial);
    frameSocialLayout->setContentsMargins(16, 0, 16, 0);
    frameSocialLayout->setSpacing(16);
    
    QLabel* explorer = new QLabel();
    explorer->setObjectName(QStringLiteral("explorer"));
    explorer->setMinimumSize(QSize(32, 32));
    explorer->setMaximumSize(QSize(32, 32));
    explorer->setBaseSize(QSize(0, 0));
    explorer->setCursor(QCursor(Qt::PointingHandCursor));
    explorer->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    explorer->setOpenExternalLinks(true);
#ifndef QT_NO_TOOLTIP
    explorer->setToolTip(QApplication::translate("OverviewPage", "Visit SINOVATE Block Explorer.", nullptr));
#endif // QT_NO_TOOLTIP
    explorer->setText(QApplication::translate("OverviewPage", "<a href=\"https://sinovate.io/links/explorer\"><img src=\":/icons/explorer\" width=\"32\" height=\"32\"></a>", nullptr));
            
            QLabel* website = new QLabel();
    website->setObjectName(QStringLiteral("website"));
    website->setMinimumSize(QSize(19, 19));
    website->setMaximumSize(QSize(19, 19));
    website->setBaseSize(QSize(0, 0));
    website->setCursor(QCursor(Qt::PointingHandCursor));
    website->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    website->setOpenExternalLinks(true);
#ifndef QT_NO_TOOLTIP
    website->setToolTip(QApplication::translate("OverviewPage", "Visit SINOVATE Website", nullptr));
#endif // QT_NO_TOOLTIP
    website->setText(QApplication::translate("OverviewPage", "<a href=\"https://sinovate.io\"><img src=\":/icons/website\" width=\"19\" height=\"19\"></a>", nullptr));
                        
            QLabel* youtube = new QLabel();
    youtube->setObjectName(QStringLiteral("youtube"));
    youtube->setMinimumSize(QSize(32, 32));
    youtube->setMaximumSize(QSize(32, 32));
    youtube->setBaseSize(QSize(0, 0));
    youtube->setCursor(QCursor(Qt::PointingHandCursor));
    youtube->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    youtube->setOpenExternalLinks(true);
#ifndef QT_NO_TOOLTIP
    youtube->setToolTip(QApplication::translate("OverviewPage", "Visit SINOVATE Youtube Channel", nullptr));
#endif // QT_NO_TOOLTIP
    youtube->setText(QApplication::translate("OverviewPage", "<a href=\"https://sinovate.io/links/youtube\"><img src=\":/icons/youtube\" width=\"32\" height=\"32\"></a>", nullptr));

            QLabel* twitter = new QLabel();
    twitter->setObjectName(QStringLiteral("twitter"));
    twitter->setMinimumSize(QSize(32, 32));
    twitter->setMaximumSize(QSize(32, 32));
    twitter->setBaseSize(QSize(0, 0));
    twitter->setCursor(QCursor(Qt::PointingHandCursor));
    twitter->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    twitter->setOpenExternalLinks(true);
#ifndef QT_NO_TOOLTIP
    twitter->setToolTip(QApplication::translate("OverviewPage", "Visit SINOVATE Twitter Channel", nullptr));
#endif // QT_NO_TOOLTIP
    twitter->setText(QApplication::translate("OverviewPage", "<a href=\"https://sinovate.io/links/twitter\"><img src=\":/icons/twitter\" width=\"32\" height=\"32\"></a>", nullptr));

            QLabel* discord = new QLabel();
    discord->setObjectName(QStringLiteral("discord"));
    discord->setMinimumSize(QSize(32, 32));
    discord->setMaximumSize(QSize(32, 32));
    discord->setBaseSize(QSize(0, 0));
    discord->setCursor(QCursor(Qt::PointingHandCursor));
    discord->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    discord->setOpenExternalLinks(true);
#ifndef QT_NO_TOOLTIP
    discord->setToolTip(QApplication::translate("OverviewPage", "Join the official SINOVATE Discord community.", nullptr));
#endif // QT_NO_TOOLTIP
    discord->setText(QApplication::translate("OverviewPage", "<a href=\"https://sinovate.io/links/discord\"><img src=\":/icons/discord\" width=\"32\" height=\"32\"></a>", nullptr));
            
            QLabel* github = new QLabel();
    github->setObjectName(QStringLiteral("github"));
    github->setMinimumSize(QSize(32, 32));
    github->setMaximumSize(QSize(32, 32));
    github->setBaseSize(QSize(0, 0));
    github->setCursor(QCursor(Qt::PointingHandCursor));
    github->setAlignment(Qt::AlignLeading|Qt::AlignLeft|Qt::AlignVCenter);
    github->setOpenExternalLinks(true);
#ifndef QT_NO_TOOLTIP
    github->setToolTip(QApplication::translate("OverviewPage", "Visit SINOVATE Github", nullptr));
#endif // QT_NO_TOOLTIP
    github->setText(QApplication::translate("OverviewPage", "<a href=\"https://github.com/SINOVATEblockchain\"><img src=\":/icons/github\" width=\"32\" height=\"32\"></a>", nullptr));
     
    frameSocialLayout->addWidget(website); 
    frameSocialLayout->addWidget(discord);
    frameSocialLayout->addWidget(twitter);
    frameSocialLayout->addWidget(github);     
    frameSocialLayout->addWidget(youtube);
    frameSocialLayout->addWidget(explorer);
    
    
    
            
        // Disable size grip because it looks ugly and nobody needs it
    statusBar()->setSizeGripEnabled(false);

    // Status bar notification icons
    QFrame *frameBlocks = new QFrame();
    frameBlocks->setContentsMargins(0,0,0,0);
    frameBlocks->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
    QHBoxLayout *frameBlocksLayout = new QHBoxLayout(frameBlocks);
    frameBlocksLayout->setContentsMargins(3,0,3,0);
    frameBlocksLayout->setSpacing(3);
    unitDisplayControl = new UnitDisplayStatusBarControl(platformStyle);
    labelWalletEncryptionIcon = new QLabel();
    labelWalletHDStatusIcon = new QLabel();
    labelProxyIcon = new QLabel();
    connectionsControl = new GUIUtil::ClickableLabel();
    labelBlocksIcon = new GUIUtil::ClickableLabel();
    if(enableWallet)
    {
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(unitDisplayControl);
        frameBlocksLayout->addStretch();
        frameBlocksLayout->addWidget(labelWalletEncryptionIcon);
        frameBlocksLayout->addWidget(labelWalletHDStatusIcon);
    }
    frameBlocksLayout->addWidget(labelProxyIcon);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(connectionsControl);
    frameBlocksLayout->addStretch();
    frameBlocksLayout->addWidget(labelBlocksIcon);
    frameBlocksLayout->addStretch();

    // Progress bar and label for blocks download
    progressBarLabel = new QLabel();
    progressBarLabel->setVisible(false);
    progressBar = new GUIUtil::ProgressBar();
    progressBar->setAlignment(Qt::AlignCenter);
    // Dash
    //progressBar->setVisible(false);
    progressBar->setVisible(true);
    //

    // Override style sheet for progress bar for styles that have a segmented progress bar,
    // as they make the text unreadable (workaround for issue #1071)
    // See https://doc.qt.io/qt-5/gallery.html
    QString curStyle = QApplication::style()->metaObject()->className();
    if(curStyle == "QWindowsStyle" || curStyle == "QWindowsXPStyle")
    {
        progressBar->setStyleSheet("QProgressBar { background-color: #e8e8e8; border: 1px solid grey; border-radius: 7px; padding: 1px; text-align: center; } QProgressBar::chunk { background: QLinearGradient(x1: 0, y1: 0, x2: 1, y2: 0, stop: 0 #FF8000, stop: 1 orange); border-radius: 7px; margin: 0px; }");
    }

    statusBar()->addWidget(frameSocial);
    statusBar()->addWidget(progressBarLabel);
    progressBarLabel->setStyleSheet("QToolTip { color: #000000; background-color: #ffffff; border: 1px solid white; } QLabel {color: #fff; }");
    statusBar()->addWidget(progressBar);
    statusBar()->addPermanentWidget(frameBlocks);

    // Install event filter to be able to catch status tip events (QEvent::StatusTip)
    this->installEventFilter(this);

    // Initially wallet actions should be disabled
    setWalletActionsEnabled(false);

    // Subscribe to notifications from core
    subscribeToCoreSignals();

    connect(connectionsControl, SIGNAL(clicked(QPoint)), this, SLOT(toggleNetworkActive()));

    ///start Exhange and Web Links
    connect(openWebsite1, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks_slot1()));
    connect(openWebsite2, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks_slot2()));
    connect(openWebsite3, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks_slot3()));
    connect(openWebsite4, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks_slot4()));
    connect(openWebsite5, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks_slot5()));
    connect(openWebsite6, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks_slot6()));
    connect(openWebsite7, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks_slot7()));
    connect(openWebsite8, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks_slot8()));
    connect(openWebsite9, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks_slot9()));
    
    
    connect(Exchangesite1, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot1()));
    connect(Exchangesite2, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot2()));
    connect(Exchangesite3, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot3()));
    connect(Exchangesite4, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot4()));
    connect(Exchangesite5, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot5()));
    connect(Exchangesite6, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot6()));
    connect(Exchangesite7, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot7()));
    connect(Exchangesite8, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot8()));
    connect(Exchangesite9, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot9()));
    connect(Exchangesite10, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot10()));
    connect(Exchangesite11, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot11()));
    connect(Exchangesite12, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks2_slot12()));

    connect(ResourcesWebsite1, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks3_slot1()));
    connect(ResourcesWebsite2, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks3_slot2()));
    connect(ResourcesWebsite3, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks3_slot3()));
    connect(ResourcesWebsite4, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks3_slot4()));
    connect(ResourcesWebsite5, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks3_slot5()));
    connect(ResourcesWebsite6, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks3_slot6()));
    connect(ResourcesWebsite7, SIGNAL(triggered()), rpcConsole, SLOT(hyperlinks3_slot7()));


    ///end Exhange, Resources and Web Links



    modalOverlay = new ModalOverlay(this->centralWidget());
#ifdef ENABLE_WALLET
    if(enableWallet) {
        connect(walletFrame, SIGNAL(requestedSyncWarningInfo()), this, SLOT(showModalOverlay()));
        connect(labelBlocksIcon, SIGNAL(clicked(QPoint)), this, SLOT(showModalOverlay()));
        connect(progressBar, SIGNAL(clicked(QPoint)), this, SLOT(showModalOverlay()));
    }
#endif
}

BitcoinGUI::~BitcoinGUI()
{
    // Unsubscribe from notifications from core
    unsubscribeFromCoreSignals();

    QSettings settings;
    settings.setValue("MainWindowGeometry", saveGeometry());
    if(trayIcon) // Hide tray icon, as deleting will let it linger until quit (on Ubuntu)
        trayIcon->hide();
#ifdef Q_OS_MAC
    delete appMenuBar;
    MacDockIconHandler::cleanup();
#endif

    delete rpcConsole;
}

void BitcoinGUI::createActions()
{
    QActionGroup *tabGroup = new QActionGroup(this);

    overviewAction = new QAction(platformStyle->SingleColorIcon(":/icons/overview1"), tr("&Overview"), this);
    overviewAction->setStatusTip(tr("Show general overview of wallet"));
    overviewAction->setToolTip(overviewAction->statusTip());
    overviewAction->setCheckable(true);
    overviewAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_1));
    tabGroup->addAction(overviewAction);

    sendCoinsAction = new QAction(platformStyle->SingleColorIcon(":/icons/send1"), tr("&Send"), this);
    sendCoinsAction->setStatusTip(tr("Send coins to a SIN address"));
    sendCoinsAction->setToolTip(sendCoinsAction->statusTip());
    sendCoinsAction->setCheckable(true);
    sendCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_2));
    tabGroup->addAction(sendCoinsAction);

    sendCoinsMenuAction = new QAction(platformStyle->TextColorIcon(":/icons/send"), sendCoinsAction->text(), this);
    sendCoinsMenuAction->setStatusTip(sendCoinsAction->statusTip());
    sendCoinsMenuAction->setToolTip(sendCoinsMenuAction->statusTip());

    //depositCoinsAction = new QAction(platformStyle->SingleColorIcon(":/icons/deposit"), tr("&Timelock"), this);
    //depositCoinsAction->setStatusTip(tr("Timelock coins to a SIN address"));
    //depositCoinsAction->setToolTip(depositCoinsAction->statusTip());
    //depositCoinsAction->setCheckable(true);
    //tabGroup->addAction(depositCoinsAction);

    //depositCoinsMenuAction = new QAction(platformStyle->TextColorIcon(":/icons/deposit"), depositCoinsAction->text(), this);
    //depositCoinsMenuAction->setStatusTip(depositCoinsAction->statusTip());
    //depositCoinsMenuAction->setToolTip(depositCoinsMenuAction->statusTip());

    receiveCoinsAction = new QAction(platformStyle->SingleColorIcon(":/icons/receiving_addresses1"), tr("&Receive"), this);
    receiveCoinsAction->setStatusTip(tr("Request payments (generates QR codes and sin: URIs)"));
    receiveCoinsAction->setToolTip(receiveCoinsAction->statusTip());
    receiveCoinsAction->setCheckable(true);
    receiveCoinsAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_3));
    tabGroup->addAction(receiveCoinsAction);

    receiveCoinsMenuAction = new QAction(platformStyle->TextColorIcon(":/icons/receiving_addresses"), receiveCoinsAction->text(), this);
    receiveCoinsMenuAction->setStatusTip(receiveCoinsAction->statusTip());
    receiveCoinsMenuAction->setToolTip(receiveCoinsMenuAction->statusTip());

    historyAction = new QAction(platformStyle->SingleColorIcon(":/icons/history1"), tr("&Transactions"), this);
    historyAction->setStatusTip(tr("Browse transaction history"));
    historyAction->setToolTip(historyAction->statusTip());
    historyAction->setCheckable(true);
    historyAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_4));
    tabGroup->addAction(historyAction);

#ifdef ENABLE_WALLET
    // Dash
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool()) {
        masternodeAction = new QAction(platformStyle->SingleColorIcon(":/icons/masternodes"), tr("&Infinity Nodes"), this);
        masternodeAction->setStatusTip(tr("Browse Infinitynodes"));
        masternodeAction->setToolTip(masternodeAction->statusTip());
        masternodeAction->setCheckable(true);
#ifdef Q_OS_MAC
        masternodeAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_5));
#else
        masternodeAction->setShortcut(QKeySequence(Qt::ALT + Qt::Key_5));
#endif
        tabGroup->addAction(masternodeAction);
        connect(masternodeAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
        connect(masternodeAction, SIGNAL(triggered()), this, SLOT(gotoMasternodePage()));
    }
    //

    // These showNormalIfMinimized are needed because Send Coins and Receive Coins
    // can be triggered from the tray menu, and need to show the GUI to be useful.
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(overviewAction, SIGNAL(triggered()), this, SLOT(gotoOverviewPage()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    connect(sendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(sendCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoSendCoinsPage()));
    //connect(depositCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    //connect(depositCoinsAction, SIGNAL(triggered()), this, SLOT(gotoDepositCoinsPage()));
    //connect(depositCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    //connect(depositCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoDepositCoinsPage()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(receiveCoinsMenuAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(receiveCoinsMenuAction, SIGNAL(triggered()), this, SLOT(gotoReceiveCoinsPage()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
    connect(historyAction, SIGNAL(triggered()), this, SLOT(gotoHistoryPage()));
#endif // ENABLE_WALLET

    quitAction = new QAction(platformStyle->TextColorIcon(":/icons/quit"), tr("E&xit"), this);
    quitAction->setStatusTip(tr("Quit application"));
    quitAction->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_Q));
    quitAction->setMenuRole(QAction::QuitRole);
    aboutAction = new QAction(platformStyle->TextColorIcon(":/icons/about2"), tr("&About %1").arg(tr("SINOVATE")), this);
    aboutAction->setStatusTip(tr("Show information about %1").arg(tr("SINOVATE")));
    aboutAction->setMenuRole(QAction::AboutRole);
    aboutAction->setEnabled(false);
    aboutQtAction = new QAction(platformStyle->TextColorIcon(":/icons/about_qt"), tr("About &Qt"), this);
    aboutQtAction->setStatusTip(tr("Show information about Qt"));
    aboutQtAction->setMenuRole(QAction::AboutQtRole);
    optionsAction = new QAction(platformStyle->TextColorIcon(":/icons/options"), tr("&Options..."), this);
    optionsAction->setStatusTip(tr("Modify configuration options for %1").arg(tr("SINOVATE")));
    optionsAction->setMenuRole(QAction::PreferencesRole);
    optionsAction->setEnabled(false);
    toggleHideAction = new QAction(platformStyle->TextColorIcon(":/icons/about2"), tr("&Show / Hide"), this);
    toggleHideAction->setStatusTip(tr("Show or hide the main Window"));

    encryptWalletAction = new QAction(platformStyle->TextColorIcon(":/icons/lock_closed"), tr("&Encrypt Wallet..."), this);
    encryptWalletAction->setStatusTip(tr("Encrypt the private keys that belong to your wallet"));
    encryptWalletAction->setCheckable(true);
    backupWalletAction = new QAction(platformStyle->TextColorIcon(":/icons/filesave"), tr("&Backup Wallet..."), this);
    backupWalletAction->setStatusTip(tr("Backup wallet to another location"));
    changePassphraseAction = new QAction(platformStyle->TextColorIcon(":/icons/key"), tr("&Change Passphrase..."), this);
    changePassphraseAction->setStatusTip(tr("Change the passphrase used for wallet encryption"));

    // Dash
    // 0: menu items
    //-//unlockWalletAction = new QAction(tr("&Unlock Wallet..."), this);
    //-//unlockWalletAction->setToolTip(tr("Unlock wallet"));
    //-//lockWalletAction = new QAction(tr("&Lock Wallet"), this);
    //

    signMessageAction = new QAction(platformStyle->TextColorIcon(":/icons/edit"), tr("Sign &message..."), this);
    signMessageAction->setStatusTip(tr("Sign messages with your SIN addresses to prove you own them"));
    verifyMessageAction = new QAction(platformStyle->TextColorIcon(":/icons/verify"), tr("&Verify message..."), this);
    verifyMessageAction->setStatusTip(tr("Verify messages to ensure they were signed with specified SIN addresses"));

    // Dash
    //-//openInfoAction = new QAction(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation), tr("&Information"), this);
    //-//openInfoAction->setStatusTip(tr("Show diagnostic information"));
    //-//openRPCConsoleAction = new QAction(QIcon(":/icons/" + theme + "/debugwindow"), tr("&Debug console"), this);
    //-//openRPCConsoleAction->setStatusTip(tr("Open debugging console"));
    //-//openGraphAction = new QAction(QIcon(":/icons/" + theme + "/connect_4"), tr("&Network Monitor"), this);
    //-//openGraphAction->setStatusTip(tr("Show network monitor"));
    //-//openPeersAction = new QAction(QIcon(":/icons/" + theme + "/connect_4"), tr("&Peers list"), this);
    //-//openPeersAction->setStatusTip(tr("Show peers info"));
    //-//openRepairAction = new QAction(QIcon(":/icons/" + theme + "/options"), tr("Wallet &Repair"), this);
    //-//openRepairAction->setStatusTip(tr("Show wallet repair options"));
    //-//openConfEditorAction = new QAction(QIcon(":/icons/" + theme + "/edit"), tr("Open Wallet &Configuration File"), this);
    //-//openConfEditorAction->setStatusTip(tr("Open configuration file"));
    openMNConfEditorAction = new QAction(platformStyle->TextColorIcon(":/icons/edit"), tr("Open &Infinitynode Configuration File"), this);
    openMNConfEditorAction->setStatusTip(tr("Open InfinityNode configuration file"));
    //-//showBackupsAction = new QAction(QIcon(":/icons/" + theme + "/browse"), tr("Show Automatic &Backups"), this);
    //-//showBackupsAction->setStatusTip(tr("Show automatically created wallet backups"));
    // initially disable the debug window menu items
    //-//openInfoAction->setEnabled(false);
    //-// ## duplicate ## openRPCConsoleAction->setEnabled(false);
    //-//openGraphAction->setEnabled(false);
    //-//openPeersAction->setEnabled(false);
    //-//openRepairAction->setEnabled(false);

    //

    openRPCConsoleAction = new QAction(platformStyle->TextColorIcon(":/icons/debugwindow"), tr("&Debug window"), this);
    openRPCConsoleAction->setStatusTip(tr("Open debugging and diagnostic console"));
    // initially disable the debug window menu item
    openRPCConsoleAction->setEnabled(false);

    usedSendingAddressesAction = new QAction(platformStyle->TextColorIcon(":/icons/address-book"), tr("&Sending addresses..."), this);
    usedSendingAddressesAction->setStatusTip(tr("Show the list of used sending addresses and labels"));
    usedReceivingAddressesAction = new QAction(platformStyle->TextColorIcon(":/icons/address-book"), tr("&Receiving addresses..."), this);
    usedReceivingAddressesAction->setStatusTip(tr("Show the list of used receiving addresses and labels"));

    openAction = new QAction(platformStyle->TextColorIcon(":/icons/open"), tr("Open &URI..."), this);
    openAction->setStatusTip(tr("Open a sin: URI or payment request"));

    showHelpMessageAction = new QAction(platformStyle->TextColorIcon(":/icons/info"), tr("&Command-line options"), this);
    showHelpMessageAction->setMenuRole(QAction::NoRole);
    showHelpMessageAction->setStatusTip(tr("Show the %1 help message to get a list with possible SIN command-line options").arg(tr("SINOVATE")));

    showSpecsHelpAction = new QAction(QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation), tr("&Specifications"), this);
    showSpecsHelpAction->setMenuRole(QAction::NoRole);
    showSpecsHelpAction->setStatusTip(tr("Show the Specifications"));



    // Dash

    //-//connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    //-//connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    //-//connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    //-//connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    //-//connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    //-//connect(showHelpMessageAction, SIGNAL(triggered()), this, SLOT(showHelpMessageClicked()));

    // Jump directly to tabs in RPC-console
    //-//connect(openInfoAction, SIGNAL(triggered()), this, SLOT(showInfo()));
    //-//connect(openRPCConsoleAction, SIGNAL(triggered()), this, SLOT(showConsole()));
    //-//connect(openGraphAction, SIGNAL(triggered()), this, SLOT(showGraph()));
    //-//connect(openPeersAction, SIGNAL(triggered()), this, SLOT(showPeers()));
    //-//connect(openRepairAction, SIGNAL(triggered()), this, SLOT(showRepair()));

    // Open configs and backup folder from menu
    //-//connect(openConfEditorAction, SIGNAL(triggered()), this, SLOT(showConfEditor()));
    connect(openMNConfEditorAction, SIGNAL(triggered()), this, SLOT(showMNConfEditor()));
    //-//connect(showBackupsAction, SIGNAL(triggered()), this, SLOT(showBackups()));

    // Get restart command-line parameters and handle restart
    //-//connect(rpcConsole, SIGNAL(handleRestart(QStringList)), this, SLOT(handleRestart(QStringList)));
    //

    connect(quitAction, SIGNAL(triggered()), qApp, SLOT(quit()));
    connect(aboutAction, SIGNAL(triggered()), this, SLOT(aboutClicked()));
    connect(aboutQtAction, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(optionsAction, SIGNAL(triggered()), this, SLOT(optionsClicked()));
    connect(toggleHideAction, SIGNAL(triggered()), this, SLOT(toggleHidden()));
    connect(showHelpMessageAction, SIGNAL(triggered()), this, SLOT(showHelpMessageClicked()));
    connect(openRPCConsoleAction, SIGNAL(triggered()), this, SLOT(showDebugWindow()));
    connect(showSpecsHelpAction, SIGNAL(triggered()), this, SLOT(showSpecsHelpClicked()));

    // prevents an open debug window from becoming stuck/unusable on client shutdown
    connect(quitAction, SIGNAL(triggered()), rpcConsole, SLOT(hide()));

//start Exchange and Web Links
    openWebsite1 = new QAction(QIcon(":/icons/website1"), tr("&Sinovate.io"), this);
    openWebsite2 = new QAction(QIcon(":/icons/discord1"), tr("&Discord"), this);
    openWebsite3 = new QAction(QIcon(":/icons/twitter1"), tr("&Twitter"), this);
    openWebsite4 = new QAction(QIcon(":/icons/btctalk1"), tr("&Bitcointalk"), this);
    openWebsite5 = new QAction(QIcon(":/icons/reddit1"), tr("&Reddit"), this);
    openWebsite6 = new QAction(QIcon(":/icons/github1"), tr("&github"), this);
    openWebsite7 = new QAction(QIcon(":/icons/youtube1"), tr("&Youtube"), this);
    openWebsite8 = new QAction(QIcon(":/icons/github1"), tr("&Github"), this);
    openWebsite9 = new QAction(QIcon(":/icons/telegram1"), tr("&Telegram"), this);
    

    Exchangesite1 = new QAction(QIcon(":/icons/cmc"), tr("&Coinmarketcap"), this);
    Exchangesite2 = new QAction(QIcon(":/icons/tradeogre"), tr("&TradeOgre"), this);
    Exchangesite3 = new QAction(QIcon(":/icons/catex"), tr("&Cat.Ex"), this);
    Exchangesite4 = new QAction(QIcon(":/icons/coinsbit"), tr("&Coinsbit"), this);
    Exchangesite5 = new QAction(QIcon(":/icons/crex24"), tr("&Crex24"), this);
    Exchangesite6 = new QAction(QIcon(":/icons/qbtc"), tr("&QBTC"), this);
    Exchangesite7 = new QAction(QIcon(":/icons/txbit"), tr("&Txbit"), this);
    Exchangesite8 = new QAction(QIcon(":/icons/catex"), tr("&Cat.Ex ETH"), this);
    Exchangesite9 = new QAction(QIcon(":/icons/crex24"), tr("&Crex24 ETH"), this);
    Exchangesite10 = new QAction(QIcon(":/icons/stex"), tr("&Stex"), this);
    Exchangesite11 = new QAction(QIcon(":/icons/citex"), tr("&Citex"), this);
    Exchangesite12 = new QAction(QIcon(":/icons/instaswap"), tr("&InstaSwap"), this);

    ResourcesWebsite1 = new QAction(QIcon(":/icons/info"), tr("&Whitepaper"), this);
    ResourcesWebsite2 = new QAction(QIcon(":/icons/info"), tr("&Roadmap"), this);
    ResourcesWebsite3 = new QAction(QIcon(":/icons/info"), tr("&GitBook"), this);
    ResourcesWebsite4 = new QAction(QIcon(":/icons/info"), tr("&sin.conf "), this);
    ResourcesWebsite5 = new QAction(QIcon(":/icons/info"), tr("&Releases"), this);
    ResourcesWebsite6 = new QAction(QIcon(":/icons/explorer1"), tr("&Explorer"), this);
    ResourcesWebsite7 = new QAction(QIcon(":/icons/info"), tr("&Web Tool"), this);




//end Exchange and Web Links

#ifdef ENABLE_WALLET
    if(walletFrame)
    {
        connect(encryptWalletAction, SIGNAL(triggered(bool)), walletFrame, SLOT(encryptWallet(bool)));
        connect(backupWalletAction, SIGNAL(triggered()), walletFrame, SLOT(backupWallet()));
        connect(changePassphraseAction, SIGNAL(triggered()), walletFrame, SLOT(changePassphrase()));

        // Dash
        //-//connect(unlockWalletAction, SIGNAL(triggered()), walletFrame, SLOT(unlockWallet()));
        //connect(lockWalletAction, SIGNAL(triggered()), walletFrame, SLOT(lockWallet()));
        //

        connect(signMessageAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
        connect(signMessageAction, SIGNAL(triggered()), this, SLOT(gotoSignMessageTab()));
        connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(showNormalIfMinimized()));
        connect(verifyMessageAction, SIGNAL(triggered()), this, SLOT(gotoVerifyMessageTab()));
        connect(usedSendingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedSendingAddresses()));
        connect(usedReceivingAddressesAction, SIGNAL(triggered()), walletFrame, SLOT(usedReceivingAddresses()));
        connect(openAction, SIGNAL(triggered()), this, SLOT(openClicked()));
    }
#endif // ENABLE_WALLET

    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C), this, SLOT(showDebugWindowActivateConsole()));
    new QShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_D), this, SLOT(showDebugWindow()));
}

void BitcoinGUI::createMenuBar()
{
#ifdef Q_OS_MAC
    // Create a decoupled menu bar on Mac which stays even if the window is closed
    appMenuBar = new QMenuBar();
#else
    // Get the main window's menu bar on other platforms
    appMenuBar = menuBar();
#endif

    // Configure the menus
    QMenu *file = appMenuBar->addMenu(tr("&File"));
    if(walletFrame)
    {
        file->addAction(openAction);
        file->addAction(backupWalletAction);
        file->addAction(signMessageAction);
        file->addAction(verifyMessageAction);
        file->addSeparator();
        file->addAction(usedSendingAddressesAction);
        file->addAction(usedReceivingAddressesAction);
        file->addSeparator();
    }
    file->addAction(quitAction);

   

    //start exchange Links
    
         if (walletFrame) {
        QMenu* hyperlinks2 = appMenuBar->addMenu(tr("&Exchanges"));
        hyperlinks2->addAction(Exchangesite1);
        hyperlinks2->addSeparator();
        hyperlinks2->addAction(Exchangesite2);
        hyperlinks2->addAction(Exchangesite3);
        hyperlinks2->addAction(Exchangesite4);
        hyperlinks2->addAction(Exchangesite5);
        hyperlinks2->addAction(Exchangesite6);
        hyperlinks2->addAction(Exchangesite7);
        hyperlinks2->addAction(Exchangesite8);
        hyperlinks2->addAction(Exchangesite9);
        hyperlinks2->addAction(Exchangesite10);
        hyperlinks2->addAction(Exchangesite11);
        hyperlinks2->addAction(Exchangesite12);
    }

    //end Exchange Links

    //start Resources Links

    if (walletFrame) {
        QMenu* hyperlinks3 = appMenuBar->addMenu(tr("&Resources"));
        hyperlinks3->addAction(ResourcesWebsite1);
        hyperlinks3->addAction(ResourcesWebsite2);
        hyperlinks3->addAction(ResourcesWebsite3);
        hyperlinks3->addAction(ResourcesWebsite4);
        hyperlinks3->addAction(ResourcesWebsite5);
        hyperlinks3->addAction(ResourcesWebsite6);
        hyperlinks3->addAction(ResourcesWebsite7);
        
    }
    //end Resources Links

    //


    // Dash
    if(walletFrame)
    {
        QMenu *tools = appMenuBar->addMenu(tr("&Tools"));
        //-//tools->addAction(openInfoAction);
        //-//tools->addAction(openRPCConsoleAction);
        //-//tools->addAction(openGraphAction);
        //-//tools->addAction(openPeersAction);
        //-//tools->addAction(openRepairAction);
        //-//tools->addSeparator();
        //-//tools->addAction(openConfEditorAction);
        tools->addAction(openMNConfEditorAction);
        //-//tools->addAction(showBackupsAction);
    }

     //start Web Links

    if (walletFrame) {
        QMenu* hyperlinks = appMenuBar->addMenu(tr("&Social"));
        hyperlinks->addAction(openWebsite1);
        hyperlinks->addSeparator();
        hyperlinks->addAction(openWebsite2);
        hyperlinks->addAction(openWebsite3);
        hyperlinks->addAction(openWebsite4);
        hyperlinks->addAction(openWebsite5);
        hyperlinks->addAction(openWebsite6);
        hyperlinks->addAction(openWebsite7);
        hyperlinks->addAction(openWebsite8);
        hyperlinks->addAction(openWebsite9);
        
    }
    //end Web Links
    //

    QMenu *settings = appMenuBar->addMenu(tr("&Settings"));
    if(walletFrame)
    {
        settings->addAction(encryptWalletAction);
        settings->addAction(changePassphraseAction);

        // Dash
        //-//settings->addAction(unlockWalletAction);
        //-//settings->addAction(lockWalletAction);
        //

        settings->addSeparator();
    }
    settings->addAction(optionsAction);

    QMenu *help = appMenuBar->addMenu(tr("&Help"));
    if(walletFrame)
    {
        help->addAction(openRPCConsoleAction);
    }
    help->addAction(showHelpMessageAction);
    help->addAction(showSpecsHelpAction);
    help->addSeparator();
    help->addAction(aboutAction);
    help->addAction(aboutQtAction);
}

void BitcoinGUI::createToolBars()
{
    if(walletFrame)
    {
        QToolBar *toolbar = addToolBar(tr("Tabs toolbar"));
        appToolBar = toolbar;
        toolbar->setContextMenuPolicy(Qt::PreventContextMenu);
        toolbar->setMovable(false);
        toolbar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toolbar->addAction(overviewAction);
        toolbar->addAction(sendCoinsAction);
        //toolbar->addAction(depositCoinsAction);
        toolbar->addAction(receiveCoinsAction);
        toolbar->addAction(historyAction);
        // Dash
        QSettings settings;
        if (settings.value("fShowMasternodesTab").toBool())
        {
            toolbar->addAction(masternodeAction);
        }
        
        //add LOGO

        QLabel* label = new QLabel();
        label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        toolbar->addWidget(label);

        QLabel* labelLogo = new QLabel();
        labelLogo->setPixmap(QPixmap(":/images/sinovate_logo_horizontal"));
        labelLogo->setStyleSheet("margin-left: 20px");
        toolbar->addWidget(labelLogo);

        //
        overviewAction->setChecked(true);

#ifdef ENABLE_WALLET
        QWidget *spacer = new QWidget();
        spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        toolbar->addWidget(spacer);

        m_wallet_selector = new QComboBox();
        connect(m_wallet_selector, SIGNAL(currentIndexChanged(int)), this, SLOT(setCurrentWalletBySelectorIndex(int)));

        m_wallet_selector_label = new QLabel();
        m_wallet_selector_label->setText(tr("Wallet:") + " ");
        m_wallet_selector_label->setBuddy(m_wallet_selector);

        m_wallet_selector_label_action = appToolBar->addWidget(m_wallet_selector_label);
        m_wallet_selector_action = appToolBar->addWidget(m_wallet_selector);

        m_wallet_selector_label_action->setVisible(false);
        m_wallet_selector_action->setVisible(false);
#endif
    }
}

void BitcoinGUI::setClientModel(ClientModel *_clientModel)
{
    this->clientModel = _clientModel;
    if(_clientModel)
    {
        // Create system tray menu (or setup the dock menu) that late to prevent users from calling actions,
        // while the client has not yet fully loaded
        createTrayIconMenu();

        // Keep up to date with client
        updateNetworkState();
        connect(_clientModel, SIGNAL(numConnectionsChanged(int)), this, SLOT(setNumConnections(int)));
        connect(_clientModel, SIGNAL(networkActiveChanged(bool)), this, SLOT(setNetworkActive(bool)));

        modalOverlay->setKnownBestHeight(_clientModel->getHeaderTipHeight(), QDateTime::fromTime_t(_clientModel->getHeaderTipTime()));
        setNumBlocks(m_node.getNumBlocks(), QDateTime::fromTime_t(m_node.getLastBlockTime()), m_node.getVerificationProgress(), false);
        connect(_clientModel, SIGNAL(numBlocksChanged(int,QDateTime,double,bool)), this, SLOT(setNumBlocks(int,QDateTime,double,bool)));

        // Dash
        connect(clientModel, SIGNAL(additionalDataSyncProgressChanged(double)), this, SLOT(setAdditionalDataSyncProgress(double)));
        //

        // Receive and report messages from client model
        connect(_clientModel, SIGNAL(message(QString,QString,unsigned int)), this, SLOT(message(QString,QString,unsigned int)));

        // Show progress dialog
        connect(_clientModel, SIGNAL(showProgress(QString,int)), this, SLOT(showProgress(QString,int)));

        rpcConsole->setClientModel(_clientModel);

        updateProxyIcon();

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->setClientModel(_clientModel);
        }
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(_clientModel->getOptionsModel());

        OptionsModel* optionsModel = _clientModel->getOptionsModel();
        if(optionsModel)
        {
            // be aware of the tray icon disable state change reported by the OptionsModel object.
            connect(optionsModel,SIGNAL(hideTrayIconChanged(bool)),this,SLOT(setTrayIconVisible(bool)));

            // initialize the disable state of the tray icon with the current value in the model.
            setTrayIconVisible(optionsModel->getHideTrayIcon());
        }
    } else {
        // Disable possibility to show main window via action
        toggleHideAction->setEnabled(false);
        if(trayIconMenu)
        {
            // Disable context menu on tray icon
            trayIconMenu->clear();
        }
        // Propagate cleared model to child objects
        rpcConsole->setClientModel(nullptr);
#ifdef ENABLE_WALLET
        if (walletFrame)
        {
            walletFrame->setClientModel(nullptr);
        }
#endif // ENABLE_WALLET
        unitDisplayControl->setOptionsModel(nullptr);
    }
}

#ifdef ENABLE_WALLET
bool BitcoinGUI::addWallet(WalletModel *walletModel)
{
    if(!walletFrame)
        return false;
    const QString name = walletModel->getWalletName();
    QString display_name = name.isEmpty() ? "["+tr("default wallet")+"]" : name;
    setWalletActionsEnabled(true);
    m_wallet_selector->addItem(display_name, name);
    if (m_wallet_selector->count() == 2) {
        m_wallet_selector_label_action->setVisible(true);
        m_wallet_selector_action->setVisible(true);
    }
    rpcConsole->addWallet(walletModel);
    return walletFrame->addWallet(walletModel);
}

bool BitcoinGUI::removeWallet(WalletModel* walletModel)
{
    if (!walletFrame) return false;
    QString name = walletModel->getWalletName();
    int index = m_wallet_selector->findData(name);
    m_wallet_selector->removeItem(index);
    if (m_wallet_selector->count() == 0) {
        setWalletActionsEnabled(false);
    } else if (m_wallet_selector->count() == 1) {
        m_wallet_selector_label_action->setVisible(false);
        m_wallet_selector_action->setVisible(false);
    }
    rpcConsole->removeWallet(walletModel);
    return walletFrame->removeWallet(name);
}

bool BitcoinGUI::setCurrentWallet(const QString& name)
{
    if(!walletFrame)
        return false;
    return walletFrame->setCurrentWallet(name);
}

bool BitcoinGUI::setCurrentWalletBySelectorIndex(int index)
{
    QString internal_name = m_wallet_selector->itemData(index).toString();
    return setCurrentWallet(internal_name);
}

void BitcoinGUI::removeAllWallets()
{
    if(!walletFrame)
        return;
    setWalletActionsEnabled(false);
    walletFrame->removeAllWallets();
}
#endif // ENABLE_WALLET

void BitcoinGUI::setWalletActionsEnabled(bool enabled)
{
    overviewAction->setEnabled(enabled);
    sendCoinsAction->setEnabled(enabled);
    sendCoinsMenuAction->setEnabled(enabled);
    //depositCoinsAction->setEnabled(enabled);
    //depositCoinsMenuAction->setEnabled(enabled);
    receiveCoinsAction->setEnabled(enabled);
    receiveCoinsMenuAction->setEnabled(enabled);
    historyAction->setEnabled(enabled);

    // Dash
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool() && masternodeAction) {
        masternodeAction->setEnabled(enabled);
    }
    //

    encryptWalletAction->setEnabled(enabled);
    backupWalletAction->setEnabled(enabled);
    changePassphraseAction->setEnabled(enabled);
    signMessageAction->setEnabled(enabled);
    verifyMessageAction->setEnabled(enabled);
    usedSendingAddressesAction->setEnabled(enabled);
    usedReceivingAddressesAction->setEnabled(enabled);
    openAction->setEnabled(enabled);
}

void BitcoinGUI::createTrayIcon(const NetworkStyle *networkStyle)
{
#ifndef Q_OS_MAC
    trayIcon = new QSystemTrayIcon(this);
    QString toolTip = tr("%1 client").arg(tr("SINOVATE")) + " " + networkStyle->getTitleAddText();
    trayIcon->setToolTip(toolTip);
    trayIcon->setIcon(networkStyle->getTrayAndWindowIcon());
    trayIcon->hide();
#endif

    notificator = new Notificator(QApplication::applicationName(), trayIcon, this);
}

void BitcoinGUI::createTrayIconMenu()
{
#ifndef Q_OS_MAC
    // return if trayIcon is unset (only on non-Mac OSes)
    if (!trayIcon)
        return;

    trayIconMenu = new QMenu(this);
    trayIcon->setContextMenu(trayIconMenu);


//start Exchange and Web Links
    trayIconMenu->addAction(openWebsite1);
    trayIconMenu->addAction(openWebsite2);
    trayIconMenu->addAction(openWebsite3);
    trayIconMenu->addAction(openWebsite4);
    trayIconMenu->addAction(openWebsite5);
    trayIconMenu->addAction(openWebsite6);
    trayIconMenu->addAction(openWebsite7);
    trayIconMenu->addAction(openWebsite8);
    trayIconMenu->addAction(openWebsite9);
       
    trayIconMenu->addAction(Exchangesite1);
    trayIconMenu->addAction(Exchangesite2);
    trayIconMenu->addAction(Exchangesite3);
    trayIconMenu->addAction(Exchangesite4);
    trayIconMenu->addAction(Exchangesite5);
    trayIconMenu->addAction(Exchangesite6);
    trayIconMenu->addAction(Exchangesite7);
    trayIconMenu->addAction(Exchangesite8);
    trayIconMenu->addAction(Exchangesite9);
    trayIconMenu->addAction(Exchangesite10);
    trayIconMenu->addAction(Exchangesite11);
    trayIconMenu->addAction(Exchangesite12);

    trayIconMenu->addAction(ResourcesWebsite1);
    trayIconMenu->addAction(ResourcesWebsite2);
    trayIconMenu->addAction(ResourcesWebsite3);
    trayIconMenu->addAction(ResourcesWebsite4);
    trayIconMenu->addAction(ResourcesWebsite5);
    trayIconMenu->addAction(ResourcesWebsite6);
    trayIconMenu->addAction(ResourcesWebsite7);


//end Exchange and Web Links


    connect(trayIcon, SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
            this, SLOT(trayIconActivated(QSystemTrayIcon::ActivationReason)));
#else
    // Note: On Mac, the dock icon is used to provide the tray's functionality.
    MacDockIconHandler *dockIconHandler = MacDockIconHandler::instance();
    connect(dockIconHandler, &MacDockIconHandler::dockIconClicked, this, &BitcoinGUI::macosDockIconActivated);

    trayIconMenu = new QMenu(this);
    trayIconMenu->setAsDockMenu();
#endif

    // Configuration of the tray icon (or Dock icon) menu
#ifndef Q_OS_MAC
    // Note: On macOS, the Dock icon's menu already has Show / Hide action.
    trayIconMenu->addAction(toggleHideAction);
    trayIconMenu->addSeparator();
#endif
    if (enableWallet) {
        trayIconMenu->addAction(sendCoinsMenuAction);
        //trayIconMenu->addAction(depositCoinsMenuAction);
        trayIconMenu->addAction(receiveCoinsMenuAction);
        trayIconMenu->addSeparator();
        trayIconMenu->addAction(signMessageAction);
        trayIconMenu->addAction(verifyMessageAction);
        trayIconMenu->addSeparator();
        trayIconMenu->addAction(openRPCConsoleAction);
    }
    trayIconMenu->addAction(optionsAction);
    trayIconMenu->addAction(openRPCConsoleAction);

    // Dash
    //-//trayIconMenu->addAction(openGraphAction);
    //-//trayIconMenu->addAction(openPeersAction);
    //-//trayIconMenu->addAction(openRepairAction);
    //-//trayIconMenu->addSeparator();
    //-//trayIconMenu->addAction(openConfEditorAction);
    trayIconMenu->addAction(openMNConfEditorAction);
    //-//trayIconMenu->addAction(showBackupsAction);
    //

#ifndef Q_OS_MAC // This is built-in on macOS
    trayIconMenu->addSeparator();
    trayIconMenu->addAction(quitAction);
#endif
}

#ifndef Q_OS_MAC
void BitcoinGUI::trayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    if(reason == QSystemTrayIcon::Trigger)
    {
        // Click on system tray icon triggers show/hide of the main window
        toggleHidden();
    }
}
#else
void BitcoinGUI::macosDockIconActivated()
{
    show();
    activateWindow();
}
#endif

void BitcoinGUI::optionsClicked()
{
    if(!clientModel || !clientModel->getOptionsModel())
        return;

    OptionsDialog dlg(this, enableWallet);
    dlg.setModel(clientModel->getOptionsModel());
    dlg.exec();
}

void BitcoinGUI::aboutClicked()
{
    if(!clientModel)
        return;

    HelpMessageDialog dlg(m_node, this, HelpMessageDialog::about);
    dlg.exec();
}

void BitcoinGUI::showDebugWindow()
{
    GUIUtil::bringToFront(rpcConsole);
}

void BitcoinGUI::showDebugWindowActivateConsole()
{
    rpcConsole->setTabFocus(RPCConsole::TAB_CONSOLE);
    showDebugWindow();
}

// Dash
void BitcoinGUI::showMNConfEditor()
{
    GUIUtil::openMNConfigfile();
}
//

void BitcoinGUI::showHelpMessageClicked()
{
    helpMessageDialog->show();
}

void BitcoinGUI::showSpecsHelpClicked()
{
    if(!clientModel)
        return;

    HelpMessageDialog dlg(m_node, this, HelpMessageDialog::pshelp);
    dlg.exec();

}

#ifdef ENABLE_WALLET
void BitcoinGUI::openClicked()
{
    OpenURIDialog dlg(this);
    if(dlg.exec())
    {
        Q_EMIT receivedURI(dlg.getURI());
    }
}

void BitcoinGUI::gotoOverviewPage()
{
    overviewAction->setChecked(true);
    if (walletFrame) walletFrame->gotoOverviewPage();
}

void BitcoinGUI::gotoHistoryPage()
{
    historyAction->setChecked(true);
    if (walletFrame) walletFrame->gotoHistoryPage();
}

// Dash
void BitcoinGUI::gotoMasternodePage()
{
    QSettings settings;
    if (settings.value("fShowMasternodesTab").toBool()) {
        masternodeAction->setChecked(true);
        if (walletFrame) walletFrame->gotoMasternodePage();
    }
}
//

void BitcoinGUI::gotoReceiveCoinsPage()
{
    receiveCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoReceiveCoinsPage();
}

void BitcoinGUI::gotoSendCoinsPage(QString addr)
{
    sendCoinsAction->setChecked(true);
    if (walletFrame) walletFrame->gotoSendCoinsPage(addr);
}

//void BitcoinGUI::gotoDepositCoinsPage(QString addr)
//{
  //  depositCoinsAction->setChecked(true);
    //if (walletFrame) walletFrame->gotoDepositCoinsPage(addr);
//}

void BitcoinGUI::gotoSignMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoSignMessageTab(addr);
}

void BitcoinGUI::gotoVerifyMessageTab(QString addr)
{
    if (walletFrame) walletFrame->gotoVerifyMessageTab(addr);
}
#endif // ENABLE_WALLET

void BitcoinGUI::updateNetworkState()
{
    int count = clientModel->getNumConnections();
    QString icon;
    switch(count)
    {
    case 0: icon = ":/icons/connect_0"; break;
    case 1: case 2: case 3: icon = ":/icons/connect_1"; break;
    case 4: case 5: case 6: icon = ":/icons/connect_2"; break;
    case 7: case 8: case 9: icon = ":/icons/connect_3"; break;
    default: icon = ":/icons/connect_4"; break;
    }

    QString tooltip;

    if (m_node.getNetworkActive()) {
        tooltip = tr("%n active connection(s) to SIN network", "", count) + QString(".<br>") + tr("Click to disable network activity.");
    } else {
        tooltip = tr("Network activity disabled.") + QString("<br>") + tr("Click to enable network activity again.");
        icon = ":/icons/network_disabled";
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");
    connectionsControl->setToolTip(tooltip);

    connectionsControl->setPixmap(platformStyle->SingleColorIcon(icon).pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
}

void BitcoinGUI::setNumConnections(int count)
{
    updateNetworkState();
}

void BitcoinGUI::setNetworkActive(bool networkActive)
{
    updateNetworkState();
}

void BitcoinGUI::updateHeadersSyncProgressLabel()
{
    int64_t headersTipTime = clientModel->getHeaderTipTime();
    int headersTipHeight = clientModel->getHeaderTipHeight();
    int estHeadersLeft = (GetTime() - headersTipTime) / Params().GetConsensus().nPowTargetSpacing;
    if (estHeadersLeft > HEADER_HEIGHT_DELTA_SYNC)
        progressBarLabel->setText(tr("Syncing Headers (%1%)...").arg(QString::number(100.0 / (headersTipHeight+estHeadersLeft)*headersTipHeight, 'f', 1)));
}

void BitcoinGUI::setNumBlocks(int count, const QDateTime& blockDate, double nVerificationProgress, bool header)
{
    if (modalOverlay)
    {
        if (header)
            modalOverlay->setKnownBestHeight(count, blockDate);
        else
            modalOverlay->tipUpdate(count, blockDate, nVerificationProgress);
    }
    if (!clientModel)
        return;

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbled text)
    statusBar()->clearMessage();

    // Acquire current block source
    enum BlockSource blockSource = clientModel->getBlockSource();
    switch (blockSource) {
        case BlockSource::NETWORK:
            if (header) {
                updateHeadersSyncProgressLabel();
                return;
            }
            progressBarLabel->setText(tr("Synchronizing with network..."));
            updateHeadersSyncProgressLabel();
            break;
        case BlockSource::DISK:
            if (header) {
                progressBarLabel->setText(tr("Indexing blocks on disk..."));
            } else {
                progressBarLabel->setText(tr("Processing blocks on disk..."));
            }
            break;
        case BlockSource::REINDEX:
            progressBarLabel->setText(tr("Reindexing blocks on disk..."));
            break;
        case BlockSource::NONE:
            if (header) {
                return;
            }
            progressBarLabel->setText(tr("Connecting to peers..."));
            break;
    }

    QString tooltip;

    QDateTime currentDate = QDateTime::currentDateTime();
    qint64 secs = blockDate.secsTo(currentDate);

    tooltip = tr("Processed %n block(s) of transaction history.", "", count);

    // Set icon state: spinning if catching up, tick otherwise
    if(secs < 90*60)
    {
        tooltip = tr("Up to date") + QString(".<br>") + tooltip;
        labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->showOutOfSyncWarning(false);
            modalOverlay->showHide(true, true);

            if(secs < 25*60)
                modalOverlay->hideForever();
        }
#endif // ENABLE_WALLET
        // Dash
        //progressBarLabel->setVisible(false);
        //progressBar->setVisible(false);
        //
    }
    // Dash
    //else
    if(!masternodeSync.IsBlockchainSynced())
    {
    //
        QString timeBehindText = GUIUtil::formatNiceTimeOffset(secs);

        progressBarLabel->setVisible(true);
        progressBar->setFormat(tr("%1 behind").arg(timeBehindText));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nVerificationProgress * 1000000000.0 + 0.5);
        progressBar->setVisible(true);

        tooltip = tr("Catching up...") + QString("<br>") + tooltip;
        if(count != prevBlocks)
        {
            labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(QString(
                ":/movies/spinner-%1").arg(spinnerFrame, 3, 10, QChar('0')))
                .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;
        }
        prevBlocks = count;

#ifdef ENABLE_WALLET
        if(walletFrame)
        {
            walletFrame->showOutOfSyncWarning(true);
            modalOverlay->showHide();
        }
#endif // ENABLE_WALLET

        tooltip += QString("<br>");
        tooltip += tr("Last received block was generated %1 ago.").arg(timeBehindText);
        tooltip += QString("<br>");
        tooltip += tr("Transactions after this will not yet be visible.");
    }

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}

// Dash
void BitcoinGUI::setAdditionalDataSyncProgress(double nSyncProgress)
{
    if(!clientModel)
        return;

    // No additional data sync should be happening while blockchain is not synced, nothing to update
    if(!masternodeSync.IsBlockchainSynced())
        return;

    // Prevent orphan statusbar messages (e.g. hover Quit in main menu, wait until chain-sync starts -> garbelled text)
    statusBar()->clearMessage();

    QString tooltip;

    // Set icon state: spinning if catching up, tick otherwise
    //QString theme = GUIUtil::getThemeName();

    QString strSyncStatus;
    tooltip = tr("Up to date") + QString(".<br>") + tooltip;

    if(masternodeSync.IsSynced()) {
        progressBarLabel->setVisible(false);
        progressBar->setVisible(false);
        //labelBlocksIcon->setPixmap(QIcon(":/icons/" + theme + "/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/synced").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        //
    } else {

        labelBlocksIcon->setPixmap(platformStyle->SingleColorIcon(QString(
            ":/movies/spinner-%1").arg(spinnerFrame, 3, 10, QChar('0')))
            .pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
        spinnerFrame = (spinnerFrame + 1) % SPINNER_FRAMES;

#ifdef ENABLE_WALLET
        if(walletFrame)
            walletFrame->showOutOfSyncWarning(false);
#endif // ENABLE_WALLET

        progressBar->setFormat(tr("Synchronizing additional data: %p%"));
        progressBar->setMaximum(1000000000);
        progressBar->setValue(nSyncProgress * 1000000000.0 + 0.5);
    }

    strSyncStatus = QString(masternodeSync.GetSyncStatus().c_str());
    progressBarLabel->setText(strSyncStatus);
    tooltip = strSyncStatus + QString("<br>") + tooltip;

    // Don't word-wrap this (fixed-width) tooltip
    tooltip = QString("<nobr>") + tooltip + QString("</nobr>");

    labelBlocksIcon->setToolTip(tooltip);
    progressBarLabel->setToolTip(tooltip);
    progressBar->setToolTip(tooltip);
}
//

void BitcoinGUI::message(const QString &title, const QString &message, unsigned int style, bool *ret)
{
    QString strTitle = tr("SIN"); // default title
    // Default to information icon
    int nMBoxIcon = QMessageBox::Information;
    int nNotifyIcon = Notificator::Information;

    QString msgType;

    // Prefer supplied title over style based title
    if (!title.isEmpty()) {
        msgType = title;
    }
    else {
        switch (style) {
        case CClientUIInterface::MSG_ERROR:
            msgType = tr("Error");
            break;
        case CClientUIInterface::MSG_WARNING:
            msgType = tr("Warning");
            break;
        case CClientUIInterface::MSG_INFORMATION:
            msgType = tr("Information");
            break;
        default:
            break;
        }
    }
    // Append title to "SIN - "
    if (!msgType.isEmpty())
        strTitle += " - " + msgType;

    // Check for error/warning icon
    if (style & CClientUIInterface::ICON_ERROR) {
        nMBoxIcon = QMessageBox::Critical;
        nNotifyIcon = Notificator::Critical;
    }
    else if (style & CClientUIInterface::ICON_WARNING) {
        nMBoxIcon = QMessageBox::Warning;
        nNotifyIcon = Notificator::Warning;
    }

    // Display message
    if (style & CClientUIInterface::MODAL) {
        // Check for buttons, use OK as default, if none was supplied
        QMessageBox::StandardButton buttons;
        if (!(buttons = (QMessageBox::StandardButton)(style & CClientUIInterface::BTN_MASK)))
            buttons = QMessageBox::Ok;

        showNormalIfMinimized();
        QMessageBox mBox(static_cast<QMessageBox::Icon>(nMBoxIcon), strTitle, message, buttons, this);
        mBox.setTextFormat(Qt::PlainText);
        int r = mBox.exec();
        if (ret != nullptr)
            *ret = r == QMessageBox::Ok;
    }
    else
        notificator->notify(static_cast<Notificator::Class>(nNotifyIcon), strTitle, message);
}

void BitcoinGUI::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
#ifndef Q_OS_MAC // Ignored on Mac
    if(e->type() == QEvent::WindowStateChange)
    {
        if(clientModel && clientModel->getOptionsModel() && clientModel->getOptionsModel()->getMinimizeToTray())
        {
            QWindowStateChangeEvent *wsevt = static_cast<QWindowStateChangeEvent*>(e);
            if(!(wsevt->oldState() & Qt::WindowMinimized) && isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(hide()));
                e->ignore();
            }
            else if((wsevt->oldState() & Qt::WindowMinimized) && !isMinimized())
            {
                QTimer::singleShot(0, this, SLOT(show()));
                e->ignore();
            }
        }
    }
#endif
}

void BitcoinGUI::closeEvent(QCloseEvent *event)
{
#ifndef Q_OS_MAC // Ignored on Mac
    if(clientModel && clientModel->getOptionsModel())
    {
        if(!clientModel->getOptionsModel()->getMinimizeOnClose())
        {
            // close rpcConsole in case it was open to make some space for the shutdown window
            rpcConsole->close();

            QApplication::quit();
        }
        else
        {
            QMainWindow::showMinimized();
            event->ignore();
        }
    }
#else
    QMainWindow::closeEvent(event);
#endif
}

void BitcoinGUI::showEvent(QShowEvent *event)
{
    // enable the debug window when the main window shows up
    openRPCConsoleAction->setEnabled(true);
    aboutAction->setEnabled(true);
    optionsAction->setEnabled(true);
}

#ifdef ENABLE_WALLET
void BitcoinGUI::incomingTransaction(const QString& date, int unit, const CAmount& amount, const QString& type, const QString& address, const QString& label, const QString& walletName)
{
    // On new transaction, make an info balloon
    QString msg = tr("Date: %1\n").arg(date) +
                  tr("Amount: %1\n").arg(BitcoinUnits::formatWithUnit(unit, amount, true));
    if (m_node.getWallets().size() > 1 && !walletName.isEmpty()) {
        msg += tr("Wallet: %1\n").arg(walletName);
    }
    msg += tr("Type: %1\n").arg(type);
    if (!label.isEmpty())
        msg += tr("Label: %1\n").arg(label);
    else if (!address.isEmpty())
        msg += tr("Address: %1\n").arg(address);
    message((amount)<0 ? tr("Sent transaction") : tr("Incoming transaction"),
             msg, CClientUIInterface::MSG_INFORMATION);
}
#endif // ENABLE_WALLET

void BitcoinGUI::dragEnterEvent(QDragEnterEvent *event)
{
    // Accept only URIs
    if(event->mimeData()->hasUrls())
        event->acceptProposedAction();
}

void BitcoinGUI::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasUrls())
    {
        for (const QUrl &uri : event->mimeData()->urls())
        {
            Q_EMIT receivedURI(uri.toString());
        }
    }
    event->acceptProposedAction();
}

bool BitcoinGUI::eventFilter(QObject *object, QEvent *event)
{
    // Catch status tip events
    if (event->type() == QEvent::StatusTip)
    {
        // Prevent adding text from setStatusTip(), if we currently use the status bar for displaying other stuff
        if (progressBarLabel->isVisible() || progressBar->isVisible())
            return true;
    }
    return QMainWindow::eventFilter(object, event);
}

#ifdef ENABLE_WALLET
bool BitcoinGUI::handlePaymentRequest(const SendCoinsRecipient& recipient)
{
    // URI has to be valid
    if (walletFrame && walletFrame->handlePaymentRequest(recipient))
    {
        showNormalIfMinimized();
        gotoSendCoinsPage();
        return true;
    }
    return false;
}

void BitcoinGUI::setHDStatus(int hdEnabled)
{
    labelWalletHDStatusIcon->setPixmap(platformStyle->SingleColorIcon(hdEnabled ? ":/icons/hd_enabled" : ":/icons/hd_disabled").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
    labelWalletHDStatusIcon->setToolTip(hdEnabled ? tr("HD key generation is <b>enabled</b>") : tr("HD key generation is <b>disabled</b>"));

    // eventually disable the QLabel to set its opacity to 50%
    labelWalletHDStatusIcon->setEnabled(hdEnabled);
}

void BitcoinGUI::setEncryptionStatus(int status)
{
    switch(status)
    {
    case WalletModel::Unencrypted:
        labelWalletEncryptionIcon->hide();
        encryptWalletAction->setChecked(false);
        changePassphraseAction->setEnabled(false);
        // Dash
        //-//unlockWalletAction->setVisible(false);
        //-//lockWalletAction->setVisible(false);
        //
        encryptWalletAction->setEnabled(true);
        break;
    case WalletModel::Unlocked:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/lock_open1").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>unlocked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        // Dash
        //-//unlockWalletAction->setVisible(false);
        //-//lockWalletAction->setVisible(true);
        //
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    case WalletModel::Locked:
        labelWalletEncryptionIcon->show();
        labelWalletEncryptionIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/lock_closed1").pixmap(STATUSBAR_ICONSIZE,STATUSBAR_ICONSIZE));
        labelWalletEncryptionIcon->setToolTip(tr("Wallet is <b>encrypted</b> and currently <b>locked</b>"));
        encryptWalletAction->setChecked(true);
        changePassphraseAction->setEnabled(true);
        encryptWalletAction->setEnabled(false); // TODO: decrypt currently not supported
        break;
    }
}

void BitcoinGUI::updateWalletStatus()
{
    if (!walletFrame) {
        return;
    }
    WalletView * const walletView = walletFrame->currentWalletView();
    if (!walletView) {
        return;
    }
    WalletModel * const walletModel = walletView->getWalletModel();
    setEncryptionStatus(walletModel->getEncryptionStatus());
    setHDStatus(walletModel->wallet().hdEnabled());
}
#endif // ENABLE_WALLET

void BitcoinGUI::updateProxyIcon()
{
    std::string ip_port;
    bool proxy_enabled = clientModel->getProxyInfo(ip_port);

    if (proxy_enabled) {
        if (labelProxyIcon->pixmap() == 0) {
            QString ip_port_q = QString::fromStdString(ip_port);
            labelProxyIcon->setPixmap(platformStyle->SingleColorIcon(":/icons/proxy").pixmap(STATUSBAR_ICONSIZE, STATUSBAR_ICONSIZE));
            labelProxyIcon->setToolTip(tr("Proxy is <b>enabled</b>: %1").arg(ip_port_q));
        } else {
            labelProxyIcon->show();
        }
    } else {
        labelProxyIcon->hide();
    }
}

void BitcoinGUI::showNormalIfMinimized(bool fToggleHidden)
{
    if(!clientModel)
        return;

    if (!isHidden() && !isMinimized() && !GUIUtil::isObscured(this) && fToggleHidden) {
        hide();
    } else {
        GUIUtil::bringToFront(this);
    }
}

void BitcoinGUI::toggleHidden()
{
    showNormalIfMinimized(true);
}

void BitcoinGUI::detectShutdown()
{
    if (m_node.shutdownRequested())
    {
        if(rpcConsole)
            rpcConsole->hide();
        qApp->quit();
    }
}

void BitcoinGUI::showProgress(const QString &title, int nProgress)
{
    if (nProgress == 0)
    {
        progressDialog = new QProgressDialog(title, "", 0, 100);
        progressDialog->setWindowModality(Qt::ApplicationModal);
        progressDialog->setMinimumDuration(0);
        progressDialog->setCancelButton(0);
        progressDialog->setAutoClose(false);
        progressDialog->setValue(0);
    }
    else if (nProgress == 100)
    {
        if (progressDialog)
        {
            progressDialog->close();
            progressDialog->deleteLater();
        }
    }
    else if (progressDialog)
        progressDialog->setValue(nProgress);
}

void BitcoinGUI::setTrayIconVisible(bool fHideTrayIcon)
{
    if (trayIcon)
    {
        trayIcon->setVisible(!fHideTrayIcon);
    }
}

void BitcoinGUI::showModalOverlay()
{
    if (modalOverlay && (progressBar->isVisible() || modalOverlay->isLayerVisible()))
        modalOverlay->toggleVisibility();
}

static bool ThreadSafeMessageBox(BitcoinGUI *gui, const std::string& message, const std::string& caption, unsigned int style)
{
    bool modal = (style & CClientUIInterface::MODAL);
    // The SECURE flag has no effect in the Qt GUI.
    // bool secure = (style & CClientUIInterface::SECURE);
    style &= ~CClientUIInterface::SECURE;
    bool ret = false;
    // In case of modal message, use blocking connection to wait for user to click a button
    QMetaObject::invokeMethod(gui, "message",
                               modal ? GUIUtil::blockingGUIThreadConnection() : Qt::QueuedConnection,
                               Q_ARG(QString, QString::fromStdString(caption)),
                               Q_ARG(QString, QString::fromStdString(message)),
                               Q_ARG(unsigned int, style),
                               Q_ARG(bool*, &ret));
    return ret;
}

void BitcoinGUI::subscribeToCoreSignals()
{
    // Connect signals to client
    m_handler_message_box = m_node.handleMessageBox(boost::bind(ThreadSafeMessageBox, this, _1, _2, _3));
    m_handler_question = m_node.handleQuestion(boost::bind(ThreadSafeMessageBox, this, _1, _3, _4));
}

void BitcoinGUI::unsubscribeFromCoreSignals()
{
    // Disconnect signals from client
    m_handler_message_box->disconnect();
    m_handler_question->disconnect();
}

void BitcoinGUI::toggleNetworkActive()
{
    m_node.setNetworkActive(!m_node.getNetworkActive());
}

UnitDisplayStatusBarControl::UnitDisplayStatusBarControl(const PlatformStyle *platformStyle) :
    optionsModel(0),
    menu(0)
{
    createContextMenu();
    setToolTip(tr("Unit to show amounts in. Click to select another unit."));
    QList<BitcoinUnits::Unit> units = BitcoinUnits::availableUnits();
    int max_width = 0;
    const QFontMetrics fm(font());
    for (const BitcoinUnits::Unit unit : units)
    {
        max_width = qMax(max_width, fm.width(BitcoinUnits::longName(unit)));
    }
    setMinimumSize(max_width, 0);
    setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    setStyleSheet(QString("QLabel { color : #EF3C23; font-weight: bold; }").arg(platformStyle->SingleColor().name()));
}

/** So that it responds to button clicks */
void UnitDisplayStatusBarControl::mousePressEvent(QMouseEvent *event)
{
    onDisplayUnitsClicked(event->pos());
}

/** Creates context menu, its actions, and wires up all the relevant signals for mouse events. */
void UnitDisplayStatusBarControl::createContextMenu()
{
    menu = new QMenu(this);
    for (BitcoinUnits::Unit u : BitcoinUnits::availableUnits())
    {
        QAction *menuAction = new QAction(QString(BitcoinUnits::longName(u)), this);
        menuAction->setData(QVariant(u));
        menu->addAction(menuAction);
    }
    connect(menu,SIGNAL(triggered(QAction*)),this,SLOT(onMenuSelection(QAction*)));
}

/** Lets the control know about the Options Model (and its signals) */
void UnitDisplayStatusBarControl::setOptionsModel(OptionsModel *_optionsModel)
{
    if (_optionsModel)
    {
        this->optionsModel = _optionsModel;

        // be aware of a display unit change reported by the OptionsModel object.
        connect(_optionsModel,SIGNAL(displayUnitChanged(int)),this,SLOT(updateDisplayUnit(int)));

        // initialize the display units label with the current value in the model.
        updateDisplayUnit(_optionsModel->getDisplayUnit());
    }
}

/** When Display Units are changed on OptionsModel it will refresh the display text of the control on the status bar */
void UnitDisplayStatusBarControl::updateDisplayUnit(int newUnits)
{
    setText(BitcoinUnits::longName(newUnits));
}

/** Shows context menu with Display Unit options by the mouse coordinates */
void UnitDisplayStatusBarControl::onDisplayUnitsClicked(const QPoint& point)
{
    QPoint globalPos = mapToGlobal(point);
    menu->exec(globalPos);
}

/** Tells underlying optionsModel to update its current display unit. */
void UnitDisplayStatusBarControl::onMenuSelection(QAction* action)
{
    if (action)
    {
        optionsModel->setDisplayUnit(action->data());
    }
}
