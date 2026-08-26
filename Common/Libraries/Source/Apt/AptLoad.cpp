#include "_Apt.h"
#include "_AptFileConvert.h"
#include "AptTarget.h"
#include "AptAnimationTarget.h"
#include "AptCIH.h"
#include "AptCharacterInst.h"
#include "Display/AptDisplayListState.h"
#include "AptRenderList.h"
#include "AptObject/AptMovieClipLoader.h"
#if defined(APT_DEBUGGER_ENABLE)
#include "AptDebugger/AptDebugger.h"
#endif

#include "MainInline.h"


// this is just added for debugging to see if we are deleting any AptValues
// from render thread when we are 'not' unresolving a file.
// int32_t gnUnresolvingFile = 0;

/*

Externally need:

    LoadMovie (unload)
    preload/release preloaded
    Block for load/unload
    Cancel load
    Update
    user callback for finishing data load

    saved input checkpoint
    check if right animations are loaded for input playback

*/
#if !defined(APT_PLATFORM_PS3) && !defined(APT_PLATFORM_PS4)
// this #define is added just to get it compiling for ps3, as it gives compiler errors for templates on ps3.
// these objects are really not needed
#endif

void GlobalNotificationFunction(AptFilePtr f);

bool AptFileNameCompare(const char *str1, const char *str2)
{
    if (str1 == NULL || str2 == NULL)
    {
        return false;
    }

    int nlen1 = (int)strlen(str1);
    if (nlen1 != (int)strlen(str2))
    {
        return false; // length are not the same, return false.
    }

    for (int i = 0; i < nlen1; i++)
    {
        if (str1[i] == str2[i]) // same char
        {
            continue;
        }
        char cChar1 = str1[i];
        char cChar2 = str2[i];

        if (cChar1 >= 'a' && cChar1 <= 'z')
        {
            cChar1 += 'A' - 'a';
        }
        else if (cChar1 == '\\')
        {
            cChar1 = '/';
        }

        if (cChar1 == cChar2)
        {
            continue; // chars are equal now
        }

        if (cChar2 >= 'a' && cChar2 <= 'z')
        {
            cChar2 += 'A' - 'a';
        }
        else if (cChar2 == '\\')
        {
            cChar2 = '/';
        }

        if (cChar1 != cChar2)
        {
            return false;
        }
    }
    return true;
}

static bool AptIsBuiltForDecoupling(void *pAptData)
{
    char *sAptTag = static_cast<char *>(pAptData);

    if (sAptTag[8] == ':')
    {
        return (sAptTag[9] == '1');
    }
    return false;
}

static int AptGetPtrSize(void *pAptData)
{
    char *sAptTag = static_cast<char *>(pAptData);

    if (sAptTag[12] == ':')
    {
        return ((sAptTag[13] - '0'));
    }
    return 4;
}

AptFile::~AptFile()
{
    AptUpdateAutoLock lock;
    if (mState == WaitingForData)
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "TODO: should have cancelled load of '%s'\n", mName.ConstRawPtr());
    }

    if (GetTarget() && GetTarget()->GetLoader())
    {
        GetTarget()->GetLoader()->Invalidate(this);
    }

    // if (mState == WaitingForData)
    //{
    //	APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "TODO: should have cancelled load of '%s'\n", mName.ConstRawPtr());
    // }

    // If the MovieClipLoader is still associated at this point, CompleteAnimationLoad was not called, so we notify that the load failed
    NotifyMovieClipLoader((int32_t)LOAD_ERROR);
}

AptMovieClipLoader *AptFile::GetMovieClipLoader() const
{
    return mMovieClipLoader;
}

void AptFile::SetMovieClipLoader(AptMovieClipLoader *pLoader)
{
    if (pLoader)
    {
        APT_INC(pLoader);
    }
    if (mMovieClipLoader)
    {
        APT_DEC(mMovieClipLoader);
    }
    mMovieClipLoader = pLoader;
}

void AptFile::NotifyMovieClipLoader(int32_t notification)
{
    AptMovieClipLoader *pLoader = GetMovieClipLoader();
    if (pLoader)
    {
        pLoader->NotifyListeners(GetName(), notification);
        if (notification == LOAD_INIT || notification == LOAD_ERROR)
        {
            SetMovieClipLoader(NULL); // Cleans the association
        }
    }
}

AptCharacter *AptAnimationFile::FindExport(const char *szName)
{
    for (int i = 0; i < mCharacter->animation.nExports; ++i)
    {
        if (strcmp(szName, mCharacter->animation.aExports[i].szName) == 0)
        {
            APT_ASSERT(mCharacter->animation.aExports[i].nID >= 0 && mCharacter->animation.aExports[i].nID < mCharacter->animation.nCharacters);
            AptCharacter *pRet = mCharacter->animation.apCharacters[mCharacter->animation.aExports[i].nID];
            APT_ASSERT(pRet);
            return pRet;
        }
    }
    APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "Couldn't find export named '%s' in '%s'\n", szName, mName.ConstRawPtr());
    APT_ASSERT(NOT_REACHED);
    return 0;
}

/******************************************************************************/
/**
    @brief  AptAnimationFile destructor
*/
/******************************************************************************/
AptAnimationFile::~AptAnimationFile()
{
#ifdef APT_DEBUGGER_ENABLE
    AptDebugger::GetInstance()->AnimationFileUnloaded(this);
#endif

    if (mState == WaitingForImports || mState == Resolved || mState == Zombie || mState == Unloaded)
    {
        if (mCharacter != NULL)
        {
            // gnUnresolvingFile = 1;
            mCharacter->animation.Unresolve(mAptData);
            // right now we use a magic number for userdata pointer for filters preloaded file
            if (mUserData != (void *)APT_FILTERS_FILE_USERDATA)
            {
                AptGetUserFuncs().pfnFreeAnimation(mUserData);
            }
            // gnUnresolvingFile = 0;
        }
    }

    if (mConvertedAptData != NULL)
    {
        APT_FREE_BLOCK(mConvertedAptData, mConvertedAptSize);
        mConvertedAptData = NULL;
        mConvertedAptSize = 0;
    }

    mAptData   = NULL;
    mCharacter = NULL;
    mUserData  = NULL;
}

