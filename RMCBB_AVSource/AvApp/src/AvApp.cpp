#include "AvApp.h"
#if defined(_AV_PRODUCT_CLASS_IPC_) || defined(_AV_PLATFORM_HI3520A_) || defined(_AV_PLATFORM_HI3521_) || defined(_AV_PLATFORM_HI3531_) || defined(_AV_PLATFORM_HI3532_) || defined(_AV_PLATFORM_HI3520D_) || defined(_AV_PLATFORM_HI3535_) ||defined(_AV_PLATFORM_HI3520D_V300_)
#include "HiAvDevice.h"
#endif


CAvApp::CAvApp()
{
    m_pAv_device = NULL;
}


CAvApp::~CAvApp()
{
    _AV_SAFE_DELETE_(m_pAv_device);
}


int CAvApp::Aa_init()
{
    _AV_ASSERT_(m_pAv_device != NULL, "You must define m_pAv_device\n");

    if(m_pAv_device->Ad_init() != 0)
    {
        DEBUG_ERROR( "CAvApp::Aa_init (m_pAv_device->Ad_init() != 0)\n");
        _AV_SAFE_DELETE_(m_pAv_device);
        return -1;
    }

    return 0;
}


int CAvApp::Aa_wait_system_ready()
{
    _AV_ASSERT_(m_pAv_device != NULL, "You must define m_pAv_device\n");

    if(m_pAv_device->Ad_wait_system_ready() != 0)
    {
        DEBUG_ERROR( "CAvApp::Aa_wait_system_ready (m_pAv_device->Ad_wait_system_ready() != 0)\n");
        return -1;
    }

    return 0;
}

int CAvApp::Aa_construct_communication_component()
{
    return 0;
}


