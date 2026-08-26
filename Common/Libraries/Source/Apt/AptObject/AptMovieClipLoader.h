/**
    This file defines a the actionscript "MovieClipLoader" object handlers.
*/

#pragma once

#include "AptObject/AptObject.h"
#include "_AptSet.h"

enum MovieClipLoaderNotifications
{
    LOAD_START = 0,
    LOAD_COMPLETE,
    LOAD_INIT,
    LOAD_ERROR,
    LISTENER_FUNCTIONS_TOT
};

class AptMovieClipLoader : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS

    AptMovieClipLoader();

    NATIVE_MEMBER_FUNCTION_DECL(loadClip);
    NATIVE_MEMBER_FUNCTION_DECL(addListener);
    NATIVE_MEMBER_FUNCTION_DECL(getProgress);
    NATIVE_MEMBER_FUNCTION_DECL(removeListener);
    NATIVE_MEMBER_FUNCTION_DECL(unloadClip);

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;

    static void Initialize(const AptInitParams &aptInitParms);
    static void CleanNativeFunctions();

    void NotifyListeners(const AptNativeString &sMovie, int32_t notification);

  protected:
    virtual ~AptMovieClipLoader() {}

  private:
    void SendToListeners(const char *szName, int32_t nNumParams, ...);
    static AptValue *GetTargetParameter(int32_t idxParameter);

    static const char *maListenerFunctions[];
    static int32_t mMaxListeners;
    static int32_t mMaxLoading;

    AptValueSet<AptValue *> mListenersSet;
    AptValueSet<AptString *> mLoadingSet;
};
