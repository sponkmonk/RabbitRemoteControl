// Author: Kang Lin <kl222@126.com>

#include "mainwindow.h"
#include "./ui_mainwindow.h"
#ifdef HAVE_UPDATE
#include "FrmUpdater/FrmUpdater.h"
#endif
#include "RabbitCommonDir.h"
#include "RabbitCommonTools.h"
#include "FrmStyle/FrmStyle.h"

#ifdef HAVE_ABOUT
#include "DlgAbout/DlgAbout.h"
#endif
#ifdef BUILD_QUIWidget
    #include "QUIWidget/QUIWidget.h"
#endif

#include "Connecter.h"
#include "FrmFullScreenToolBar.h"
#include "ParameterDlgSettings.h"
#include "FrmOpenConnect.h"

#ifdef HAVE_ICE
    #include "Ice.h"
#endif

#include <QMessageBox>
#include <QScreen>
#include <QApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QWidgetAction>
#include <QMouseEvent>
#include <QDrag>
#include <QMimeData>
#include <QKeySequence>
#include <QPushButton>
#include <QDateTime>
#include <QLoggingCategory>

Q_DECLARE_LOGGING_CATEGORY(App)

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
      m_pSignalStatus(nullptr),
      ui(new Ui::MainWindow),
      m_pView(nullptr),
      m_pFullScreenToolBar(nullptr),
      m_ptbZoom(nullptr),
      m_psbZoomFactor(nullptr),
      m_pGBViewZoom(nullptr),
      m_pRecentMenu(nullptr),
      m_pFavoriteDockWidget(nullptr),
      m_pFavoriteView(nullptr)
{
    bool check = false;

    ui->setupUi(this);
    
    //addToolBar(Qt::LeftToolBarArea, ui->toolBar);
    setAcceptDrops(true);
    
    m_pFavoriteDockWidget = new QDockWidget(this);
    if(m_pFavoriteDockWidget)
    {
        m_pFavoriteView = new CFavoriteView(m_pFavoriteDockWidget);
        if(m_pFavoriteView)
        {
            check = connect(m_pFavoriteView, SIGNAL(sigConnect(const QString&, bool)),
                            this, SLOT(slotOpenFile(const QString&, bool)));
            Q_ASSERT(check);
            check = connect(&m_Parameter, SIGNAL(sigFavoriteEditChanged(bool)),
                            m_pFavoriteView, SLOT(slotDoubleEditNode(bool)));
            Q_ASSERT(check);
            m_pFavoriteDockWidget->setWidget(m_pFavoriteView);
        }
        // Must set ObjectName then restore it. See: saveState help document
        m_pFavoriteDockWidget->setObjectName("dckFavorite");
        m_pFavoriteDockWidget->setWindowTitle(tr("Favorite"));
        m_pFavoriteDockWidget->hide();
        ui->actionFavorites->setChecked(false);
        addDockWidget(Qt::DockWidgetArea::LeftDockWidgetArea, m_pFavoriteDockWidget);
        check = connect(m_pFavoriteDockWidget, SIGNAL(visibilityChanged(bool)),
                        this, SLOT(slotDockWidgetFavoriteVisibilityChanged(bool)));
        Q_ASSERT(check);
    }
    
    m_pRecentMenu = new RabbitCommon::CRecentMenu(this);
    check = connect(m_pRecentMenu, SIGNAL(recentFileTriggered(const QString&)),
                    this, SLOT(slotOpenFile(const QString&)));
    Q_ASSERT(check);
    check = connect(&m_Parameter, SIGNAL(sigRecentMenuMaxCountChanged(int)),
                    m_pRecentMenu, SLOT(setMaxCount(int)));
    Q_ASSERT(check);
    ui->actionRecently_connected->setMenu(m_pRecentMenu);
    QToolButton* tbRecent = new QToolButton(ui->toolBar);
    tbRecent->setFocusPolicy(Qt::NoFocus);
    tbRecent->setPopupMode(QToolButton::InstantPopup);
    tbRecent->setMenu(m_pRecentMenu);
    tbRecent->setIcon(ui->actionRecently_connected->icon());
    tbRecent->setText(ui->actionRecently_connected->text());
    tbRecent->setToolTip(ui->actionRecently_connected->toolTip());
    tbRecent->setStatusTip(ui->actionRecently_connected->statusTip());
    ui->toolBar->insertWidget(ui->actionOpen, tbRecent);
    
#ifdef HAVE_UPDATE
    CFrmUpdater updater;
    ui->actionUpdate_U->setIcon(updater.windowIcon());
#endif

    //TODO: Change view
    m_pView = new CViewTable(this);
    if(m_pView)
    {
        m_pView->setFocusPolicy(Qt::NoFocus);
        check = connect(m_pView, SIGNAL(sigCloseView(const QWidget*)),
                        this, SLOT(slotCloseView(const QWidget*)));
        Q_ASSERT(check);
#ifdef USE_FROM_OPENGL
        check = connect(m_pView, SIGNAL(sigAdaptWindows(const CFrmViewerOpenGL::ADAPT_WINDOWS)),
                        this, SLOT(slotAdaptWindows(const CFrmViewerOpenGL::ADAPT_WINDOWS)));
#else
        check = connect(m_pView, SIGNAL(sigAdaptWindows(const CFrmViewer::ADAPT_WINDOWS)),
                        this, SLOT(slotAdaptWindows(const CFrmViewer::ADAPT_WINDOWS)));
#endif
        Q_ASSERT(check);
        this->setCentralWidget(m_pView);
    }
    
    m_Client.EnumPlugins(this);
    m_Client.LoadSettings();

    QToolButton* tbConnect = new QToolButton(ui->toolBar);
    tbConnect->setFocusPolicy(Qt::NoFocus);
    tbConnect->setPopupMode(QToolButton::InstantPopup);
    //tbConnect->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    tbConnect->setMenu(ui->menuConnect_C);
    tbConnect->setIcon(QIcon::fromTheme("network-wired"));
    tbConnect->setText(tr("Connect"));
    tbConnect->setToolTip(tr("Connect"));
    tbConnect->setStatusTip(tr("Connect"));
    ui->toolBar->insertWidget(ui->actionDisconnect_D, tbConnect);
    
    m_ptbZoom = new QToolButton(ui->toolBar);
    m_ptbZoom->setPopupMode(QToolButton::InstantPopup);
    //m_ptbZoom->setToolButtonStyle(Qt::ToolButtonFollowStyle);
    m_ptbZoom->setMenu(ui->menuZoom);
    m_ptbZoom->setIcon(QIcon::fromTheme("zoom"));
    m_ptbZoom->setText(tr("Zoom"));
    m_ptbZoom->setStatusTip(tr("Zoom"));
    m_ptbZoom->setToolTip(tr("Zoom"));
    m_ptbZoom->setEnabled(false);
    ui->toolBar->insertWidget(ui->actionShow_TabBar_B, m_ptbZoom);

    m_psbZoomFactor = new QSpinBox(ui->toolBar);
    m_psbZoomFactor->setRange(0, 9999999);
    m_psbZoomFactor->setValue(100);
    m_psbZoomFactor->setSuffix("%");
    m_psbZoomFactor->setEnabled(false);
    m_psbZoomFactor->setFocusPolicy(Qt::NoFocus);
    check = connect(m_psbZoomFactor, SIGNAL(valueChanged(int)),
                    this, SLOT(slotZoomFactor(int)));
    Q_ASSERT(check);
    QWidgetAction* pFactor = new QWidgetAction(ui->menuZoom);
    pFactor->setDefaultWidget(m_psbZoomFactor);
    ui->menuZoom->insertAction(ui->actionZoom_Out, pFactor);
    
    m_pGBViewZoom = new QActionGroup(this);
    if(m_pGBViewZoom) {
        m_pGBViewZoom->addAction(ui->actionZoomToWindow_Z);
        m_pGBViewZoom->addAction(ui->actionKeep_aspect_ration_to_windows_K);
        m_pGBViewZoom->addAction(ui->actionOriginal_O);
        m_pGBViewZoom->addAction(ui->actionZoom_In);
        m_pGBViewZoom->addAction(ui->actionZoom_Out);
        m_pGBViewZoom->setEnabled(false);
    }
    ui->actionZoomToWindow_Z->setChecked(true);

    EnableMenu(false);
    
    setFocusPolicy(Qt::NoFocus);

    //TODO: complete the function
    ui->actionZoom_window_to_remote_desktop->setVisible(false);

    check = connect(&m_Parameter, SIGNAL(sigReceiveShortCutChanged()),
                    this, SLOT(slotShortCut()));
    Q_ASSERT(check);
    check = connect(&m_Parameter, SIGNAL(sigSystemTrayIconTypeChanged()),
                    this,
                    SLOT(slotSystemTrayIconTypeChanged()));
    Q_ASSERT(check);
    check = connect(&m_Parameter, SIGNAL(sigShowSystemTrayIcon()),
                    this, SLOT(slotShowSystemTryIcon()));
    Q_ASSERT(check);
    m_Parameter.Load();
    slotShortCut();
#ifdef HAVE_ICE
    if(CICE::Instance()->GetSignal())
    {
        check = connect(CICE::Instance()->GetSignal().data(),
                        SIGNAL(sigConnected()),
                        this, SLOT(slotSignalConnected()));
        Q_ASSERT(check);
        check = connect(CICE::Instance()->GetSignal().data(),
                        SIGNAL(sigDisconnected()),
                        this, SLOT(slotSignalDisconnected()));
        Q_ASSERT(check);
        check = connect(CICE::Instance()->GetSignal().data(),
                        SIGNAL(sigError(const int, const QString&)),
                        this, SLOT(slotSignalError(const int, const QString&)));
        Q_ASSERT(check);
    }
    CICE::Instance()->slotStart();
    m_pSignalStatus = new QPushButton();
    m_pSignalStatus->setToolTip(tr("ICE singal status"));
    m_pSignalStatus->setStatusTip(m_pSignalStatus->toolTip());
    m_pSignalStatus->setWhatsThis(m_pSignalStatus->toolTip());
    slotSignalDisconnected();
    statusBar()->addPermanentWidget(m_pSignalStatus);
#endif

    if(m_Parameter.GetSaveMainWindowStatus())
    {
        QSettings set(RabbitCommon::CDir::Instance()->GetFileUserConfigure(),
                      QSettings::IniFormat);
        QByteArray geometry
                = set.value("MainWindow/Status/Geometry").toByteArray();
        if(!geometry.isEmpty())
            restoreGeometry(geometry);
        QByteArray state = set.value("MainWindow/Status/State").toByteArray();
        if(!state.isEmpty())
            restoreState(state);
        ui->actionToolBar_T->setChecked(!ui->toolBar->isHidden());
    }

    LoadConnectLasterClose();
}

