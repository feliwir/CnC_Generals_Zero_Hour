/*** Include files ********************************************************************************/
#include "_Apt.h"
#include "AptFastStack.h"
#include "MainInline.h" // added for psp-release build

#if defined(APT_PLATFORM_PLAYSTATION2) // implementation is PS2 only
#include <eekernel.h>
/*** Defines **************************************************************************************/

#define STACK_SPRADDRESS 0x70000000

/*** Macros ***************************************************************************************/

#define STACK_EI() __asm__ volatile("ei")
#define STACK_DI()                                           \
    {                                                        \
        u_int stat;                                          \
        do                                                   \
        {                                                    \
            __asm__ volatile(".p2align 3");                  \
            __asm__ volatile("di");                          \
            __asm__ volatile("sync.p");                      \
            __asm__ volatile("mfc0 %0, $12" : "=r"(stat) :); \
        } while (stat & 0x00010000);                         \
    }

/*** Type Definitions *****************************************************************************/

/*** Variables ************************************************************************************/

// Private variables
Uint128 *AptFastStack::m_pStackMem;
uint32_t AptFastStack::m_uStackFrameSize;

// Public variables

/*** Private Functions ****************************************************************************/

// Returns value of SP
static inline Uint128 *_StackGetSP()
{
    register Uint128 *SP;
    asm __volatile__("add  %0,$0,$29" : "=r"(SP));
    return SP;
}

// Sets SP to passed value
static inline void _StackSetSP(Uint128 *SP)
{
    asm __volatile__("add  $29,$0,%0" : : "r"(SP));
}

/*** Public functions *****************************************************************************/

void AptFastStack::Begin(uint32_t FrameSize, uint32_t SprAddr)
{
    Uint128 *pMemStack;
    Uint128 *pSprStack;

    if (m_pStackMem == NULL)
    {
        // disable interrupts
        STACK_DI();

        pMemStack = _StackGetSP();

        // get pointer to new scratchpad stack address
        pSprStack = (Uint128 *)(STACK_SPRADDRESS + SprAddr);

        // allocate stack frame
        pSprStack -= FrameSize;

        _StackSetSP(pSprStack);

        // store info about memory stack for later
        m_uStackFrameSize = FrameSize;
        m_pStackMem       = pMemStack;

        while (FrameSize > 0)
        {
            // copy stack frame
            *pSprStack = *pMemStack;

            pMemStack++;
            pSprStack++;
            FrameSize--;
        }

        // enable interrupts
        STACK_EI();
    }
}

void AptFastStack::End()
{
    Uint128 *pMemStack;
    Uint128 *pSprStack;
    uint32_t FrameSize;

    if (m_pStackMem)
    {
        // disable interrupts
        STACK_DI();

        pSprStack = _StackGetSP();
        pMemStack = m_pStackMem;
        FrameSize = m_uStackFrameSize;

        // set stack pointer to ram
        _StackSetSP(pMemStack);

        // copy stack frame
        while (FrameSize > 0)
        {
            *pMemStack = *pSprStack;

            pMemStack++;
            pSprStack++;
            FrameSize--;
        }

        m_pStackMem       = (Uint128 *)NULL;
        m_uStackFrameSize = 0;

        // enable interrupts
        STACK_EI();
    }
}

#endif