/******************************************************************************/
/**
    @brief  AptImageFile destructor
*/
/******************************************************************************/
AptImageFile::~AptImageFile()
{
    // Mimicking what was in AptAnimationFile. Not sure why we only
    // call free in these situations
    if (mState == WaitingForImports || mState == Resolved || mState == Zombie || mState == Unloaded)
    {
        if (NULL != mUserData)
        {
            AptGetUserFuncs().pfnFreeImage(mUserData);
        }
    }

    if (mCharacter)
    {
        delete mCharacter;
        mCharacter = NULL;
    }
    mUserData = NULL;
    mTexture  = NULL;
}

/******************************************************************************/
/**
    @brief  Set internal data for an AptImageFile

    @param  texture  Actual texture
    @param  width    Width of the texture
    @param  height   Height of the texture
    @param  userData Userdata, which will be passed back when the image file
                     is freed
*/
/******************************************************************************/
void AptImageFile::SetData(AptAssetTexture texture, int width, int height, void *userData)
{
    APT_ASSERT(NULL == mTexture);
    APT_ASSERT(NULL == mUserData);
    APT_ASSERT(NULL == mCharacter);

    mTexture  = texture;
    mUserData = userData;
    mWidth    = width;
    mHeight   = height;

    AptCharacter *imageChar = new AptCharacter();
    imageChar->eType        = AptCharacterType_Image;
    imageChar->SetupCharacter();
    imageChar->image.bounds.fTop    = 0.f;
    imageChar->image.bounds.fLeft   = 0.f;
    imageChar->image.bounds.fRight  = static_cast<float>(width);
    imageChar->image.bounds.fBottom = static_cast<float>(height);
    imageChar->image.texture        = mTexture;
    mCharacter                      = imageChar;
}

void AptSavedInputCheckpoints::updateState(const AptNativeString &name, AptFileSavedInputState::State lookFor, AptFileSavedInputState::State setTo, AptFileSavedInputState::State ifNotFound)
{
    for (auto &i : mPending)
    {
        if (i.GetName() == name)
        {
            if (i.GetState() == lookFor)
            {
                i.SetState(setTo);
            }
            return;
        }
    }

    mPending.push_back(AptFileSavedInputState(name, ifNotFound));
}

bool AptSavedInputCheckpoints::allStatesAre1(AptFileSavedInputState::State state0)
{
    for (auto &i : mPending)
        if (i.GetState() != state0)
            return false;
    return true;
}

bool AptSavedInputCheckpoints::allStatesAre2(AptFileSavedInputState::State state0, AptFileSavedInputState::State state1)
{
    for (auto &i : mPending)
    {
        if (i.GetState() != state0 && i.GetState() != state1)
        {
            // APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "Waiting For %s In State %d\n", i->GetName().c_str(), i->GetState());
            return false;
        }
    }
    return true;
}

bool AptSavedInputCheckpoints::allStatesAre3(AptFileSavedInputState::State state0, AptFileSavedInputState::State state1, AptFileSavedInputState::State state2)
{
    for (auto &i : mPending)
        if (i.GetState() != state0 && i.GetState() != state1 && i.GetState() != state2)
            return false;
    return true;
}

void AptSavedInputCheckpoints::Checkpoint(const AptNativeString &s)
{
    for (auto &i : mPending)
    {
        if (i.GetName() == s)
        {
            if (i.GetState() == AptFileSavedInputState::LoadedButNotAtCheckpoint)
            {
                i.SetState(AptFileSavedInputState::ReadyToBeLinked);
            }
            return;
        }
    }

    if (GetTargetSim()->GetLoader()->IsLoaded(s))
    {
        mPending.push_back(AptFileSavedInputState(s, AptFileSavedInputState::ReadyToBeLinked));
    }
    else
    {
        mPending.push_back(AptFileSavedInputState(s, AptFileSavedInputState::ExpectedBySavedInputButUnavailable));
    }
}

void AptSavedInputCheckpoints::FileLoaded(const AptNativeString &s)
{
    updateState(s, AptFileSavedInputState::ExpectedBySavedInputButUnavailable,
                AptFileSavedInputState::ReadyToBeLinked,
                AptFileSavedInputState::LoadedButNotAtCheckpoint);
}

void AptSavedInputCheckpoints::AllLinked()
{
    FileSavedInputStateVector x = FileSavedInputStateVector(); // for some reason, ngc-mw requires the temporary
    mPending.swap(x);
}

bool AptSavedInputCheckpoints::CanLinkPendingFiles()
{
    return allStatesAre2(AptFileSavedInputState::ReadyToBeLinked, AptFileSavedInputState::LoadedButNotAtCheckpoint);
}

bool AptSavedInputCheckpoints::CanContinueSavedInputs()
{
    return allStatesAre2(AptFileSavedInputState::Linked, AptFileSavedInputState::LoadedButNotAtCheckpoint);
}

bool AptAnimationFile::isFileImported(AptFilePtr pFile)
{
    // this call should be called only from linker code as it checks if pFile is
    // imported by 'this' animation.
    for (int j = 0; j < GetMainCharacter()->animation.nImports; j++)
    {
        if (AptNativeString(GetMainCharacter()->animation.aImports[j].szFile) == pFile->GetName())
        {
            return true;
        }
    }
    return false;
}

// ------------------------------------
void AptLoader::GetFileVector(AptFilePtr *aFilePtrs, int nMaxSize)
{
    int j = 0;
    APT_THREAD_NAMESPACE::AutoFutex lock(mFilesLock);
    for (auto &mFile : mFiles)
    {
        if (j < nMaxSize)
        {
            aFilePtrs[j] = AptFilePtr(mFile);
        }
        ++j;
    }

    if (j > nMaxSize)
    {
        char buf[128];
        SNPRINTF(buf, 128, "AptLoader::GetFileVector() was passed a max list size of %d, but there are %d files loaded!", nMaxSize, j);
        APT_FAIL(buf);
    }
}

AptFilePtr AptLoader::findFile(const AptNativeString &sFilename)
{
    for (auto &mFile : mFiles)
    {
        // Crash in AptLoader::Update

        if (AptFileNameCompare(mFile->GetName(), sFilename.c_str()) == true)
        {
            return AptFilePtr(mFile);
        }
    }

    return AptFilePtr(0);
}

void AptSharedPtrDelete(AptFile *p);

