/*** Includes ******************************************************************/
#if defined(APT_DEBUG)
#include "Apt.h"
#include <EABase/eabase.h>
#include "_AptValue.h"
#include "_AptActions.h"
#include "AptPubDebugCallStack.h"

namespace EA
{
namespace Apt
{
namespace Debug
{

/*** Macros *******************************************************************/

/*** Interface ****************************************************************/

/*** Variables ****************************************************************/

/*** Implementation ***********************************************************/

/******************************************************************************/
/** AptPubDebugCallStackC:: _GetFunctionName

@brief  Gets the function name from the aptdebug call stack at the index
specified.

@param          - none.
@return         - The function name.  NULL for a null string AND for
cases where the index is invalid.

[Since these functions should only be used in non-ship builds
I sided on erorr checking more than speed]
*/
/******************************************************************************/
const char *AptPubDebugCallStackC::_GetFunctionName(uint32_t Index)
{
#if defined(APT_DEBUG)
    uint32_t size;

    size = gAptActionInterpreter.debugCallStack.GetSize();
    if (Index < size)
    {
        return gAptActionInterpreter.debugCallStack.At(Index)->sFunctionName.c_str();
    }
    else
    {
        APT_ASSERT(0);
    }
#endif
    return NULL;
}

/******************************************************************************/
/** AptPubDebugCallStackC:: _GetNumItems

@brief  Gets the number of items on the debugCallStack

@param          - none.
@return         - Number of items in the stack. 0 if none or
gAptActionInterpreter is NULL.

[Since these functions should only be used in non-ship builds
I sided on erorr checking more than speed]
*/
/******************************************************************************/
uint32_t AptPubDebugCallStackC::_GetNumItems()
{
#if defined(APT_DEBUG)
    return gAptActionInterpreter.debugCallStack.GetSize();
#else
    return 0;
#endif
}
} // namespace Debug
} // namespace Apt
} // namespace EA

#endif // defined (APT_DEBUG)