MainWindow::~MainWindow()
{
    qDebug() << "MainWindow::~MainWindow()";
    if(m_pFullScreenToolBar) m_pFullScreenToolBar->close();
    if(m_pGBViewZoom) delete m_pGBViewZoom;
    
    delete ui;
}

void MainWindow::on_actionAbout_A_triggered()
{
#ifdef HAVE_ABOUT
    CDlgAbout *about = new CDlgAbout(this);
    QIcon icon = QIcon::fromTheme("app");
    if(icon.isNull()) return;
    auto sizeList = icon.availableSizes();
    if(sizeList.isEmpty()) return;
    QPixmap p = icon.pixmap(*sizeList.begin());
    about->m_AppIcon = p.toImage();
    about->m_szCopyrightStartTime = "2020";
    if(about->isHidden())
    {
#ifdef BUILD_QUIWidget
        QUIWidget quiwidget;
        quiwidget.setMainWidget(about);
        quiwidget.setPixmap(QUIWidget::Lab_Ico, ":/image/App");
    #if defined (Q_OS_ANDROID)
        quiwidget.showMaximized();
    #endif
        quiwidget.exec();
#else
    #if defined (Q_OS_ANDROID)
        about->showMaximized();
    #endif
        about->exec();
#endif
    }
#endif
}