AptLoader::~AptLoader()
{
    // 06.15.04 : run through and cancel any preloaded animations to ensure everything is freed properly.
    APT_THREAD_NAMESPACE::AutoFutex lock(mFilesLock);
    while (!mFiles.empty())
    {
        RawFileList::iterator i = mFiles.begin();
        CancelPreloadedAnimation((*i)->GetName());
        if (i == mFiles.begin())
        {
            APT_FAIL("A reference to a swf file remains!");

            AptFile *filePtr = *i;

            // Manually update the mFiles list now. This must be done before calling AptSharedPtrDelete() because
            //   ~AptFile() will call AptLoader::Invalidate() to update mFiles. However, if the AptRenderList contains
            //   this AptFile, then it won't be deleted immediately. It's okay if AptLoader::Invalidate() is called
            //   after this update because it will properly handle the case that the AptFile* is not in mFiles.
            mFiles.erase(i);

            AptSharedPtrDelete(filePtr); // allow this to continue w/o failing, but seek out the problem!!
        }
    }
}
// ------------------------------------
/** @brief Used by ~AptFile to remove view-only raw pointers from mFiles */
void AptLoader::Invalidate(AptFile *pFile)
{
    APT_THREAD_NAMESPACE::AutoFutex lock(mFilesLock);
    if ((mFiles.begin() != mFiles.end()) && *(mFiles.begin()) == pFile)
    {
        mFiles.pop_front();
        return;
    }

    for (RawFileList::iterator i = mFiles.begin(); i != mFiles.end(); ++i)
    {
        RawFileList::iterator next(i);
        ++next;
        if (next != mFiles.end() && *next == pFile)
        {
            mFiles.erase(next);
            return;
        }
    }
}

// ------------------------------------
void AptLoader::notify(AptFilePtr f)
{

    GlobalNotificationFunction(f);
}

// ------------------------------------
AptFilePtr AptLoader::IsLoaded(const AptNativeString &sFilename)
{
    APT_THREAD_NAMESPACE::AutoFutex lock(mFilesLock);
    AptFilePtr f = findFile(sFilename);
    if (f && (f->GetState() == AptFile::Resolved || f->GetState() == AptFile::Zombie))
    {
        return f;
    }
    return AptFilePtr(0);
}

/******************************************************************************/
/**
    @brief  Adds the file to a queue of files that are waiting to be loaded.

    @param  sFilename Name of the file
    @param  fileType  Type of file we know this to be

    @return           - Pointer to the file

    File may already be loaded or loading, in which case we'll return the same
    Ptr as last time this was called
*/
/******************************************************************************/
AptFilePtr AptLoader::Load(const AptNativeString &sFilename, AptFileType fileType)
{
    APT_THREAD_NAMESPACE::AutoFutex lock(mFilesLock);
    AptFilePtr f = findFile(sFilename);
    if (f.pData != NULL)
    {
        // load is already in progress, don't do anything
        return f;
    }

    AptFile *pFile = NULL;
    switch (fileType)
    {
    case AptFileType_Animation:
        pFile = new AptAnimationFile(sFilename);
        break;
    case AptFileType_Image:
        pFile = new AptImageFile(sFilename);
        break;
    default:
        APT_FAIL("Unsupported file type");
        break;
    }
    AptFilePtr ret(pFile);
    mFiles.push_front(pFile);
    return ret;
}

// ------------------------------------
bool AptLoader::AllImportsAvailable(AptFilePtr f)
{
    bool bReady = true;
    switch (f->GetFileType())
    {
    case AptFileType_Animation:
    {
        AptAnimationFile *af = static_cast<AptAnimationFile *>(f.Get());
        for (int j = 0; j < af->GetMainCharacter()->animation.nImports && bReady; j++)
        {
            if (!IsLoaded(AptNativeString(af->GetMainCharacter()->animation.aImports[j].szFile)))
            {
                bReady = false;
                break;
            }
        }
        break;
    }

    default:
        // Assuming this file doesn't have any imports or whatnot
        break;
    }

    return bReady;
}

// ------------------------------------
// TODO: maintain an "all resolved" flag
void AptLoader::Update()
{
    bool bMovedToResolved = false;

    do
    {
        bMovedToResolved = false;

        for (RawFileList::iterator i = mFiles.begin(); i != mFiles.end(); ++i)
        {
            for (; i != mFiles.end();)
            {
                AptFilePtr f(*i);

                AptFile::State state = f->GetState();
                if (state == AptFile::Queued)
                {
                    f->setState(AptFile::WaitingForData);

                    void (*loadCallback)(const char *, AptFilePtr) = NULL;
                    switch (f->GetFileType())
                    {
                    case AptFileType_Animation:
                        loadCallback = AptGetUserFuncs().pfnLoadAnimation;
                        break;
                    case AptFileType_Image:
                        loadCallback = AptGetUserFuncs().pfnLoadImage;
                        break;
                    default:
                        APT_FAIL("File type not yet supported");
                        break;
                    }
                    APT_ASSERTM(NULL != loadCallback, ("Loading callback unset for this type of file!"));



                    loadCallback(f->GetName().ConstRawPtr(), f);


                    i = mFiles.begin();
                }
                else if (state == AptFile::WaitingForData)
                {
                    break;
                }
                else if (state == AptFile::WaitingForImports)
                {
                    if (AllImportsAvailable(f))
                    {
                        f->setState(AptFile::Resolved);
                        bMovedToResolved = true;

                        switch (f->GetFileType())
                        {
                        case AptFileType_Animation:
                        {
                            AptAnimationFile *animFile = AptAnimationFile::Cast(f);
                            animFile->GetMainCharacter()->animation.Link(animFile->GetMainCharacter(), animFile->GetUserData());
                            break;
                        }

                        case AptFileType_Image:
                        {
                            // Nothing to link up, here
                            break;
                        }

                        default:
                        {
                            APT_FAIL("Unsure how to link up file of this type");
                            break;
                        }
                        }

                        notify(f);
                    }
                    else
                    {
                        break;
                    }
                }
                else if (state == AptFile::Resolved || state == AptFile::Zombie || state == AptFile::Unloaded)
                {
                    break;
                }
                else
                {
                    APT_ASSERT(false && "Invalid File in the file List!!!");
                    Invalidate(f.pData); // Remove it from the List!!! to prevent a crash... but that only hides the problem.
                    i = mFiles.begin();
                }
            }

            if (mFiles.empty())
            {
                // The for() loop's ++i statement would dereference NULL if the list is empty.
                // mFiles can be empty for couple different reasons: if pfnLoadAnimation tried
                // to load a nonexistent file, or if Invalidate() was called, AND if Apt hasn't
                // loaded the filters bytecode (i.e. if APT_USE_FILTERS is disabled).
                break;
            }
        }
    } while (bMovedToResolved);
}
// ------------------------------------

