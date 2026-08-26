/**
 * Templated stack class.
 *
 * Unlike a generic AptBaseStack<T>, AptBasePtrStack<T> is specifically for stacks of pointers (Apt
 * only ever needs a stack of AptValue*, never a stack of AptValue objects by value), so it never
 * needs to construct/destruct the pointed-to objects. Define it as AptBasePtrStack<AptValue> (not
 * AptBasePtrStack<AptValue*>) — the implementation always uses T* internally regardless.
 */

#pragma once
/*** Include files ********************************************************************************/
#include <stack>
#include <vector>

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

template <class T>
class AptBasePtrStack
{
  public:
    AptBasePtrStack()
    {
        //  Do nothing...
    }

    ~AptBasePtrStack()
    {
        Shutdown();
    }

    void Init(const int32_t nCapacity)
    {
        APT_ASSERT(m_aElements.size() == 0);
        m_aElements.reserve(nCapacity);
    }

    void Shutdown()
    {
        APT_ASSERT(m_aElements.size() == 0);
        m_aElements.clear();
    }

    int32_t GetCapacity() const
    {
        return (int32_t)m_aElements.capacity();
    }

    int32_t GetSize() const
    {
        return (int32_t)m_aElements.size();
    }

    void Push(T *element)
    {
        m_aElements.push_back(element);

        // 5/12/04.
        // this is added as if deferred vector is full then we need
        // to switch back to older way of APT_INC/APT_DEC for stack operations.
        // Actually using AptValue * is not correct as this is a templated class
        // but here we assume that T is always a T * as we are not using this class
        // for anythign else rightnow in Apt.
        AptValue *pValue = (AptValue *)element;
        APT_INC(pValue);
    }

    void PushNoInc(T *element)
    {
        m_aElements.push_back(element);
    }

    void Pop()
    {
        if (m_aElements.empty())
        {
            APT_ASSERT(false && "[APT] Error, Popping from Stack with 0 elements. Please contact the Apt Team for Support.");
        }
        else
        {
            // 5/12/04.
            // this is added as if deferred vector is full then we need
            // to switch back to older way of APT_INC/APT_DEC for stack operations.
            AptValue *pValue = (AptValue *)m_aElements.back();
            APT_DEC(pValue);
            m_aElements.pop_back();
        }
    }

    void Pop(const int32_t nItems)
    {
        APT_ASSERT(nItems >= 0);
        if ((int32_t)m_aElements.size() < nItems)
        {
            APT_ASSERT(false && "[APT] Error, Popping more elements than the stack contains. Please contact the Apt Team for Support.");
        }
        else
        {
            // 5/12/04.
            // this is added as if deferred vector is full then we need
            // to switch back to older way of APT_INC/APT_DEC for stack operations.
            for (int iLoop = 1; iLoop <= nItems; iLoop++)
            {
                AptValue *pValue = (AptValue *)m_aElements.back();
                APT_DEC(pValue);
                m_aElements.pop_back();
            }
        }
    }

    void PopNoDec()
    {
        if (m_aElements.empty())
        {
            APT_ASSERT(false && "[APT] Error, Popping from Stack with 0 elements. Please contact the Apt Team for Support.");
        }
        else
        {
            m_aElements.pop_back();
        }
    }

    void PopAndPush(const int32_t nItems, T *element)
    {
        APT_ASSERT(nItems >= 0);

        if ((int32_t)m_aElements.size() < nItems)
        {
            APT_ASSERT(false && "[APT] Error, Popping more elements than the stack contains. Please contact the Apt Team for Support.");
        }
        else
        {
            {
                AptValue *pValue = (AptValue *)element;
                APT_INC(pValue);
            }

            // 5/12/04.
            // this is added as if deferred vector is full then we need
            // to switch back to older way of APT_INC/APT_DEC for stack operations.
            for (int iLoop = 1; iLoop <= nItems; iLoop++)
            {
                AptValue *pValue = (AptValue *)m_aElements.back();
                APT_DEC(pValue);
                m_aElements.pop_back();
            }

            m_aElements.push_back(element);
        }
    }

    void SafePop(const int32_t nItems)
    {

        if (nItems <= 0)
        {
            return;
        }

        if ((int32_t)m_aElements.size() < nItems)
        {
            APT_ASSERT(false && "[APT] Error, Popping more elements than the stack contains. Please contact the Apt Team for Support.");
        }
        else
        {

            // 5/12/04.
            // this is added as if deferred vector is full then we need
            // to switch back to older way of APT_INC/APT_DEC for stack operations.
            for (int iLoop = 1; iLoop <= nItems; iLoop++)
            {
                AptValue *pValue = (AptValue *)m_aElements.back();
                APT_DEC(pValue);
                m_aElements.pop_back();
            }
        }
    }

    T *Top()
    {
        return m_aElements.back();
    }

    T *At(const int32_t nPos) const
    {
        APT_ASSERT(m_aElements.size() - nPos > 0);
        return (m_aElements[m_aElements.size() - nPos - 1]);
    }

    T *GetPop()
    {
        APT_ASSERT(GetSize() >= 1);
        T *pValue = m_aElements.back();
        m_aElements.pop_back();
        return pValue;
    }

  private:
    std::vector<T *> m_aElements;
};

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/