void MainWindow::on_actionUpdate_U_triggered()
{
#ifdef HAVE_UPDATE
    CFrmUpdater* m_pfrmUpdater = new CFrmUpdater();
    QIcon icon = QIcon::fromTheme("app");
    if(icon.isNull()) return;
    auto sizeList = icon.availableSizes();
    if(sizeList.isEmpty()) return;
    QPixmap p = icon.pixmap(*sizeList.begin());
    m_pfrmUpdater->SetTitle(p.toImage());
    m_pfrmUpdater->SetInstallAutoStartup();
#ifdef BUILD_QUIWidget
    QUIWidget* pQuiwidget = new QUIWidget(nullptr, true);
    pQuiwidget->setMainWidget(m_pfrmUpdater);
#if defined (Q_OS_ANDROID)
    pQuiwidget->showMaximized();
#else
    pQuiwidget->show();
#endif 
#else
#if defined (Q_OS_ANDROID)
    m_pfrmUpdater->showMaximized();
#else
    m_pfrmUpdater->show();
#endif 
#endif
#endif
}

void MainWindow::on_actionOpen_log_configure_file_triggered()
{
    RabbitCommon::CTools::OpenLogConfigureFile();
}

void MainWindow::on_actionLog_directory_triggered()
{
    RabbitCommon::CTools::OpenLogFolder();
}

void MainWindow::on_actionOpen_Log_file_triggered()
{
    RabbitCommon::CTools::OpenLogFile();
}

void MainWindow::on_actionToolBar_T_triggered(bool checked)
{
    qDebug(App) << "MainWindow::on_actionToolBar_T_triggered:" << checked;
    ui->toolBar->setVisible(checked);
}

void MainWindow::on_actionFull_screen_F_triggered()
{
    CView* pTab = dynamic_cast<CView*>(this->centralWidget());
    if(pTab)
    {
        pTab->SetFullScreen(!isFullScreen());
    }
    
    if(isFullScreen())
    {
        ui->actionFull_screen_F->setIcon(QIcon::fromTheme("view-fullscreen"));
        ui->actionFull_screen_F->setText(tr("Full screen(&F)"));
        ui->actionFull_screen_F->setToolTip(tr("Full screen"));
        ui->actionFull_screen_F->setStatusTip(tr("Full screen"));
        ui->actionFull_screen_F->setWhatsThis(tr("Full screen"));

        ui->toolBar->setVisible(true);
        //ui->toolBar->setAllowedAreas(Qt::AllToolBarAreas);
        ui->toolBar->setVisible(ui->actionToolBar_T->isChecked());
        ui->statusbar->setVisible(true);
        ui->menubar->setVisible(true);
        if(m_pFullScreenToolBar)
        {
            m_pFullScreenToolBar->close();
            m_pFullScreenToolBar = nullptr;
        }
        
        emit sigShowNormal();
        this->showNormal();
        this->activateWindow();
        
        return;
    }

    emit sigFullScreen();
    //setWindowFlags(Qt::FramelessWindowHint | windowFlags());
    this->showFullScreen();
   
    ui->actionFull_screen_F->setIcon(QIcon::fromTheme("view-restore"));
    ui->actionFull_screen_F->setText(tr("Exit full screen(&E)"));
    ui->actionFull_screen_F->setToolTip(tr("Exit full screen"));
    ui->actionFull_screen_F->setStatusTip(tr("Exit full screen"));
    ui->actionFull_screen_F->setWhatsThis(tr("Exit full screen"));

    ui->toolBar->setVisible(false);
    //ui->toolBar->setAllowedAreas(Qt::NoToolBarArea);

    ui->statusbar->setVisible(false);
    ui->menubar->setVisible(false);

    if(m_pFullScreenToolBar) m_pFullScreenToolBar->close();
    m_pFullScreenToolBar = new CFrmFullScreenToolBar(this);
    m_pFullScreenToolBar->move((qApp->primaryScreen()->geometry().width()
               - m_pFullScreenToolBar->frameGeometry().width()) / 2, 0);
    bool check = connect(m_pFullScreenToolBar, SIGNAL(sigExitFullScreen()),
                         this, SLOT(on_actionFull_screen_F_triggered()));
    Q_ASSERT(check);
    check = connect(m_pFullScreenToolBar, SIGNAL(sigExit()),
                    this, SLOT(on_actionExit_E_triggered()));
    Q_ASSERT(check);
    check = connect(m_pFullScreenToolBar, SIGNAL(sigDisconnect()), 
                    this, SLOT(on_actionDisconnect_D_triggered()));
    Q_ASSERT(check);
    
    CViewTable* p = dynamic_cast<CViewTable*>(pTab);
    if(p)
    {
        check = connect(m_pFullScreenToolBar, SIGNAL(sigShowTabBar(bool)),
                        SLOT(slotShowTabBar(bool)));
        Q_ASSERT(check);
    }
    m_pFullScreenToolBar->show();
}