void AptLoader::CompleteAnimationLoad(AptFilePtr afp, void *pData, void *pConstTable, void *pUserData)
{
    // load was canceled, just discard the fileptr
    if (pData == 0)
        return;

    APT_ASSERT(afp->GetFileType() == AptFileType_Animation);
    APT_ASSERT(afp->GetState() == AptFile::WaitingForData);

    AptAnimationFile *f = AptAnimationFile::Cast(afp);

    int nPtrSize = AptGetPtrSize(pData);

    void *pConvertedConst      = NULL;
    size_t nConvertedConstSize = 0;
    if (nPtrSize != APT_PLATFORM_PTR_SIZE)
    {
#if APT_PLATFORM_PTR_SIZE == 8
        if (nPtrSize == 4 && pConstTable)
        {
            // The file was built by a 32-bit swfc: its pointer slots are 4 bytes wide, so
            // struct sizes and member offsets differ from ours. Convert both blobs into
            // freshly allocated native-layout (still unresolved) buffers and continue the
            // normal pipeline on those.
            void *pConvertedApt      = NULL;
            size_t nConvertedAptSize = 0;
            if (!AptConvertFile32(pData, pConstTable,
                                  &pConvertedApt, &nConvertedAptSize,
                                  &pConvertedConst, &nConvertedConstSize))
            {
                APT_FAIL("Failed to convert 32-bit .apt file for a 64-bit process");
                return;
            }
            f->setConvertedAptData(pConvertedApt, nConvertedAptSize);
            pData = pConvertedApt;
        }
        else
#endif
        {
            APT_FAIL("This .apt file was built for a different pointer size and cannot be loaded");
            return;
        }
    }

    // resolve the file
    unsigned char *pBase = (unsigned char *)pData;

    if (pConstTable)
    {
        // The const blob is used in place: aConstants and the string constants it points
        // at are offsets relative to the const blob base (resolved by Resolve/_parseStream),
        // while pMainCharacter is an offset into the .apt blob.
        AptConstFile *pConstFile = pConvertedConst ? (AptConstFile *)pConvertedConst
                                                   : (AptConstFile *)pConstTable;
        APT_RESOLVE(pConstFile->pMainCharacter);
        pConstFile->pMainCharacter->animation.Resolve(pData, pConstFile, pUserData, afp);
        // If the load was started by a MovieClipLoader, it has to notify the listeners that the load is complete
        f->NotifyMovieClipLoader((int32_t)LOAD_COMPLETE);

        f->setDataPointers(pData, pConstFile->pMainCharacter, pUserData);

        // Check if it is correct edition "APT", if not clear timeStamp.
        bool hasTimeStamp = ('A' == pConstFile->aMagic[0] &&
                             'p' == pConstFile->aMagic[1] &&
                             't' == pConstFile->aMagic[2] &&
                             '1' == pConstFile->aMagic[3]);

        char *source      = pConstFile->GetTimeStamp();
        char *destination = f->GetTimeStamp();

        for (int i = 0; i < f->GetTimeStampSize(); ++i)
        {
            destination[i] = hasTimeStamp ? source[i] : 0;
        }

        f->setState(AptFile::WaitingForImports); // Set state after all the work is done, this allows this callback to run in parallel with AptUpdate.
        APT_UNRESOLVE(pConstFile->pMainCharacter);

        if (pConvertedConst)
        {
            APT_FREE_BLOCK(pConvertedConst, nConvertedConstSize);
        }

        if (pUserData != (void *)APT_FILTERS_FILE_USERDATA)
        {
            AptGetUserFuncs().pfnFreeConstantTable(pConstTable);
        }

#ifdef APT_DEBUGGER_ENABLE
        if (NULL != AptDebugger::GetInstance())
        {
            AptDebugger::GetInstance()->AnimationFileLoaded(f);
        }
#endif
    }
}

/******************************************************************************/
/**
    @brief  An Image has finished loading

    @param  afp      File pointer
    @param  texture  Texture data
    @param  width    Width of the image
    @param  height   Height of the image
    @param  userData User data, which will be passed out later when this thing
            is unloaded again
*/
/******************************************************************************/
void AptLoader::CompleteImageLoad(AptFilePtr afp, AptAssetTexture texture, int width, int height, void *userData)
{
    // load was canceled, just discard the fileptr
    if (texture == 0)
        return;

    APT_ASSERT(afp->GetFileType() == AptFileType_Image);
    APT_ASSERT(afp->GetState() == AptFile::WaitingForData);

    AptImageFile *f = AptImageFile::Cast(afp);
    f->SetData(texture, width, height, userData); // Expected this will create the Character
#if defined(APT_DECOUPLED_RENDERING)
    f->GetCharacter()->m_pAnimFile = afp;
#endif

    // There are no imports in AptAnimationFiles, but we want to still go through the nice
    // file state machine so we'll set the state here and wait to move to AptFile::Resolved
    // in the Update() function.
    f->setState(AptFile::WaitingForImports);

    // If the load was started by a MovieClipLoader, it has to notify the listeners that the load is complete
    f->NotifyMovieClipLoader(static_cast<int>(NULL != texture ? LOAD_COMPLETE : LOAD_ERROR));
}

