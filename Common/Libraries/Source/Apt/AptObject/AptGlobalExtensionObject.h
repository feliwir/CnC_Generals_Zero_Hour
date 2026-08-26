#pragma once
/*** Include files ********************************************************************************/

#include "AptValue/AptValue.h"
#include "_Apt.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/
// AptGlobalExtensionObject now derives from AptObject instead of AptValueGlobalWithHash

class AptGlobalExtensionObject : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptGlobalExtensionObject() : AptObject(AptVFT_GlobalExtension, APT_OBJECTHASHSIZE)
    {
        setGCRoot(1);
    }

    APT_INLINE void Set(const AptNativeString *const pKey, AptValue *const pValue);
    APT_INLINE void UnSet(const AptNativeString *const pKey);
    APT_INLINE AptValue *Lookup(const AptNativeString *const pKey);

  protected:
    APT_INLINE
    virtual ~AptGlobalExtensionObject()
    {
    }
};

/*** Variables ************************************************************************************/


/*** Functions ************************************************************************************/