void MainWindow::on_actionZoom_window_to_remote_desktop_triggered()
{
    qDebug(App) << "main window:" <<  frameGeometry();
    if(!m_pView) return;
    QSize s = m_pView->GetDesktopSize();
    qDebug(App) << "size:" << s;
    resize(s);
}

void MainWindow::on_actionZoomToWindow_Z_triggered()
{
    if(!m_pView) return;
    m_pView->SetAdaptWindows(CFrmViewer::ZoomToWindow);
    if(m_ptbZoom)
        m_ptbZoom->setIcon(ui->actionZoomToWindow_Z->icon());
}

void MainWindow::on_actionKeep_aspect_ration_to_windows_K_toggled(bool arg1)
{
    if(!arg1) return;
    if(!m_pView) return;
    m_pView->SetAdaptWindows(CFrmViewer::KeepAspectRationToWindow);
    if(m_ptbZoom)
        m_ptbZoom->setIcon(ui->actionKeep_aspect_ration_to_windows_K->icon());
}

void MainWindow::on_actionOriginal_O_changed()
{
    ui->actionZoom_window_to_remote_desktop->setEnabled(ui->actionOriginal_O->isChecked());
}

void MainWindow::on_actionOriginal_O_triggered()
{
    qDebug(App) << "on_actionOriginal_O_triggered()";
    if(!m_pView) return;
    m_pView->SetAdaptWindows(CFrmViewer::Original);
    if(m_psbZoomFactor)
        m_psbZoomFactor->setValue(m_pView->GetZoomFactor() * 100);
    if(m_ptbZoom)
        m_ptbZoom->setIcon(ui->actionOriginal_O->icon());
}

void MainWindow::on_actionZoom_In_triggered()
{
    if(!m_pView) return;
    m_pView->slotZoomIn();
    if(m_psbZoomFactor)
        m_psbZoomFactor->setValue(m_pView->GetZoomFactor() * 100);
    m_pView->SetAdaptWindows(CFrmViewer::Zoom);
    if(m_ptbZoom)
        m_ptbZoom->setIcon(ui->actionZoom_In->icon());
}

void MainWindow::on_actionZoom_Out_triggered()
{
    if(!m_pView) return;
    m_pView->slotZoomOut();
    if(m_psbZoomFactor)
        m_psbZoomFactor->setValue(m_pView->GetZoomFactor() * 100);
    m_pView->SetAdaptWindows(CFrmViewer::Zoom);
    if(m_ptbZoom)
        m_ptbZoom->setIcon(ui->actionZoom_Out->icon());
}

void MainWindow::slotZoomFactor(int v)
{
    m_pView->slotZoomFactor(((double)v) / 100);
    m_pView->SetAdaptWindows(CFrmViewer::Zoom);
}

void MainWindow::slotAdaptWindows(const CFrmViewer::ADAPT_WINDOWS aw)
{
    if(!m_pView)
        return;
    EnableMenu(true);
    switch (aw) {
    case CFrmViewer::Auto:
    case CFrmViewer::Original:
        ui->actionOriginal_O->setChecked(true);
        if(m_ptbZoom)
            m_ptbZoom->setIcon(ui->actionOriginal_O->icon());
        break;
    case CFrmViewer::ZoomToWindow:
        ui->actionZoomToWindow_Z->setChecked(true);
        if(m_ptbZoom)
            m_ptbZoom->setIcon(ui->actionZoomToWindow_Z->icon());
        break;
    case CFrmViewer::KeepAspectRationToWindow:
        ui->actionKeep_aspect_ration_to_windows_K->setChecked(true);
        if(m_ptbZoom)
            m_ptbZoom->setIcon(ui->actionKeep_aspect_ration_to_windows_K->icon());
        break;
    case CFrmViewer::Zoom:
        ui->actionZoom_Out->setChecked(true);
        if(m_ptbZoom)
            m_ptbZoom->setIcon(QIcon::fromTheme("zoom"));
        break;
    case CFrmViewer::Disable:
        if(m_ptbZoom)
            m_ptbZoom->setIcon(QIcon::fromTheme("zoom"));
        EnableMenu(false);
        break;
    default:
        break;
    }
}