void AptLoader::CancelPreloadedAnimation(const AptNativeString &sFilename)
{
    APT_THREAD_NAMESPACE::AutoFutex lock(mFilesLock);
    AptFilePtr f = findFile(sFilename);

    if (!f)
    {
        return;
    }
    f->NotifyMovieClipLoader(LOAD_ERROR);

    AptFile::State state = f->GetState();
    if (state == AptFile::Queued)
    {
        // no need to doo anything as nothing is loaded yet, just remove it from mFiles list
        //  f->setState(AptFile::WaitingForData);
        // AptGetUserFuncs().pfnLoadAnimation(f->GetName().ConstRawPtr(), f);
        Invalidate(f.pData);
    }
    else if (state == AptFile::WaitingForData)
    {
        // if state is this then pfnLoadAnimation() is called so we need to call freeanimation.
        // but htis is really unknown state as Auxiliary library might have called async load operation and
        // it might not have completed so it has not called AptCompleteAsyncLoad so mUserData inside AptFile
        // might not be yet set. but check value of it and if it is not null then call pree animation on it.
        switch (f->GetFileType())
        {
        case AptFileType_Animation:
        {
            AptAnimationFile *animFile = AptAnimationFile::Cast(f);
            if (animFile->GetUserData())
            {
                AptGetUserFuncs().pfnFreeAnimation(animFile->GetUserData());
            }
            break;
        }

        case AptFileType_Image:
        {
            AptImageFile *textureFile = AptImageFile::Cast(f);
            if (textureFile->GetUserData())
            {
                AptGetUserFuncs().pfnFreeImage(textureFile->GetUserData());
            }
            break;
        }

        default:
            break;
        }
        Invalidate(f.pData);
    }
    else if ((state == AptFile::WaitingForImports) || (state == AptFile::Resolved) || (state == AptFile::Zombie))
    {
        if (f->GetFileType() == AptFileType_Animation)
        {
            AptAnimationFile *animFile = AptAnimationFile::Cast(f);
            // at this point we have to make sure we also unload all the imported animations.
            // but this could be dangerous as some other already playing animation might have also imported that animation.
            // so check if there is a item in mLinkerDataList with this AptFile object.
            // actually here we should go thru each and every item of imported section of already playing movies and
            // compare it with imported items in this animations and if there are any common then do not
            // remove them as those imported items are needed by already playing animations.
            // XXX - This code is not yet implemented as it might take a lot of time to walk thru all the
            // GetTarget()->GetLinker()->mLinkerDataList and check with each and every imported libraries.

            for (int j = 0; j < animFile->GetMainCharacter()->animation.nImports; j++)
            {
                AptFilePtr fTemp = findFile(AptNativeString(animFile->GetMainCharacter()->animation.aImports[j].szFile));
                if (fTemp)
                {
                    // add code to walk thru all animations in mLinkerDataList and import section of each one
                    if (!GetTargetSim()->GetLinker()->isFileImported(fTemp))
                    {
                        // just checking if it is loaded as a standalone animation, this check is actually not enough
                        // as it could be a library.
                        // its not linked by linker to some pCIH yet so ok to remove it.
                        // needs testing...can be dangerous.
                        CancelPreloadedAnimation(AptNativeString(animFile->GetMainCharacter()->animation.aImports[j].szFile));
                    }
                }
            }
        }
    }
    // delete this AptFile which will remove it from mFiles list and also call
    // animation.unresolve(mAptData); and pfnFreeAnimation. if state is waitingforimports or resolved.
    f.Reset();
}

void AptLinker::SwapOut(AptFilePtr f1, AptFilePtr f2)
{
    Notify(f2);
    LinkerDataPtr pLinkerData = *(findLinkerData(f1));
    AptCIH *pTarget           = pLinkerData->GetTarget();

    LinkerDataList::iterator pLinkerData2 = findLinkerData(pTarget);
    if (pLinkerData2 == mLinkerDataList.end())
    {
        mLinkerDataList.push_front(LinkerDataPtr(new AptLinkerThingy(f2, pTarget)));
    }
    else
    {
        mLinkerDataList.erase(pLinkerData2);
        mLinkerDataList.push_front(LinkerDataPtr(new AptLinkerThingy(f2, pTarget)));
    }
}

/******************************************************************************/
/**
    @brief  Find the spot in our Linker data where the given AptCIH exists

    @param  pCIH    AptCIH we're looking for

    @return         - Iterator in our linker list for where that AptCIH is
*/
/******************************************************************************/
AptLinker::LinkerDataList::iterator AptLinker::findLinkerData(AptCIH *pCIH)
{
    for (LinkerDataList::iterator i = mLinkerDataList.begin(); i != mLinkerDataList.end(); ++i)
    {
        if ((*i)->pTarget == pCIH)
            return i;
    }
    return mLinkerDataList.end();
}

/******************************************************************************/
/**
    @brief  Find the spot in our Linker data where the given AptFile exists

    @param  pFile   File we're looking for

    @return         - Iterator in our linker list for where that file is
*/
/******************************************************************************/
AptLinker::LinkerDataList::iterator AptLinker::findLinkerData(AptFilePtr pFile)
{
    for (LinkerDataList::iterator i = mLinkerDataList.begin(); i != mLinkerDataList.end(); ++i)
    {
        if ((*i)->mFile == pFile)
            return i;
    }
    return mLinkerDataList.end();
}

/******************************************************************************/
/**
    @brief  Check whether the given File is marked for import somewhere in our
            linker data list

    @param  pFile   File we're looking for

    @return         - True if it's imported
*/
/******************************************************************************/
bool AptLinker::isFileImported(AptFilePtr pFile)
{
    for (auto &i : mLinkerDataList)
    {
        AptAnimationFile *animFile = AptAnimationFile::Cast(i->mFile);
        // call function on AptFile that looks thru all imported libraries.
        if (animFile->isFileImported(pFile))
        {
            return true;
        }
    }
    return false;
}

