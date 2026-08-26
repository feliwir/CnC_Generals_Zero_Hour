/*** Include files ********************************************************************************/
#include "AptAllocator.h"
#include "Apt.h"
#include "AptDefine.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Functions ************************************************************************************/

AptStdAllocator::AptStdAllocator()
    : m_name("AptStdAllocator")
{
}

AptStdAllocator::AptStdAllocator(const char *name)
    : m_name(name)
{
}

void *AptStdAllocator::allocate(size_t n, int flags)
{
    return allocate(n, sizeof(void *), 0, flags);
}

void *AptStdAllocator::allocate(size_t n, size_t alignment, size_t offset, int flags)
{
    return AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(n));
}

void AptStdAllocator::deallocate(void *p, size_t n)
{
    AptGetUserFuncs().pfnMemFree(p);
}

const char *AptStdAllocator::get_name() const
{
    return m_name;
}

void AptStdAllocator::set_name(const char *pName)
{
    m_name = pName;
}