void MainWindow::EnableMenu(bool bEnable)
{
    if(m_pGBViewZoom)
        m_pGBViewZoom->setEnabled(bEnable);
    if(m_ptbZoom)
        m_ptbZoom->setEnabled(bEnable);
    if(m_psbZoomFactor)
        m_psbZoomFactor->setEnabled(bEnable);
    ui->actionClone->setEnabled(bEnable);
    ui->actionAdd_to_favorite->setEnabled(bEnable);
    ui->actionCurrent_connect_parameters->setEnabled(bEnable);
    ui->actionScreenshot->setEnabled(bEnable);
    ui->actionDisconnect_D->setEnabled(bEnable);
}

void MainWindow::on_actionExit_E_triggered()
{
    close();
}

void MainWindow::keyReleaseEvent(QKeyEvent *event)
{
    switch(event->key())
    {
    case Qt::Key_Escape:
        if(isFullScreen())
            on_actionFull_screen_F_triggered();
        break;
    }
    QMainWindow::keyReleaseEvent(event);
}

void MainWindow::slotUpdateParameters(CConnecter* pConnecter)
{
    auto it = m_ConfigureFiles.find(pConnecter);
    if(m_ConfigureFiles.end() == it)
        return;
    m_Client.SaveConnecter(it.value(), pConnecter);
}

void MainWindow::on_actionClone_triggered()
{
    if(!m_pView) return;
    QWidget* p = m_pView->GetCurrentView();
    foreach(auto c, m_Connecters)
    {
        if(c->GetViewer() == p)
        {
            auto it = m_ConfigureFiles.find(c);
            if(m_ConfigureFiles.end() == it)
                return;
            QString szFile = it.value();
            auto pConnecter = m_Client.LoadConnecter(szFile);
            if(!pConnecter) return;
            Connect(pConnecter, false, szFile);
            return;
        }
    }
}

void MainWindow::slotOpenFile(const QString& szFile, bool bOpenSettings)
{
    if(szFile.isEmpty()) return;
    CConnecter* p = m_Client.LoadConnecter(szFile);
    if(nullptr == p)
    {
        slotInformation(tr("Load file fail: ") + szFile);
        return;
    }
    
    Connect(p, bOpenSettings, szFile);
}

void MainWindow::on_actionOpenRRCFile_triggered()
{
    QString szFile = RabbitCommon::CDir::GetOpenFileName(this,
                     tr("Open rabbit remote control file"),
                     RabbitCommon::CDir::Instance()->GetDirUserData(), 
                     tr("Rabbit remote control Files (*.rrc);;All files(*.*)"));
    if(szFile.isEmpty()) return;

    CConnecter* p = m_Client.LoadConnecter(szFile);
    if(nullptr == p)
    {
        slotInformation(tr("Load file fail: ") + szFile);
        return;
    }

    Connect(p, true);
}

void MainWindow::on_actionOpen_triggered()
{
    CFrmOpenConnect* p = new CFrmOpenConnect(&m_Client);
    bool check = connect(p, SIGNAL(sigConnect(const QString&, bool)),
                    this, SLOT(slotOpenFile(const QString&, bool)));
    Q_ASSERT(check);
    p->show();
}

void MainWindow::slotConnect()
{
    if(nullptr == m_pView)
    {
        Q_ASSERT(false);
        return;
    }
    QAction* pAction = dynamic_cast<QAction*>(this->sender());    
    CConnecter* p = m_Client.CreateConnecter(pAction->data().toString());
    if(nullptr == p) return;
    
    Connect(p, true);
}

/*!
 * \brief Connect
 * \param p: CConnecter instance pointer
 * \param set: whether open settings dialog.
 *            true: open settings dialog and save configure file
 *            false: don't open settings dialog
 * \param szFile: Configure file.
 *            if is empty. the use default configure file.
 * \return 
 */
