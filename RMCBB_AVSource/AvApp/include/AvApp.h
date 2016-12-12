/********************************************************
    Copyright (C), 2007-2099,SZ-STREAMING Tech. Co., Ltd.
    File name:
    Author:
    Date:
    Description:
********************************************************/
#ifndef __AVAPP_H__
#define __AVAPP_H__
#include "AvConfig.h"
#include "AvCommonMacro.h"
#include "AvDeviceInfo.h"
#include "AvDevice.h"


class CAvApp{
    public:
       CAvApp();
       virtual ~CAvApp();

    public:
        virtual int Aa_load_system_config(int argc = 0, char *argv[] = NULL) = 0;
        virtual int Aa_construct_communication_component();
        virtual int Aa_init();
        virtual int Aa_wait_system_ready();

    protected:
        CAvDevice *m_pAv_device;
};


#endif/*__AVAPP_H__*/

