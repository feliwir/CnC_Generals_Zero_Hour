/**
 * Extremely basic ActionScript call stack, used for debugging in opt builds of Apt. Designed to be
 * resilient to overflow/underflow since it is a debugging aid, not a critical system: no asserts on
 * misuse, it just gracefully does nothing.
 */

#pragma once

#include <vector>
#include "AptValue/AptValue.h"
#include "AptDefine.h"

class AptStackItem
{
  public:
    APT_NEW_DELETE_OPERATORS

    const char *file;
    const char *scope;
    const char *function;
    AptValue *context;

    inline AptStackItem(const char *function = 0, const char *file = (const char *)"<native>", AptValue *context = 0, const char *scope = 0)
        : file(file), scope(scope), function(function), context(context) {}
    inline AptStackItem(const AptStackItem &item)
        : file(item.file), scope(item.scope), function(item.function), context(item.context) {}
};

/**
 * Not intended to be instantiated by user code; a single instance named gAptOptCallStack is
 * provided for debugging convenience.
 */
class AptCallStack
{
  public:
    AptCallStack()
    {
    }

    ~AptCallStack()
    {
        Resize(0);
    }

    void Resize(int size)
    {
        mData.clear();
        mData.reserve(size);
    }

    /** Pushes a stack item (including the name of the function) onto the stack. */
    inline void Push(const AptStackItem &item)
    {
        mData.push_back(item);
    }

    inline void Push(const char *function, const char *file = (const char *)"<native>", AptValue *context = 0, const char *scope = 0)
    {
        Push(AptStackItem(function, file, context, scope));
    }

    /** Pops the top item from the stack. */
    inline void Pop()
    {
        if (!mData.empty())
        {
            mData.pop_back();
        }
    }

    /** @return the top item on the stack. */
    inline AptStackItem &Top()
    {
        if (mData.empty())
            return mNull;
        return mData.back();
    }

    /** @return the item at @p index, or a null item if out of range [0, GetItemCount()). */
    inline const AptStackItem &GetItem(int index) const
    {
        if (index >= 0 && index < (int32_t)mData.size())
        {
            return mData[index];
        }
        return mNull;
    }

    /** @return the number of items in the stack. */
    inline int GetItemCount() const
    {
        return (int)mData.size();
    }

  private:
    std::vector<AptStackItem> mData;
    AptStackItem mNull;
};
