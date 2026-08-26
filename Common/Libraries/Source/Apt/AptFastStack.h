#pragma once

/*** Include files ********************************************************************************/

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

#if defined(APT_PLATFORM_PLAYSTATION2)

typedef signed int Int128 __attribute__((mode(TI)));
typedef unsigned int Uint128 __attribute__((mode(TI)));

class AptFastStack
{
  public:
    static void Begin(uint32_t FrameSize, uint32_t SprAddr);
    static void End();
    static inline bool IsActive() { return m_pStackMem ? 1 : 0; }

  private:
    static Uint128 *m_pStackMem;
    static uint32_t m_uStackFrameSize;
};
#else
class AptFastStack
{
  public:
    static inline void Begin(uint32_t FrameSize, uint32_t SprAddr) {}
    static inline void End() {}
    static inline bool IsActive() { return 0; }
};
#endif

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

