#pragma once

/*** Include files ********************************************************************************/
#include "AptDefine.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Functions ************************************************************************************/

class AptStdAllocator
{
  public:
    AptStdAllocator();
    AptStdAllocator(const char *name);

  public:
    void *allocate(size_t n, int flags = 0);
    void *allocate(size_t n, size_t alignment, size_t offset, int flags = 0);
    void deallocate(void *p, std::size_t n);

    const char *get_name() const;
    void set_name(const char *pName);

  private:
    const char *m_name;
};