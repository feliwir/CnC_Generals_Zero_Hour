/**
 * Templated stack class.
 *
 * Unlike a generic AptValueStack<T>, AptValuePtrStack<T> is specifically for stacks of pointers
 * (Apt only ever needs a stack of AptValue*, never a stack of AptValue objects by value), so it
 * never needs to construct/destruct the pointed-to objects. Define it as
 * AptValuePtrStack<AptValue> (not AptValuePtrStack<AptValue*>) — the implementation always uses T*
 * internally regardless.
 */

#pragma once
/*** Include files ********************************************************************************/

#include <stack>
#include <vector>

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

template <class T>
class AptValuePtrStack
{
  public:
    AptValuePtrStack()
    {
    }

    AptValuePtrStack(int _nSize)
    {
        APT_ASSERT(_nSize > 0);

        m_aElements.reserve(_nSize);
    }

    ~AptValuePtrStack()
    {
        shutdown();
    }

    void init(int _nSize)
    {
        APT_ASSERT(m_aElements.size() == 0);
        m_aElements.reserve(_nSize);
    }

    void shutdown()
    {
        APT_ASSERT(m_aElements.size() == 0);
        m_aElements.clear();
    }

    int capacity() { return (int32_t)m_aElements.capacity(); }

    int size()
    {
        return (int32_t)m_aElements.size();
    }

    void push(T *element)
    {
        m_aElements.push_back(element);
        APT_INC(element);
    }

    void pop()
    {
        APT_ASSERT(m_aElements.size() > 0);
        T *element = m_aElements.back();
        APT_DEC(element);
        m_aElements.pop_back();
    }

    T *top()
    {
        return m_aElements.back();
    }

    T *at(int nPos)
    {
        APT_ASSERT(m_aElements.size() - nPos > 0);
        return m_aElements[m_aElements.size() - nPos - 1];
    }

    void SetAt(int nPos, T *element)
    {
        APT_ASSERT(m_aElements.size() - nPos > 0);
        if (element != m_aElements[m_aElements.size() - nPos - 1])
        {
            // APT_DECSAFE(m_aElements[m_nElements - nPos - 1]);
            m_aElements[m_aElements.size() - nPos - 1] = element;
            // APT_INC(element);
        }
    }

  private:
    std::vector<T *> m_aElements;
};

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/
