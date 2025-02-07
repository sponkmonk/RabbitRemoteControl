// Author: Kang Lin <kl222@126.com>

#ifndef CCONNECTERFREERDP_H
#define CCONNECTERFREERDP_H

#include "Connecter.h"
#include "ConnecterDesktopThread.h"
#include "freerdp/freerdp.h"
#include "ParameterFreeRDP.h"

class CConnecterFreeRDP : public CConnecterDesktopThread
{
    Q_OBJECT
public:
    explicit CConnecterFreeRDP(CPluginClient *parent = nullptr);
    virtual ~CConnecterFreeRDP() override;

public:
    virtual qint16 Version() override;
    
protected:
    virtual QDialog *GetDialogSettings(QWidget *parent) override;

    virtual CConnect *InstanceConnect() override;

private:
    CParameterFreeRDP m_ParameterFreeRdp;
};

#endif // CCONNECTERFREERDP_H