void AptLinker::Update()
{

    GetTargetSim()->GetLoader()->Update();
    if (AptGetLib()->mSIPlayback.pSavedInputs && !AptGetLib()->mpSavedInputCheckpoints->CanLinkPendingFiles())
        return;
    int nOrigLength           = mLoadedFilesWaitingForLink.size();
    FilePtrVector::iterator i = mLoadedFilesWaitingForLink.begin();
    // Bingo .. New logic for updating iterator list

    for (LinkerDataList::iterator j = mLinkerDataList.begin(); j != mLinkerDataList.end(); j++)
    {
        LinkerDataPtr pLinkerData = *j;
        if (pLinkerData->GetTarget() && pLinkerData->GetFile()->GetState() == AptFile::Unloaded)
        {
            AptCIH *pTarget = pLinkerData->GetTarget();
            int32_t nTmpD   = pTarget->GetDepth();
            pTarget->SetCIHState(AptCIH::AptCIHState_Normal);
            pTarget->ClearCIH(false);
            if (pTarget->GetCIHState() == AptCIH::AptCIHState_Zombie)
            {
                pTarget = ConvertToZombie(pTarget);
                pTarget->GetCharacterInst()->SetDepth(nTmpD);
            }
            else
            {
                pTarget->SetCharacterInst(AptCharacterInst::CreateCharacterInst(NULL), true);
                pTarget->GetCharacterInst()->SetDepth(nTmpD);
                mLinkerDataList.erase(j);
            }
            j = mLinkerDataList.begin();
            if (mLinkerDataList.empty())
            {
                break;
            }
        }
    }

    while (i != mLoadedFilesWaitingForLink.end())
    {
        bool flag    = false;
        AptFilePtr f = *i;
        for (auto pLinkerData : mLinkerDataList)
        {
            if (pLinkerData->GetFile() == f && !pLinkerData->IsAttachedToMovie())
            {

                AptCIH *pTarget = pLinkerData->GetTarget();

                if (pTarget->GetCharacterInst()->GetNativeHash() != NULL)
                {
                    // Clean out the target if there was already a hash for this target
                    pTarget->ClearCIH(true);
                    pTarget->ForceCleanNativeHash();
                }

                // if this CIH was previously in the zombie state, we need to remove its potential placement in
                // the zombie vector before we change its state back to normal
                if (pTarget->GetCIHState() == AptCIH::AptCIHState_Zombie)
                {
                    AptRemoveCIHFromZombieVector(pTarget);
                }

                pTarget->SetCIHState(AptCIH::AptCIHState_Normal);

                // Link up the target character with an appropriate AptCharacter
                switch (f->GetFileType())
                {
                case AptFileType_Animation:
                {
                    AptAnimationFile *animFile     = AptAnimationFile::Cast(f);
                    AptCharacterAnimationInst *cai = new AptCharacterAnimationInst(animFile->GetMainCharacter(), f);
                    pTarget->SetCharacterInst(cai, true);

                    // If we haven't determined what global SWF version we're working with yet, for
                    // some reason we grab that from the first animation loaded.  Dunno why, seems odd.
                    // We also have a special case for filter data, because that's built into C++ for
                    // some reason
                    int SwfID = AptGetSwfVersion();
                    if (SwfID == 0 && animFile->GetUserData() != (void *)APT_FILTERS_FILE_USERDATA)
                    {
                        SwfID = cai->GetSwfVersion();
                        AptSetSwfVersion(SwfID);
                    }
                    break;
                }

                case AptFileType_Image:
                {
                    AptImageFile *textureFile = AptImageFile::Cast(f);
#if defined(APT_DECOUPLED_RENDERING)
                    if (textureFile && textureFile->GetCharacter()->m_pAnimFile == AptFilePtr(0))
                    {
                        // If we got into this state, that means that the character associated with
                        // this file does not have a reference to the file anymore, due to a release
                        // that happened at some point in the past.  The Character thinks it is in a
                        // state where it (and its file) is ready to be cleaned up, but the file didn't
                        // actually go away, yet.  Meanwhile, something else attempted to load the file,
                        // and now we're back to the state where we are trying to link the file again.
                        // So the character no longer knows what file it is associated with.  So here
                        // we are fixing the issue by setting the file pointer again
                        textureFile->GetCharacter()->m_pAnimFile = f;
                    }
#endif
                    AptCharacterInst *inst = new AptCharacterImageInst(textureFile->GetCharacter(), f);
                    pTarget->SetCharacterInst(inst, true);
                    break;
                }

                default:
                {
                    APT_FAIL("AptCharacter hookup must be implemented");
                    break;
                }
                }

                pTarget->setIsDefined(1);
                if (pTarget->GetCharacterInst()->IsSpriteInstBase())
                {
                    pTarget->GetSpriteInstBase()->mnFrame = -1;

                    // removeMovieClip followed closely by loadMovie fails
                    // This is added so that we make sure we execute initactions, every time we load the same file.
                    pTarget->ResetInitActions();

                    // added so that onLoad for animation will also be executed.
                    // this is part of FR 380 - Async load complete function
                    pTarget->GetSpriteInstBase()->mbJustLoaded = 1;
                }

                pTarget->SetDirtyState(true, false); // Set this sprite as dirty to notify its parents to tick it
                pTarget->tick();
                pTarget->SetDirtyState(true, true); // Set this sprite as dirty to notify its parents to tick it
                pLinkerData->SetAttachedToMovie(true);

                switch (f->GetFileType())
                {
                case AptFileType_Animation:
                {
                    // now call the callback function to let aux know that loading of animation is done.
                    // As implementing this is kept optional first check if callback function exits or not
                    if (AptGetUserFuncs().pfnLoadAnimationCompleted)
                    {
                        AptGetUserFuncs().pfnLoadAnimationCompleted(f->GetName().ConstRawPtr(), pTarget->GetInstanceName().ConstRawPtr());
                    }
                    break;
                }
                default:
                    // No complete load callback for other types at the moment
                    break;
                }

                if (AptGetLib()->mbSavedInputsEnabled)
                {
                    AptSavedInputRecordCheckpoint inputRecord;
                    const AptNativeString &sName = f->GetName();
                    int nLength                  = sName.Size();
                    inputRecord.nTick            = AptGetLib()->mnCurTick;
                    inputRecord.szBuf[0]         = SET_CHECKPOINT_INPUT(1);
                    strcpy(&inputRecord.szBuf[1], sName.ConstRawPtr());

                    nLength += (5 + 1); // 5 = sizeof(unit32_t) + sizeof(char) : 1 = '\0' char in string
#if defined(APT_SYSTEM_BIG_ENDIAN)
                    // Byte Align inputRecord
                    if (nLength & 0x03)
                    {
                        nLength += 4;
                        nLength &= ~0x03;
                    }
#endif
                    AptGetUserFuncs().pfnDebugAddSavedInput(&inputRecord, nLength);
                }


                // loadMovie() -> Assert: "Iterators not in same range"
                // Whenever new files are ready to be loaded, the debug iterator list needs to be updated
                // to take into account the newly loaded file.
                if (nOrigLength != mLoadedFilesWaitingForLink.size())
                {
                    i           = mLoadedFilesWaitingForLink.begin();
                    nOrigLength = mLoadedFilesWaitingForLink.size();
                    flag        = true;
                    break;
                }
            }
        }
        // The first frame of the clips has been executed. We can send the onLoadInit notification to the MovieClipLoader
        f->NotifyMovieClipLoader((int32_t)LOAD_INIT);
        if (!flag)
        {
            ++i;
        }
    }
    if (AptGetLib()->mSIPlayback.pSavedInputs)
        AptGetLib()->mpSavedInputCheckpoints->AllLinked();
    if (nOrigLength > 0)
    {
        FilePtrVector tmp = FilePtrVector(); // ngc-mw requires the temporary
        mLoadedFilesWaitingForLink.swap(tmp);
        GetTargetSim()->GetAnimationTarget()->RunActions();
    }
}

// ------------------------------------
void AptLinker::Notify(AptFilePtr f)
{
    for (auto &i : mLoadedFilesWaitingForLink)
    {
        if (i == f)
            return;
    }
    mLoadedFilesWaitingForLink.push_back(f);
}

