#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "_Apt.h"
#include "AptValueFactory.h"
#include "MainInline.h"

AptValue *AptValueFactory::CreateString(const char *szValue)
{
    APT_ASSERT(szValue != NULL);

    AptUpdateAutoLock lock;

    AptValue *pValue = AptString::Create(szValue);

    return pValue;
}

AptValue *AptValueFactory::CreateInteger(int nValue)
{
    AptUpdateAutoLock lock;

    AptInteger *pValue = AptInteger::Create(nValue);

    return pValue;
}

AptValue *AptValueFactory::CreateFloat(float fValue)
{
    AptUpdateAutoLock lock;

    AptValue *pValue = (AptValue *)AptFloat::Create(fValue);

    return pValue;
}

AptValue *AptValueFactory::CreateBoolean(bool bValue)
{
    AptUpdateAutoLock lock; // don't really need a lock here in boolean..

    AptValue *pValue = AptBoolean::Create(bValue);

    return pValue;
}

AptValue *AptValueFactory::CreateArray(int nElements, AptValue **pAptValue)
{
    AptUpdateAutoLock lock;

    if (nElements)
    {
        APT_ASSERT(pAptValue != NULL);
    }

    AptValue *pValue = new AptArray(nElements, pAptValue);

    return pValue;
}

/******************************************************************************/
/**
    @brief  Create and return an empty array

    @return         - A new AptArray containing nothing
*/
/******************************************************************************/
AptValue *AptValueFactory::CreateArray()
{
    AptUpdateAutoLock lock;
    return new AptArray();
}

/******************************************************************************/
/**
    @brief  Create and return an empty object

    @return         - A new object with no properties
*/
/******************************************************************************/
AptValue *AptValueFactory::CreateObject()
{
    AptUpdateAutoLock lock;
    return (new AptObject(AptVFT_Object));
}

AptValue *AptValueFactory::CreateStringFormatted(const char *szFormat, ...)
{
    APT_ASSERT(szFormat != NULL);

    AptUpdateAutoLock lock;

    AptString *pValue = AptString::Create("");

    va_list Args;

    va_start(Args, szFormat); //  Initialize variable arguments.

    AptNativeString *ans = pValue->GetInternalString();
    ans->vsFormat(szFormat, Args);

    va_end(Args);
    return pValue;
}

AptValue *AptValueFactory::CreateUndefined()
{
    return gpUndefinedValue;
}

/*H*************************************************************************************************
    EOF
*************************************************************************************************H*/
