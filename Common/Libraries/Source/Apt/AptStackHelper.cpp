/*** Include files ********************************************************************************/
#include "_Apt.h"
#include "AptStackHelper.h"

#if defined(APT_ENABLE_INLINE)
#include "AptActionInterpreter.inl"
#endif

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Function Prototypes **************************************************************************/

/*** Variables ************************************************************************************/

// Private variables

// Public variables

/*** Private Functions ****************************************************************************/

/*** Public Functions *****************************************************************************/

/******************************************************************************/
/**
    @brief  Get the AptValue at a given index on the AptActionInterpreter stack

    @param  index   Index on the stack

    @return         - AptValue at that index

*/
/******************************************************************************/
AptValue *AptStackHelper::GetStackAt(const int index)
{
    return gAptActionInterpreter.stackAt(index);
}