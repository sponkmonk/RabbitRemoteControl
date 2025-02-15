// Author: Kang Lin <kl222@126.com>

#include "ParameterDlgSettings.h"
#include "ui_ParameterDlgSettings.h"
#include "mainwindow.h"
#include "RabbitCommonDir.h"
#include "RabbitCommonStyle.h"
#ifdef HAVE_ICE
    #include "Ice.h"
#endif

#include <QLoggingCategory>
Q_DECLARE_LOGGING_CATEGORY(App)

CParameterDlgSettings::CParameterDlgSettings(CParameterApp *pPara,
        QList<QWidget *> wViewer,
        QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CParameterDlgSettings),
    m_pParameters(pPara)
{
    ui->setupUi(this);

    foreach(auto p, wViewer)
    {
        ui->tabWidget->addTab(p, p->windowTitle());
        bool check = false;
        check = connect(this, SIGNAL(accepted()), p, SLOT(slotAccept()));
        if(!check)
        {
            qCritical(App) << "Class" << p->metaObject()->className()
                            << "must has slot slotAccept(), please add it";
                            
        }
        Q_ASSERT(check);
    }

#ifdef HAVE_ICE
    QWidget* pWidget = CICE::Instance()->GetParameterWidget(this);
    if(pWidget)
    {
        ui->tabWidget->addTab(pWidget, pWidget->windowTitle());
        bool check = false;
        check = connect(this, SIGNAL(accepted()),
                        pWidget, SLOT(slotAccept()));
        if(!check)
        {
            qCritical(App) << "Class" << pWidget->metaObject()->className()
                            << "must has slot slotAccept(), please add it";
        }
        Q_ASSERT(check);
    }
#endif
    
    ui->leShotScreenSavepath->setText(m_pParameters->GetScreenShotPath());
    ui->cbShotRemoteDesktop->setChecked(m_pParameters->GetScreenShot());
    switch (m_pParameters->GetScreenShotEndAction()) {
    case CParameterApp::NoAction:
        ui->rbShotScreenNoAction->setChecked(true);
        break;
    case CParameterApp::OpenFile:
        ui->rbShotScreenOpenFile->setChecked(true);
        break;
    case CParameterApp::OpenFolder:
        ui->rbShotScreenOpenFolder->setChecked(true);
        break;    
    }
    
    ui->cbReciverShortCut->setChecked(m_pParameters->GetReceiveShortCut());
    ui->cbSaveMainWindowStatus->setChecked(m_pParameters->GetSaveMainWindowStatus());
    ui->cbResumLasterClose->setChecked(m_pParameters->GetOpenLasterClose());
    
    switch (m_pParameters->GetTabPosition()) {
    case QTabWidget::North:
        ui->rbNorth->setChecked(true);
        break;
    case QTabWidget::South:
        ui->rbSouth->setChecked(true);
        break;
    case QTabWidget::West:
        ui->rbWest->setChecked(true);
        break;
    case QTabWidget::East:
        ui->rbEast->setChecked(true);
        break;
    }
    
    ui->sbRecentMenuMaxCount->setValue(m_pParameters->GetRecentMenuMaxCount());
    
    ui->rbSystemTrayIconFavorite->hide();
    switch (m_pParameters->GetSystemTrayIconMenuType()) {
    case CParameterApp::SystemTrayIconMenuType::No:
        ui->rbSystemTrayIconNo->setChecked(true);
        break;
    case CParameterApp::SystemTrayIconMenuType::RecentOpen:
        ui->rbSystemTrayIconRecentOpen->setChecked(true);
        break;
    case CParameterApp::SystemTrayIconMenuType::Remote:
        ui->rbSystemTrayIconRemote->setChecked(true);
        break;
    case CParameterApp::SystemTrayIconMenuType::Favorite:
        ui->rbSystemTrayIconFavorite->setChecked(true);
        break;
    }
    ui->cbShowSystemTrayIcon->setChecked(m_pParameters->GetEnableSystemTrayIcon());
    
    ui->cbFavoriteDoubleEdit->setChecked(m_pParameters->GetFavoriteEdit());

}

CParameterDlgSettings::~CParameterDlgSettings()
{
    delete ui;
}

void CParameterDlgSettings::on_pbOk_clicked()
{
    if(!m_pParameters) return;
    
    m_pParameters->SetScreenShot(ui->cbShotRemoteDesktop->isChecked());
    m_pParameters->SetScreenShotPath(ui->leShotScreenSavepath->text());
    if(ui->rbShotScreenNoAction->isChecked())
        m_pParameters->SetScreenShotEndAction(CParameterApp::NoAction);
    if(ui->rbShotScreenOpenFile->isChecked())
        m_pParameters->SetScreenShotEndAction(CParameterApp::OpenFile);
    if(ui->rbShotScreenOpenFolder->isChecked())
        m_pParameters->SetScreenShotEndAction(CParameterApp::OpenFolder);
    
    m_pParameters->SetReceiveShortCut(ui->cbReciverShortCut->isChecked());
    m_pParameters->SetSaveMainWindowStatus(ui->cbSaveMainWindowStatus->isChecked());
    m_pParameters->SetOpenLasterClose(ui->cbResumLasterClose->isChecked());
    
    if(ui->rbNorth->isChecked()) m_pParameters->SetTabPosition(QTabWidget::North);
    if(ui->rbSouth->isChecked()) m_pParameters->SetTabPosition(QTabWidget::South);
    if(ui->rbEast->isChecked()) m_pParameters->SetTabPosition(QTabWidget::East);
    if(ui->rbWest->isChecked()) m_pParameters->SetTabPosition(QTabWidget::West);

    m_pParameters->SetRecentMenuMaxCount(ui->sbRecentMenuMaxCount->value());
    
    if(ui->rbSystemTrayIconNo->isChecked())
        m_pParameters->SetSystemTrayIconMenuType(CParameterApp::SystemTrayIconMenuType::No);
    if(ui->rbSystemTrayIconRecentOpen->isChecked())
        m_pParameters->SetSystemTrayIconMenuType(CParameterApp::SystemTrayIconMenuType::RecentOpen);
    if(ui->rbSystemTrayIconRemote->isChecked())
        m_pParameters->SetSystemTrayIconMenuType(CParameterApp::SystemTrayIconMenuType::Remote);
    if(ui->rbSystemTrayIconFavorite->isChecked())
        m_pParameters->SetSystemTrayIconMenuType(CParameterApp::SystemTrayIconMenuType::Favorite);
    
    m_pParameters->SetEnableSystemTrayIcon(ui->cbShowSystemTrayIcon->isChecked());
    m_pParameters->SetFavoriteEdit(ui->cbFavoriteDoubleEdit->isChecked());

    accept();
}

void CParameterDlgSettings::on_pbNo_clicked()
{
    reject();
}

void CParameterDlgSettings::on_pbScreenShotBrower_clicked()
{
    QString szPath = RabbitCommon::CDir::Instance()->GetOpenDirectory(this,
                                                    tr("Open shot screen path"),
                                              ui->leShotScreenSavepath->text());
    if(szPath.isEmpty())
        return;
    ui->leShotScreenSavepath->setText(szPath);
}

void CParameterDlgSettings::on_cbShowSystemTrayIcon_toggled(bool checked)
{
    ui->gbSystemTrayIconContextMenu->setEnabled(checked);
}
