#pragma once

#include "_AptActions.h"

AptValue *AptActionInterpreter::stackAt(const int32_t nPos) const
{
    return (stack.At(nPos));
}

AptValue *AptActionInterpreter::stackGetPop()
{
    return (stack.GetPop());
}

void AptActionInterpreter::stackPush(AptValue *const pValue)
{
    APT_ASSERT(pValue);
    APT_ASSERT(pValue->isLookup() == false);
    APT_ASSERT(pValue->isRegister() == false);
    APT_ASSERT(stack.GetSize() < stack.GetCapacity());

    stack.Push(pValue);
}

void AptActionInterpreter::stackPushNoInc(AptValue *const pValue)
{
    APT_ASSERT(pValue);
    APT_ASSERT(pValue->isLookup() == false);
    APT_ASSERT(pValue->isRegister() == false);
    APT_ASSERT(stack.GetSize() < stack.GetCapacity());

    stack.PushNoInc(pValue);
}

void AptActionInterpreter::stackPopNoDec()
{
    stack.PopNoDec();
}

void AptActionInterpreter::stackPopAndPush(const int32_t nCountToPop, AptValue *const pValue)
{
    APT_ASSERT(pValue);
    APT_ASSERT(pValue->isLookup() == false);
    APT_ASSERT(pValue->isRegister() == false);
    stack.PopAndPush(nCountToPop, pValue);
}

void AptActionInterpreter::stackPop()
{
    stack.Pop();
}

void AptActionInterpreter::stackPop(const int32_t nItems)
{
    return (stack.Pop(nItems));
}

void AptActionInterpreter::stackSafePop(const int32_t nItems)
{
    return (stack.SafePop(nItems));
}
