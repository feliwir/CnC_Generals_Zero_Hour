#pragma once

/*** Include files ********************************************************************************/
#include "Apt.h"
#include "AptString/EAString.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

// Forward Declarations

class AptDebugHelper
{
  public:
    using pushBinaryCbFunc = void (*)(uint8_t *dataValue, size_t dataSize);

    // Singleton instance function
    static bool Initialize();
    static bool Shutdown();
    static AptDebugHelper *GetInstance();

    void AddTrace(const AptNativeString &aptTraceStr);
    void SetPushBinaryCB(pushBinaryCbFunc pushBinaryCB) { mPushBinaryCb = pushBinaryCB; }

  private:
    AptDebugHelper();  //!< Constructor
    ~AptDebugHelper(); //!< Destructor

    static AptDebugHelper *sInstance; //!< Singleton instance

    pushBinaryCbFunc mPushBinaryCb;

}; // end of AptDebugHelper