AptCIH *AptLinker::ConvertToZombie(AptCIH *pCIH)
{
    if (pCIH->GetCIHState() == AptCIH::AptCIHState_Zombie)
    {
        LinkerDataList::iterator j = findLinkerData(pCIH);
        LinkerDataPtr pLinkerData  = *j;
        AptFile *pFileTemp         = pLinkerData->GetFile().Get();

        // If the file wasn't already a zombie, add it to the AptGetLib()->mpZombieVector to clean up in AptRelease.
        if (pFileTemp->GetState() != AptFile::Zombie)
        {
            mLinkerDataList.erase(j); // We have to erase it here, otherwise mThingy will keep track of the original pCIH pointer
            pFileTemp->setState(AptFile::Zombie);
        }
        AptCIH *pNewItem = new AptCIH(NULL, pCIH->GetDisplayListParent()); // Create a replacement AptCIH

        // Copy some info over to the new guy
        AptCIH *pCIHParent = pCIH->GetDisplayListParent();
        if (pCIHParent) // this case is for sprites
        {
            pCIHParent->ReplaceZombieChild(pNewItem, pCIH);
            APT_INC(pCIH);
            pCIH->setIsDefined(1); // removeObject call sets pCIH to undefined, we need to reset this to true
        }
        else // else we have an animation loaded into a level
        {
            pNewItem->GetCharacterInst()->CopyRenderDataFrom(pCIH->GetCharacterInst());
            pNewItem->GetCharacterInst()->SetDepth(pCIH->GetDepth());
            // Need to clean out hashes, but not delete! for both!
            AptCIH *pPrev = pCIH->GetDisplayListPrevious();
            GetTargetSim()->GetAnimationTarget()->GetDisplayList()->removeObject(pCIH); // Remove Old AptCIH pointer from displayList
            APT_INC(pCIH);
            pCIH->setIsDefined(1); // removeObject call sets pCIH to undefined, we need to reset this to true
            GetTargetSim()->GetAnimationTarget()->GetDisplayList()->pState->insert(pPrev, pNewItem);
        }

        APT_INC(pNewItem);
        pCIH->setGCRoot(1);

        AptGC::ReplaceReferences(pCIH, pNewItem);
#if !defined(DO_COVERAGE)
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "####### Swaping zombie[%i : %x] with[%i : %x]\n", pCIH->getRefCount(), pCIH, pNewItem->getRefCount(), pNewItem);
#endif
        return pNewItem;
    }
    else
    {
        pCIH->SetCIHState(AptCIH::AptCIHState_Unloaded);
    }
    return pCIH;
}

// ------------------------------------

void AptLinker::Load(const AptNativeString &filename, const AptNativeString &sTarget, AptMovieClipLoader *pLoader)
{
    AptValue *pTarget = gAptActionInterpreter.getVariable(_AptGetAnimationAtLevel(0), NULL, &sTarget);
    if (pTarget == 0 || !pTarget->isCIH())
    {
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEBUG_LVL, "target ('%s') for '%s' is undefined; not loading\n", sTarget.ConstRawPtr(), filename.ConstRawPtr());
        return;
    }

    // Figure out the type of file based on the file extension - SWFs or empty strings
    // are Animations, all else are Textures
    AptNativeString adjustedFilename = filename;
    AptFileType fileType             = (adjustedFilename.EndWithRemoveIgnoreCase(".swf") || adjustedFilename.IsEmpty()) ? AptFileType_Animation : AptFileType_Image;

    AptCIH *pCIH = pTarget->c_cih();

    AptFilePtr f;

    if (adjustedFilename.IsEmpty())
    {
        /* nothing */
    }
    else
    {
        f = GetTargetSim()->GetLoader()->IsLoaded(adjustedFilename);

        if (f.pData == NULL)
        {
            // It doesn't exist or it's partially loaded
            f = GetTargetSim()->GetLoader()->Load(adjustedFilename, fileType);
            f->SetMovieClipLoader(pLoader);
            if (f->GetState() == AptFile::Unloaded)
            {
                f->setState(f->GetPrevState());
                if (f->GetState() == AptFile::Resolved)
                {
                    Notify(f);
                }
            }
        }
        else
        {
            Notify(f);
            // This file was already loaded: notify to the MovieClipLoader if any
            if (pLoader)
            {
                pLoader->NotifyListeners(adjustedFilename, LOAD_COMPLETE);
            }
        }
    }

#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)
    extern bool gbDumpOnLoadAnimation;
    if (gbDumpOnLoadAnimation)
    {
        extern void PrintObjectMapXML(const char *);
        PrintObjectMapXML(adjustedFilename.c_str());
    }
#endif

    APT_ASSERT(pCIH->GetCIHState() != AptCIH::AptCIHState_Zombie);

    pCIH->setIsDefined(1);
    pCIH->SetCIHState(AptCIH::AptCIHState_Unloaded);
    pCIH->SetDirtyState(false, false); // Set this sprite to not dirty so it won't waste time ticking it next frame

    LinkerDataList::iterator pLinkerData = findLinkerData(pCIH);

    if (pLinkerData != mLinkerDataList.end())
    {
        mLinkerDataList.erase(pLinkerData);
    }

    if (f.pData != NULL)
    {
        mLinkerDataList.push_front(LinkerDataPtr(new AptLinkerThingy(f, pCIH)));
    }
    else
    {
        if (pCIH->IsLevelInst())
        {
            // Unloading a level... nothing todo!
        }
        else
        {
            AptFile *file = NULL;
            switch (fileType)
            {
            case AptFileType_Animation:
                file = new AptAnimationFile(adjustedFilename);
                break;
            case AptFileType_Image:
                file = new AptImageFile(adjustedFilename);
                break;
            default:
                APT_FAIL("Unsupported file type");
                break;
            }
            if (!pCIH->IsAnimationInst())
            {
                AptFilePtr ret(file);
                f = ret;
                f->setState(AptFile::Unloaded);
                mLinkerDataList.push_front(LinkerDataPtr(new AptLinkerThingy(f, pCIH)));
            }
            else
            {
                // 04/05/2007   EATech 108089: Fixed MovieClip.unloadMovie.

                // create a new AptFile and set its state as Unloaded
                AptFilePtr filePtr(file);
                filePtr->setState(AptFile::Unloaded);

                // Push the new AptFilePtr onto the linker list so AptLinker::Update
                // will process pCIH.
                mLinkerDataList.push_front(LinkerDataPtr(new AptLinkerThingy(filePtr, pCIH)));
            }
        }
    }
}

