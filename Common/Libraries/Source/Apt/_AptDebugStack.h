/** Templated stack class, used only for debug builds. */

#pragma once
/*** Include files ********************************************************************************/

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

template <class T>
class AptDebugStack
{
  public:
    AptDebugStack()
        : m_nElements(0),
          m_nCapacity(0),
          m_aElements(NULL)
    {
        //  Do nothing...
    }

    AptDebugStack(const int32_t nCapacity)
        : m_nElements(0),
          m_nCapacity(nCapacity)
    {
        APT_ASSERT(nCapacity > 0);
        APT_ASSERT(AptGetUserFuncs().pfnMemAlloc);
#if defined(APT_ALLOCATION_TRACKING)
        m_aElements = (T **)AptGetUserFuncs().pfnMemAlloc(nCapacity * sizeof(T *), __FILE__, __LINE__);
#else
        m_aElements = (T **)AptGetUserFuncs().pfnMemAlloc(nCapacity * sizeof(T *));
#endif
        APT_ASSERT(m_aElements != NULL);
    }

    ~AptDebugStack()
    {
        APT_ASSERT(m_nElements == 0);
        if (m_aElements) // this can cause problem if shutdown is not called and callback is set to 0 while exiting from Apt.
        {
            APT_ASSERT(AptGetUserFuncs().pfnMemFreeSize);
            AptGetUserFuncs().pfnMemFreeSize(m_aElements, m_nCapacity * sizeof(T *));
        }
    }

    void Init(const int32_t nCapacity)
    {
        APT_ASSERT(m_nCapacity == 0);
        m_nCapacity = nCapacity;
        APT_ASSERT(AptGetUserFuncs().pfnMemAlloc);
#if defined(APT_ALLOCATION_TRACKING)
        m_aElements = (T **)AptGetUserFuncs().pfnMemAlloc(m_nCapacity * sizeof(T *), __FILE__, __LINE__);
#else
        m_aElements = (T **)AptGetUserFuncs().pfnMemAlloc(m_nCapacity * sizeof(T *));
#endif
        APT_ASSERT(m_aElements != NULL);
    }

    void Shutdown()
    {
        APT_ASSERT(m_nElements == 0);
        if (m_aElements)
        {
            APT_ASSERT(AptGetUserFuncs().pfnMemFreeSize);
            AptGetUserFuncs().pfnMemFreeSize(m_aElements, m_nCapacity * sizeof(T *));
        }
        m_nCapacity = 0;
        m_nElements = 0;
        m_aElements = NULL;
    }

    int32_t GetCapacity() const
    {
        return (m_nCapacity);
    }

    int32_t GetSize() const
    {
        return (m_nElements);
    }

    void Push(T *element)
    {
        APT_ASSERT(m_nElements < m_nCapacity);
        m_aElements[m_nElements++] = element;
    }

    void Pop()
    {
        --m_nElements;
        delete m_aElements[m_nElements];
        m_aElements[m_nElements] = NULL;
    }

    void Pop(const int32_t nItems)
    {
        APT_ASSERT(GetSize() >= nItems);
        APT_ASSERT(nItems >= 0);
        for (int iLoop = 1; iLoop <= nItems; iLoop++)
        {
            Pop();
        }
    }

    T *Top()
    {
        return (At(0));
    }

    T *At(const int32_t nPos) const
    {
        APT_ASSERT(m_nElements - nPos > 0);
        return (m_aElements[m_nElements - nPos - 1]);
    }

    T *GetPop()
    {
        APT_ASSERT(GetSize() >= 1);
        return (m_aElements[--m_nElements]);
    }

  private:
    int32_t m_nElements;
    int32_t m_nCapacity;
    T **m_aElements;
};

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/