int MainWindow::Connect(CConnecter *p, bool set, QString szFile)
{
    bool bSave = false; //whether is save configure file
    Q_ASSERT(p);
    bool check = connect(p, SIGNAL(sigConnected()),
                         this, SLOT(slotConnected()));
    Q_ASSERT(check);
    check = connect(p, SIGNAL(sigDisconnected()),
                             this, SLOT(slotDisconnected()));
    Q_ASSERT(check);
    check = connect(p, SIGNAL(sigError(const int, const QString &)),
                    this, SLOT(slotError(const int, const QString&)));
    Q_ASSERT(check);
    check = connect(p, SIGNAL(sigInformation(const QString&)),
                    this, SLOT(slotInformation(const QString&)));
    Q_ASSERT(check);
    check = connect(p, SIGNAL(sigUpdateName(const QString&)),
                    this, SLOT(slotUpdateName(const QString&)));
    Q_ASSERT(check);
    check = connect(p, SIGNAL(sigUpdateParamters(CConnecter*)),
                         this, SLOT(slotUpdateParameters(CConnecter*)));
    Q_ASSERT(check);
        
    if(set)
    {
        int nRet = p->OpenDialogSettings(this);
        switch(nRet)
        {
        case QDialog::Rejected:
            p->deleteLater();
            return 0;
        case QDialog::Accepted:
            bSave = true;
            break;
        }
    }

    if(szFile.isEmpty())
        szFile = RabbitCommon::CDir::Instance()->GetDirUserData()
                    + QDir::separator()
                    + p->Id()
                    + ".rrc";
    m_ConfigureFiles[p] = szFile;
    
    int nRet = 0;
    if(bSave)
        nRet = m_Client.SaveConnecter(szFile, p);
    if(0 == nRet)
        m_pRecentMenu->addRecentFile(szFile, p->Name());

    if(!p->Name().isEmpty())
        slotInformation(tr("Connecting to ") + p->Name());
    if(m_pView)
    {
        m_pView->SetAdaptWindows(CFrmViewer::Auto, p->GetViewer());
        m_pView->AddView(p->GetViewer());
        m_pView->SetWidowsTitle(p->GetViewer(), p->Name()); 
    }
    
    m_Connecters.push_back(p);
    
    p->Connect();
    
    return 0;
}

void MainWindow::slotConnected()
{
    CConnecter* p = dynamic_cast<CConnecter*>(sender());
    if(!p) return;

//    if(m_pView)
//    {
//        m_pView->AddView(p->GetViewer());
//    }
//    m_Connecters.push_back(p);

    slotInformation(tr("Connected to ") + p->Name());
}

void MainWindow::slotCloseView(const QWidget* pView)
{
    if(!pView) return;
    foreach(auto c, m_Connecters)
    {
        if(c->GetViewer() == pView)
        {
            //TODO: Whether to save the setting
            emit c->sigUpdateParamters(c);
            c->DisConnect();
        }
    }
}

void MainWindow::on_actionDisconnect_D_triggered()
{
    if(!m_pView) return;
    
    QWidget* pView = m_pView->GetCurrentView();
    slotCloseView(pView);    
}

void MainWindow::slotSignalConnected()
{
    m_pSignalStatus->setToolTip(tr("ICE singal status: Connected"));
    m_pSignalStatus->setStatusTip(m_pSignalStatus->toolTip());
    m_pSignalStatus->setWhatsThis(m_pSignalStatus->toolTip());
    //m_pSignalStatus->setText(tr("Connected"));
    m_pSignalStatus->setIcon(QIcon::fromTheme("newwork-wired"));
}

void MainWindow::slotSignalDisconnected()
{
    m_pSignalStatus->setToolTip(tr("ICE singal status: Disconnected"));
    m_pSignalStatus->setStatusTip(m_pSignalStatus->toolTip());
    m_pSignalStatus->setWhatsThis(m_pSignalStatus->toolTip());
    //m_pSignalStatus->setText(tr("Disconnected"));
    m_pSignalStatus->setIcon(QIcon::fromTheme("network-wireless"));
}

void MainWindow::slotSignalError(const int nError, const QString &szInfo)
{
    slotSignalDisconnected();
    slotInformation(szInfo);
}

void MainWindow::slotSignalPushButtonClicked(bool checked)
{
#ifdef HAVE_ICE
    if(checked)
        CICE::Instance()->slotStart();
    else
        CICE::Instance()->slotStop();
#endif
}

void MainWindow::slotDisconnected()
{
    CConnecter* pConnecter = dynamic_cast<CConnecter*>(sender());
    foreach(auto c, m_Connecters)
    {
        if(c == pConnecter)
        {
            m_pView->RemoveView(c->GetViewer());
            m_Connecters.removeAll(c);
            m_ConfigureFiles.remove(c);
            c->deleteLater();
            return;
        }
    }
}

void MainWindow::slotError(const int nError, const QString &szInfo)
{
    Q_UNUSED(nError);
    slotInformation(szInfo);
}

void MainWindow::slotInformation(const QString& szInfo)
{
    statusBar()->showMessage(szInfo);
}

void MainWindow::slotUpdateName(const QString& szName)
{
    CConnecter* pConnecter = dynamic_cast<CConnecter*>(sender());
    if(!pConnecter) return;
    m_pView->SetWidowsTitle(pConnecter->GetViewer(), szName);
}

void MainWindow::on_actionStyle_S_triggered()
{
    CFrmStyle* s = new CFrmStyle();
    s->show();
}

