/*** Include files ********************************************************************************/
#include "AptDebugHelper.h"
#include "_AptThread.h"
#include APT_INC_THREAD_H

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/

// Private variables

// Public variables
AptDebugHelper *AptDebugHelper::sInstance = NULL;

/*** Private Functions ****************************************************************************/

/*** Public Functions *****************************************************************************/

/******************************************************************************/
/**
    AptDebugHelper::Initialize
    @brief  Initialize AptDebugHelper. Singleton instantiation function

    @return Return true if success. Otherwise return false
*/
/******************************************************************************/
bool AptDebugHelper::Initialize()
{
    APT_ASSERT(NULL == sInstance);

    if (NULL == sInstance)
    {
        sInstance = new AptDebugHelper();
    }

    return true;
}

/******************************************************************************/
/**
    AptDebugHelper::Shutdown
    @brief  Shutdown AptDebugHelper. Singleton destruction function

    @return Return true if success. Otherwise return false
*/
/******************************************************************************/
bool AptDebugHelper::Shutdown()
{
    if (NULL != sInstance)
    {
        delete sInstance;
        sInstance = NULL;
    }

    return true;
}

/******************************************************************************/
/**
    AptDebugHelper::GetInstance
    @brief  AptDebugHelper singleton instance function

    @return Return if exists, otherwise create then return.
*/
/******************************************************************************/
AptDebugHelper *AptDebugHelper::GetInstance()
{
    if (NULL == sInstance)
    {
        sInstance = new AptDebugHelper();
    }

    return sInstance;
}

/*** AptDebugHelper    ***********************************************************/

/******************************************************************************/
/**
    AptDebugHelper::AptDebugHelper
    @brief  AptDebugHelper constructor
*/
/******************************************************************************/
AptDebugHelper::AptDebugHelper() : mPushBinaryCb(NULL)
{
}

/******************************************************************************/
/**
    AptDebugHelper::AptDebugHelper
    @brief  AptDebugHelper destructor
*/
/******************************************************************************/
AptDebugHelper::~AptDebugHelper()
{
}

void AptDebugHelper::AddTrace(const AptNativeString &aptTraceStr)
{
    if (!mPushBinaryCb)
        return;

    uint8_t *binaryData   = (uint8_t *)aptTraceStr.ConstRawPtr();
    size_t binaryDataSize = (size_t)aptTraceStr.GetLength();
    mPushBinaryCb(binaryData, binaryDataSize);
    uint8_t *delimiter   = (uint8_t *)"\n";
    size_t delimiterSize = 1;
    mPushBinaryCb(delimiter, delimiterSize);
}