// ------------------------------------
void AptLinker::CancelLoad(AptCIH *pCIH)
{
    LinkerDataList::iterator i = findLinkerData(pCIH);
    if (i != mLinkerDataList.end())
    {
        mLinkerDataList.erase(i);
    }
}

void AptLinker::PrintLoadedFiles(bool bPrintImports, bool bPrintExports)
{
#if defined(APT_DEBUG) || defined(APT_PRINT_LOADED_SWFS)
    int nTotalFiles = 0;
    for (auto &i : mLinkerDataList)
    {
        AptFilePtr filePtr = i->GetFile();

        switch (filePtr->GetFileType())
        {
        case AptFileType_Animation:
        {
            AptAnimationFile *animFile = AptAnimationFile::Cast(filePtr);
            int nImports               = (animFile->GetState() == AptFile::Resolved && animFile->GetMainCharacter()) ? animFile->GetMainCharacter()->animation.nImports : 0;
            int nExports               = (animFile->GetState() == AptFile::Resolved && animFile->GetMainCharacter()) ? animFile->GetMainCharacter()->animation.nExports : 0;
            char cEndChar              = ' ';
            if (nImports == 0 && nExports == 0)
                cEndChar = '/';

            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<SWFFile name=\"%s\" state=\"%d\" target=\"%s\" imports=\"%d\" exports=\"%d\" %c>\n",
                           animFile->GetName().c_str(),
                           animFile->GetState(),
                           i->GetTarget() ? i->GetTarget()->GetInstanceName().c_str() : "",
                           nImports,
                           nExports,
                           cEndChar);

            if (nImports)
            {
                for (int j = 0; j < nImports; j++)
                {
                    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "\t<imports name=\"%s\"/>\n", animFile->GetMainCharacter()->animation.aImports[j].szFile);
                }
            }

            if (nExports)
            {
                for (int j = 0; j < nExports; j++)
                {
                    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "\t<exports name=\"%s\"/>\n", animFile->GetMainCharacter()->animation.aExports[j].szName);
                }
            }
            if (nExports > 0 || nExports > 0)
            {
                APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "</SWFFile>\n");
            }
            break;
        }

        case AptFileType_Image:
        {
            AptImageFile *textureFile = AptImageFile::Cast(filePtr);
            APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<TextureFile name=\"%s\" state=\"%d\" target=\"%s\" />\n",
                           textureFile->GetName().c_str(),
                           textureFile->GetState(),
                           i->GetTarget() ? i->GetTarget()->GetInstanceName().c_str() : "");
            break;
        }
        }

        nTotalFiles++;
    }
    APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "<total SWF_files=\"%d\"/>\n", nTotalFiles);
#endif
}

/******************************************************************************/
/**
    @brief  Cast an AptFilePtr to an AptAnimationFile*

    @param  ptr     AptFilePtr we happen to know is an AptAnimationFile under
                    the covers

    @return         - AptAnimationFile* from that AptFilePtr
*/
/******************************************************************************/
AptAnimationFile *AptAnimationFile::Cast(AptFilePtr ptr)
{
    // Must be either NULL or, if it exists, actually be an Animation file. If this
    // is not true, there's a bug in Apt somewhere where code that's expecting an
    // Animation is somehow working on a file that can't possibly be an animation
    APT_ASSERT(NULL == ptr.Get() || ptr->GetFileType() == AptFileType_Animation);
    return static_cast<AptAnimationFile *>(ptr.Get());
}

/******************************************************************************/
/**
    @brief  Cast an AptFilePtr to an AptImageFile*

    @param  ptr     AptFilePtr we happen to know is an AptImageFile under
                    the covers

    @return         - AptImageFile* from that AptFilePtr
*/
/******************************************************************************/
AptImageFile *AptImageFile::Cast(AptFilePtr ptr)
{
    // Must be either NULL or, if it exists, actually be an Image file. If this
    // is not true, there's a bug in Apt somewhere where code that's expecting an
    // Image is somehow working on a file that can't possibly be an image
    APT_ASSERT(NULL == ptr.Get() || ptr->GetFileType() == AptFileType_Image);
    return static_cast<AptImageFile *>(ptr.Get());
}

// ------------------------------------
#if defined(APT_DEBUG) || defined(DO_COVERAGE)
void AptSharedPtrSetFileSize(AptFile *p, int nSize) { p->SetFileSize(nSize); }
int AptSharedPtrGetFileSize(AptFile *p) { return p->GetFileSize(); }
#endif

// 9/28/07 a note on the use of atomic ints in the following ref-counting -
// this is necessary not because the render thread modifies the ref-count, because it doesn't
// but because the football games do weird things with moving the sim thread around

int AptSharedPtrIncRef(AptFile *p)
{
    return ++p->mRefCount;
}

inline int AptSharedPtrDecRef(AptFile *p)
{
    return --p->mRefCount;
}

inline void AptSharedPtrDelete(AptFile *p)
{
    AptRenderList::Destroy(p);
}

void AptSharedPtrDispose(AptFile *p)
{
    if (p)
    {
        if (AptSharedPtrDecRef(p
#if defined(REFCOUNT_DEBUGGING)
                               ,
                               this
#else
        /* nothing */
#endif
                               ) == 0)
        {
            AptSharedPtrDelete(p);
        }
    }
}

int AptSharedPtrIncRef(AptLinkerThingy *p)
{
    return ++p->mRefCount;
}

inline int AptSharedPtrDecRef(AptLinkerThingy *p)
{
    return --p->mRefCount;
}

inline void AptSharedPtrDelete(AptLinkerThingy *p)
{
    delete p;
}

void AptSharedPtrDispose(AptLinkerThingy *p)
{
    if (p)
    {
        if (AptSharedPtrDecRef(p
#if defined(REFCOUNT_DEBUGGING)
                               ,
                               this
#else
        /* nothing */
#endif
                               ) == 0)
        {
            AptSharedPtrDelete(p);
        }
    }
}

// this is an icky workaround for not having functors
void GlobalNotificationFunction(AptFilePtr f)
{

    GetTarget()->GetLinker()->Notify(f);
    if (AptGetLib()->mSIPlayback.pSavedInputs)
    {
        AptGetLib()->mpSavedInputCheckpoints->FileLoaded(f->GetName());
    }
}