int MainWindow::onProcess(const QString &id, CPluginClient *pPlug)
{
    Q_UNUSED(id);
    // Connect menu and toolbar
    QAction* p = ui->menuConnect_C->addAction(pPlug->Protocol()
                                              + ": " + pPlug->DisplayName(),
                                              this, SLOT(slotConnect()));
    p->setToolTip(pPlug->Description());
    p->setStatusTip(pPlug->Description());
    p->setData(id);
    p->setIcon(pPlug->Icon());

    return 0;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    Q_UNUSED(event);
    qDebug(App) << "MainWindow::closeEvent()";
    
    if(m_Parameter.GetSaveMainWindowStatus())
        if(isFullScreen())
            on_actionFull_screen_F_triggered();
    
    SaveConnectLasterClose();
    
    foreach (auto it, m_Connecters)
    {
        //TODO: Whether to save the setting
        emit it->sigUpdateParamters(it);
        it->DisConnect();
    }
    
    QSettings set(RabbitCommon::CDir::Instance()->GetFileUserConfigure(),
                  QSettings::IniFormat);    
    if(m_Parameter.GetSaveMainWindowStatus())
    {
        set.setValue("MainWindow/Status/Geometry", saveGeometry());
        set.setValue("MainWindow/Status/State", saveState());
    } else {
        set.remove("MainWindow/Status/Geometry");
        set.remove("MainWindow/Status/State");
    }
    QMainWindow::closeEvent(event);
}

int MainWindow::LoadConnectLasterClose()
{
    if(!m_Parameter.GetOpenLasterClose())
        return 0;
    
    QFile f(RabbitCommon::CDir::Instance()->GetDirUserConfig()
            + QDir::separator() + "LasterClose.dat");
    if(f.open(QFile::ReadOnly))
    {
        QDataStream d(&f);
        while(1){
            QString szFile;
            d >> szFile;
            if(szFile.isEmpty())
                break;
            slotOpenFile(szFile);
        }
        f.close();
    }
    return 0;
}

int MainWindow::SaveConnectLasterClose()
{
    QFile f(RabbitCommon::CDir::Instance()->GetDirUserConfig()
                  + QDir::separator() + "LasterClose.dat");
    f.open(QFile::WriteOnly);
    if(m_Parameter.GetOpenLasterClose())
    {
        QDataStream d(&f);
        foreach(auto it, m_ConfigureFiles)
        {
            d << it;
        }
    }
    f.close();
    return 0;
}

void MainWindow::on_actionSend_ctl_alt_del_triggered()
{
    if(m_pView)
        m_pView->slotSystemCombination();
}

void MainWindow::on_actionShow_TabBar_B_triggered()
{
    slotShowTabBar(ui->actionShow_TabBar_B->isChecked());
}

void MainWindow::slotShowTabBar(bool bShow)
{
    CViewTable* p = dynamic_cast<CViewTable*>(m_pView);
    if(p)
        p->ShowTabBar(bShow);
}

void MainWindow::on_actionScreenshot_triggered()
{
    if(!m_pView || !m_pView->GetCurrentView()) return;
    QString szFile = m_Parameter.GetScreenShotPath()
            + QDir::separator()
            + QDateTime::currentDateTime().toLocalTime().toString("yyyy_MM_dd_hh_mm_ss_zzz")
            + ".jpg";
    if(szFile.isEmpty()) return;
    int nRet = m_pView->Screenslot(szFile, m_Parameter.GetScreenShot());
    if(0 == nRet)
    {
        slotInformation(tr("Save screenslot to ") + szFile);
        switch (m_Parameter.GetScreenShotEndAction()) {
        case CParameterApp::OpenFile:
            QDesktopServices::openUrl(QUrl::fromLocalFile(szFile));
            break;
        case CParameterApp::OpenFolder:
        {
            QFileInfo f(szFile);
            QString path = f.absolutePath();
            QUrl url = QUrl::fromLocalFile(path);
            QDesktopServices::openUrl(url);
            break;
        }
        default:
            break;
        }
    }
    return ;
}

void MainWindow::on_actionSettings_triggered()
{
    CParameterDlgSettings set(&m_Parameter, m_Client.GetSettingsWidgets(this), this);
    if(CParameterDlgSettings::Accepted == set.exec())
    {
        m_Client.SaveSettings();
        m_Parameter.Save();
    }
}

void MainWindow::slotShortCut()
{
    if(m_Parameter.GetReceiveShortCut())
    {
        setFocusPolicy(Qt::WheelFocus);
        if(m_psbZoomFactor)
            m_psbZoomFactor->setFocusPolicy(Qt::WheelFocus);
        if(m_ptbZoom)
            m_ptbZoom->setFocusPolicy(Qt::WheelFocus);
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        ui->actionFull_screen_F->setShortcut(
                    QKeySequence(QKeyCombination(Qt::CTRL, Qt::Key_R),
                                 QKeyCombination(Qt::Key_F)));
        ui->actionScreenshot->setShortcut(
                    QKeySequence(QKeyCombination(Qt::CTRL, Qt::Key_R),
                                 QKeyCombination(Qt::Key_S)));
        ui->actionZoom_In->setShortcut(
                    QKeySequence(QKeyCombination(Qt::CTRL, Qt::Key_R),
                                 QKeyCombination(Qt::Key_Plus)));
        ui->actionZoom_Out->setShortcut(
                    QKeySequence(QKeyCombination(Qt::CTRL, Qt::Key_R),
                                 QKeyCombination(Qt::Key_Minus)));
        ui->actionOriginal_O->setShortcut(
                    QKeySequence(QKeyCombination(Qt::CTRL, Qt::Key_R),
                                 QKeyCombination(Qt::Key_O)));
        ui->actionZoomToWindow_Z->setShortcut(
                    QKeySequence(QKeyCombination(Qt::CTRL, Qt::Key_R),
                                 QKeyCombination(Qt::Key_W)));
#else
        ui->actionFull_screen_F->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R, Qt::Key_F));
        ui->actionScreenshot->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R, Qt::Key_S));
        ui->actionZoom_In->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R, Qt::Key_Plus));
        ui->actionZoom_Out->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R, Qt::Key_Minus));
        ui->actionOriginal_O->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R, Qt::Key_O));
        ui->actionZoomToWindow_Z->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_R, Qt::Key_W));
