#include "_Apt.h"
#include "MainInline.h"
#include "AptObject/AptMovieClipLoader.h"
#include "AptTarget.h"


const char *AptMovieClipLoader::maListenerFunctions[LISTENER_FUNCTIONS_TOT] =
    {
        "onLoadStart",
        "onLoadComplete",
        "onLoadInit",
        "onLoadError"};

int32_t AptMovieClipLoader::mMaxListeners = 64;  // Default value in Apt for iListenerSetSize
int32_t AptMovieClipLoader::mMaxLoading   = 364; // Default value in Apt for iMaxNewMovieClipsPerFrame

AptMovieClipLoader::AptMovieClipLoader()
    : AptObject(AptVFT_MovieClipLoader),
      mListenersSet(mMaxListeners),
      mLoadingSet(mMaxLoading)
{
}

/** Attaches a listener that's notified when the movie finishes loading. */
NATIVE_MEMBER_FUNCTION(AptMovieClipLoader, addListener)
{
    if (nParams > 0)
    {
        AptValue *pListener = gAptActionInterpreter.stackAt(0);
        if (pListener->isObject())
        {
            AptMovieClipLoader *pLoader = (AptMovieClipLoader *)pThis;
            if (!pLoader->mListenersSet.has(pListener))
            {
                pLoader->mListenersSet.add(pListener);
            }
        }
    }
    return gpUndefinedValue;
}

/** Removes a listener previously attached with addListener. */
NATIVE_MEMBER_FUNCTION(AptMovieClipLoader, removeListener)
{
    if (nParams > 0)
    {
        AptValue *pParam = gAptActionInterpreter.stackAt(0);
        if (pParam->getIsDefined())
        {
            AptMovieClipLoader *pLoader = (AptMovieClipLoader *)pThis;
            if (pLoader->mListenersSet.has(pParam))
            {
                pLoader->mListenersSet.remove(pParam);
            }
        }
    }
    return gpUndefinedValue;
}

/**
 * Loads a movieclip.
 * @param pMovie name of the movie file
 * @param pHolder movieclip (or level number) to hold the movie
 * @return true on success
 */
NATIVE_MEMBER_FUNCTION(AptMovieClipLoader, loadClip)
{
    if (nParams < 2)
    {
        return AptBoolean::Create(false);
    }

    AptMovieClipLoader *pLoader = (AptMovieClipLoader *)pThis;

    AptValue *pMovie  = gAptActionInterpreter.stackAt(0);
    AptValue *pTarget = GetTargetParameter(1);

    if (pTarget == NULL || pMovie == NULL)
    {
        return AptBoolean::Create(false);
    }

    AptNativeString sMovieName;
    AptNativeString sTarget;
    pMovie->toString(sMovieName);
    pTarget->toString(sTarget);

    // Composed as movie-name + '*' + target-name (an asterisk can't be part of a filename): the
    // same movie can be loading into several targets at once, but its "complete" notification
    // should only fire once.
    AptString *pComposed = AptString::Create(sMovieName);
    pComposed->cat("*");
    pComposed->cat(sTarget);
    pLoader->mLoadingSet.add(pComposed);

    pLoader->SendToListeners(maListenerFunctions[LOAD_START], 1, pTarget);

    GetTargetSim()->GetLinker()->Load(sMovieName, sTarget, pLoader);

    return AptBoolean::Create(true);
}

/**
 * @param pHolder movieclip (or level number) holding the movie
 * @return an object with bytesLoaded/bytesTotal properties
 */
NATIVE_MEMBER_FUNCTION(AptMovieClipLoader, getProgress)
{
    if (nParams < 1)
    {
        return gpUndefinedValue;
    }

    AptValue *pTarget = GetTargetParameter(0);

    if (pTarget)
    {
        int32_t nBytesLoaded = 0;
        int32_t nBytesTotal  = 0;
        // pfnGetBytesLoaded/pfnGetBytesTotal aren't implemented (yet), so these stay at 0.

        AptObject *pObject      = new AptObject(AptVFT_Object);
        AptNativeString tempStr = "bytesLoaded";
        pObject->Set(&tempStr, AptInteger::Create(nBytesLoaded));
        tempStr = "bytesTotal";
        pObject->Set(&tempStr, AptInteger::Create(nBytesTotal));
        return pObject;
    }

    return gpUndefinedValue;
}

/** @param pHolder movieclip (or level number) holding the movie to unload. */
NATIVE_MEMBER_FUNCTION(AptMovieClipLoader, unloadClip)
{
    if (nParams < 1)
    {
        return gpUndefinedValue;
    }

    AptValue *pTarget = GetTargetParameter(0);

    if (pTarget)
    {
        // Unloading is equivalent to loading an empty movie.
        AptNativeString sMovie("");
        AptNativeString sTarget;
        pTarget->toString(sTarget);
        GetTargetSim()->GetLinker()->Load(sMovie, sTarget);
    }

    return gpUndefinedValue;
}

