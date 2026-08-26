#pragma once

#include "AptExtObject.h"

/**
 * @brief This class is a singleton, never instantiated, used internally only by Apt.cpp in the
 * AptRegisterExtension and AptUnRegisterExtension API functions.
 */
class AptExtObjectRegistry
{
  public:
    static void Register(class AptExtObject *pExtObject);

    static void UnRegister(const char *szName);

  private:
    using ScopeFuncPtr = AptObject *(*)(const AptNativeString &, AptObject *);

    static int32_t _ForEachScope(const AptNativeString &sObjName, AptObject **ppScope, ScopeFuncPtr pActionFunction);
    static AptObject *_CreateScope(const AptNativeString &sName, AptObject *pScope);
    static AptObject *_GetNextScope(const AptNativeString &sName, AptObject *pScope);
    static bool _UnsetEmptyChild(AptObject *pScope, const AptNativeString &sFullName, int32_t i, int32_t j);

  private:
    AptExtObjectRegistry();
    AptExtObjectRegistry(const AptExtObjectRegistry &);
    AptExtObjectRegistry &operator=(const AptExtObjectRegistry &);
};