#endif
    } else {
        setFocusPolicy(Qt::NoFocus);
        if(m_psbZoomFactor)
            m_psbZoomFactor->setFocusPolicy(Qt::NoFocus);
        if(m_ptbZoom)
            m_ptbZoom->setFocusPolicy(Qt::NoFocus);
        ui->actionFull_screen_F->setShortcut(QKeySequence());
        ui->actionScreenshot->setShortcut(QKeySequence());
        ui->actionZoom_In->setShortcut(QKeySequence());
        ui->actionZoom_Out->setShortcut(QKeySequence());
        ui->actionOriginal_O->setShortcut(QKeySequence());
        ui->actionZoomToWindow_Z->setShortcut(QKeySequence());
    }
}

void MainWindow::on_actionCurrent_connect_parameters_triggered()
{
    if(!m_pView) return;
    QWidget* p = m_pView->GetCurrentView();
    foreach(auto c, m_Connecters)
    {
        if(c->GetViewer() == p)
        {
            int nRet = c->OpenDialogSettings(this);
            if(QDialog::Accepted == nRet)
            {
                emit c->sigUpdateParamters(c);
                return;
            }
        }
    }
}

void MainWindow::on_actionAdd_to_favorite_triggered()
{
    if(!m_pView || !m_pFavoriteView) return;
    QWidget* p = m_pView->GetCurrentView();
    foreach(auto c, m_Connecters)
    {
        if(c->GetViewer() == p)
        {
            auto it = m_ConfigureFiles.find(c);
            if(m_ConfigureFiles.end() == it)
                return;
            m_pFavoriteView->AddFavorite(c->Name(), it.value());
        }
    }
}

void MainWindow::on_actionFavorites_triggered()
{
    if(!m_pFavoriteDockWidget)
        return;
    
    if(ui->actionFavorites->isChecked())
        m_pFavoriteDockWidget->show();
    else
        m_pFavoriteDockWidget->hide();
}

void MainWindow::slotDockWidgetFavoriteVisibilityChanged(bool visib)
{
    ui->actionFavorites->setChecked(visib);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    qDebug(App) << "dragEnterEvent";
    
    if(event->mimeData()->hasUrls())
    {
        qWarning(App) << event->mimeData()->urls();
        event->acceptProposedAction();
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    //qDebug(App) << "dragMoveEvent";
}

void MainWindow::dropEvent(QDropEvent *event)
{
    qDebug(App) << "dropEvent";
    if(!event->mimeData()->hasUrls())
        return;
    auto urls = event->mimeData()->urls();
    foreach(auto url, urls)
    {
        if(url.isLocalFile())
            slotOpenFile(url.toLocalFile());
    }
}

void MainWindow::slotSystemTrayIconActivated(QSystemTrayIcon::ActivationReason reason)
{
    Q_UNUSED(reason)
#if defined(Q_OS_ANDROID)
    showMaximized();
#else
    switch(reason)
    {
    case QSystemTrayIcon::Trigger:
    {
        showNormal();
        activateWindow();
        break;
    }
    default:
        break;
    }
#endif
}

void MainWindow::slotSystemTrayIconTypeChanged()
{
    if(!m_Parameter.GetEnableSystemTrayIcon())
    {
        m_TrayIcon.reset();
        return;
    }

    m_TrayIcon = QSharedPointer<QSystemTrayIcon>(new QSystemTrayIcon(this));
    if(QSystemTrayIcon::isSystemTrayAvailable())
    {
        bool check = connect(m_TrayIcon.data(),
                        SIGNAL(activated(QSystemTrayIcon::ActivationReason)),
                        this,
          SLOT(slotSystemTrayIconActivated(QSystemTrayIcon::ActivationReason)));
        Q_ASSERT(check);
        m_TrayIcon->setIcon(this->windowIcon());
        m_TrayIcon->setToolTip(windowTitle());
        m_TrayIcon->show();
    } else
        qWarning(App) << "System tray is not available";
    
    switch (m_Parameter.GetSystemTrayIconMenuType())
    {
    case CParameterApp::SystemTrayIconMenuType::Remote:
        m_TrayIcon->setContextMenu(ui->menuRemote);
        break;
    case CParameterApp::SystemTrayIconMenuType::RecentOpen:
        m_TrayIcon->setContextMenu(m_pRecentMenu);
        break;
    case CParameterApp::SystemTrayIconMenuType::Favorite:
    case CParameterApp::SystemTrayIconMenuType::No:
        m_TrayIcon->hide();
        break;
    }   
}

void MainWindow::slotShowSystemTryIcon()
{
    slotSystemTrayIconTypeChanged();
}