/** Releases the native script functions; must be called at least during AptShutdown. */
void AptMovieClipLoader::CleanNativeFunctions()
{
    NATIVE_MEMBER_FUNCTION_DESTROY(loadClip);
    NATIVE_MEMBER_FUNCTION_DESTROY(addListener);
    NATIVE_MEMBER_FUNCTION_DESTROY(getProgress);
    NATIVE_MEMBER_FUNCTION_DESTROY(removeListener);
    NATIVE_MEMBER_FUNCTION_DESTROY(unloadClip);
}

void AptMovieClipLoader::Initialize(const AptInitParams &aptInitParms)
{
    mMaxListeners = aptInitParms.iListenerSetSize;
    mMaxLoading   = aptInitParms.iMaxNewMovieClipsPerFrame;
}

AptValue *AptMovieClipLoader::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    char c = *pName->c_str();
    switch (c)
    {
    case 'l':
        if (strcmp(pName->c_str(), "loadClip") == 0)
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(loadClip);
        }
        break;
    case 'a':
        if (strcmp(pName->c_str(), "addListener") == 0)
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(addListener);
        }
        break;
    case 'g':
        if (strcmp(pName->c_str(), "getProgress") == 0)
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(getProgress);
        }
        break;
    case 'r':
        if (strcmp(pName->c_str(), "removeListener") == 0)
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(removeListener);
        }
        break;
    case 'u':
        if (strcmp(pName->c_str(), "unloadClip") == 0)
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(unloadClip);
        }
        break;
    }
    return 0;
}

/** Called when a load completes or aborts; notifies the listeners. */
void AptMovieClipLoader::NotifyListeners(const AptNativeString &sMovie, int32_t notification)
{
    int nCount = mLoadingSet.mnElements;
    int nFile  = sMovie.GetLength();

    for (int32_t i = 0; i < mLoadingSet.capacity(); ++i)
    {
        AptString *pLoading = mLoadingSet.maElements[i];
        // The movie name is the first part of the composed string.
        if (pLoading)
        {
            AptNativeString *pString = pLoading->GetInternalString();
            if (pString->UTF8_CharAt(nFile) == '*' && strncmp(pString->GetBuffer(), sMovie, nFile) == 0)
            {
                bool bRemove = false;
                // The target is the last part of the composed string (after the asterisk).
                AptValue *pTarget = AptString::Create(pString->Right(pString->GetLength() - nFile - 1));
                switch (notification)
                {
                case LOAD_ERROR:
                {
                    // The error notification also carries the movie name.
                    bRemove          = true;
                    AptValue *pMovie = AptString::Create(sMovie.GetBuffer());
                    SendToListeners(maListenerFunctions[notification], 2, pTarget, pMovie);
                    break;
                }
                case LOAD_COMPLETE:
                    SendToListeners(maListenerFunctions[notification], 1, pTarget);
                    break;
                case LOAD_INIT:
                    bRemove = true;
                    SendToListeners(maListenerFunctions[notification], 1, pTarget);
                    break;
                }
                if (bRemove)
                {
                    APT_DEC(mLoadingSet.maElements[i]);
                    mLoadingSet.mnElements--;
                    mLoadingSet.maElements[i] = 0;
                }
            }
            nCount--;
            if (nCount == 0)
            {
                break;
            }
        }
    }
}

/** Calls each attached listener's named function with the given arguments. */
void AptMovieClipLoader::SendToListeners(const char *szFunctionName, int32_t nNumParams, ...)
{
    int32_t nCount = mListenersSet.mnElements;
    AptNativeString strName(szFunctionName);

    for (int32_t i = 0; i < mListenersSet.capacity() && nCount > 0; ++i)
    {
        AptValue *pListener = mListenersSet.maElements[i];
        if (pListener)
        {
            va_list varargs;
            va_start(varargs, nNumParams);
            AptCallMemberFunctionV(szFunctionName, NULL, pListener, nNumParams, varargs);
            va_end(varargs);
            nCount--;
        }
    }
}

/**
 * Resolves the "target" parameter, which can be a movieclip or a level number.
 * @param idxParameter parameter index
 */
AptValue *AptMovieClipLoader::GetTargetParameter(int32_t idxParameter)
{
    AptValue *pTarget = gAptActionInterpreter.stackAt(idxParameter);

    if (pTarget->isInteger()) // It's a level number.
    {
        pTarget = _AptGetAnimationAtLevel(pTarget->toInteger());
    }

    return pTarget;
}
