#pragma once
#include <list>
#include <vector>

#include "Apt.h"
#include "_AptThread.h"

struct AptCharacter;

#include APT_INC_THREAD_FUTEX_H
#include APT_INC_THREAD_ATOMIC_H

/**
 * Minimal std allocator over APT_MALLOC_BLOCK/APT_FREE_BLOCK, so container nodes
 * keep going through the game-provided Apt allocator (and its leak/metrics
 * reporting), as the old CommonSense SingleList did.
 */
template <class T>
struct AptStlAllocator
{
    using value_type = T;

    AptStlAllocator() = default;
    template <class U>
    AptStlAllocator(const AptStlAllocator<U> &) {}

    T *allocate(size_t n)
    {
        return (T *)APT_MALLOC_BLOCK(n * sizeof(T));
    }
    void deallocate(T *p, size_t n)
    {
        APT_FREE_BLOCK(p, n * sizeof(T));
    }

    template <class U>
    bool operator==(const AptStlAllocator<U> &) const { return true; }
    template <class U>
    bool operator!=(const AptStlAllocator<U> &) const { return false; }
};

using FilePtrVector = std::vector<AptFilePtr, AptStlAllocator<AptFilePtr>>;
using RawFileList   = std::list<AptFile *, AptStlAllocator<AptFile *>>;

enum AptFileType
{
    AptFileType_Animation,
    AptFileType_Image,
};

struct AptLoader
{
  private:
    RawFileList mFiles; // TODO need to enforce that there's only one file with a given name in this list
    APT_THREAD_NAMESPACE::Futex mFilesLock;

    AptLoader(const AptLoader &);
    AptLoader &operator=(const AptLoader &);
    void notify(AptFilePtr f);

  public:
    AptLoader() {};
    ~AptLoader();

    void Invalidate(AptFile *pFile);

    AptFilePtr findFile(const AptNativeString &sFilename);

    AptFilePtr IsLoaded(const AptNativeString &sFilename);
    AptFilePtr Load(const AptNativeString &sFilename, AptFileType fileType);

    bool AllImportsAvailable(AptFilePtr f);

    /** @brief move all files in the list as far as possible through their states */
    void Update();

    void CompleteAnimationLoad(AptFilePtr f, void *pData, void *pConstTable, void *pUser);
    void CompleteImageLoad(AptFilePtr f, AptAssetTexture texture, int width, int height, void *userData);

    void GetFileVector(AptFilePtr *aFilePtrs, int nMaxSize);

    void CancelPreloadedAnimation(const AptNativeString &sFilename);
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines
};

struct AptSharedPtrRefCount
{
    APT_THREAD_NAMESPACE::AtomicInt32 mRefCount;
    AptSharedPtrRefCount() : mRefCount(0) {}

    APT_NEW_DELETE_OPERATORS
    // Added Metric defines
};

/**
    @brief The Zombie state was added to keep track of files that are being held onto
    after they were unloaded by Apt, because external function references into
    them were still live.
*/
struct AptFile : public AptSharedPtrRefCount
{
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptFile(const AptNativeString &name) :
#if defined(APT_DEBUG) || defined(DO_COVERAGE) || defined(APT_ENABLE_ZOMBIE_OUTPUT)
    // this was not being initialized
#endif
                                           mName(name),

                                           mMovieClipLoader(NULL)
    {
    }

    virtual ~AptFile();

    enum State
    {
        Invalid,
        Queued,
        WaitingForData,
        WaitingForImports,
        Resolved,
        Zombie,
        Unloaded
    };

    virtual AptFileType GetFileType() const = 0;
    const AptNativeString &GetName() const { return mName; }
    State GetState() const { return mState; }
    State GetPrevState() const { return mPrevState; }

    void NotifyMovieClipLoader(int32_t notification);
    void SetMovieClipLoader(AptMovieClipLoader *pLoader);

    void setState(State s)
    {
        mPrevState = mState;
        mState     = s;
    } // moved to public so we can set the ZOMBIE flag on the file

    friend struct AptLoader;

#if defined(APT_DEBUG) || defined(DO_COVERAGE) || defined(APT_ENABLE_ZOMBIE_OUTPUT)
    void SetFileSize(int nSize)
    {
        nFileSize = nSize;
    }
    int GetFileSize()
    {
        return nFileSize;
    }
#endif

  protected:
    AptMovieClipLoader *GetMovieClipLoader() const;

#if defined(APT_DEBUG) || defined(DO_COVERAGE) || defined(APT_ENABLE_ZOMBIE_OUTPUT)
    int nFileSize{0};
#endif
    AptNativeString mName;
    State mState{Queued};
    State mPrevState{Queued};
    AptMovieClipLoader *mMovieClipLoader; // The loader that asked for this file. Removed after CompleteLoad
};

/** @brief Apt File that wraps a SWF */
struct AptAnimationFile : public AptFile
{
    APT_NEW_DELETE_OPERATORS

    AptAnimationFile(const AptNativeString &name)
        : AptFile(name), mAptData(NULL), mCharacter(NULL), mUserData(NULL), mConvertedAptData(NULL)

    {
    }

    virtual ~AptAnimationFile();

    AptCharacter *FindExport(const char *szName);

    bool isFileImported(AptFilePtr pFile);

    /** @brief Cast an AptFilePtr to an AptAnimationFile */
    static AptAnimationFile *Cast(AptFilePtr ptr);

    virtual AptFileType GetFileType() const { return AptFileType_Animation; }
    AptCharacter *GetMainCharacter() const { return mCharacter; }
    void *GetUserData() const { return mUserData; }
    void *GetAptData() const { return mAptData; }
    char *GetTimeStamp() { return mTimeStamp; }
    int GetTimeStampSize() { return sizeof(mTimeStamp); }

    friend struct AptLoader;

  private:
    void setDataPointers(void *aptData, AptCharacter *mainCharacter, void *userData)
    {
        mAptData   = aptData;
        mCharacter = mainCharacter;
        mUserData  = userData;
    }

    /**
        @brief Takes ownership of a native-layout buffer produced by AptConvertFile32 when the
        file on disk was built for a different pointer size; freed in the destructor
        after the animation is unresolved. mAptData aliases this buffer in that case.
    */
    void setConvertedAptData(void *convertedData, size_t convertedSize)
    {
        mConvertedAptData = convertedData;
        mConvertedAptSize = convertedSize;
    }

    void *mAptData;
    AptCharacter *mCharacter;
    void *mUserData;
    void *mConvertedAptData;
    size_t mConvertedAptSize{0};
    char mTimeStamp[16];
};

/** @brief Apt file representing an Image */
struct AptImageFile : public AptFile
{
    APT_NEW_DELETE_OPERATORS

    AptImageFile(const AptNativeString &name)
        : AptFile(name), mTexture(NULL), mCharacter(NULL),
          mUserData(NULL)
    {
    }

    virtual ~AptImageFile();

    /** @brief Cast an AptFilePtr to an AptImageFile */
    static AptImageFile *Cast(AptFilePtr ptr);
    virtual AptFileType GetFileType() const { return AptFileType_Image; }
    AptCharacter *GetCharacter() const { return mCharacter; }
    void *GetUserData() const { return mUserData; }
    int GetWidth() const { return mWidth; }
    int GetHeight() const { return mHeight; }

    void SetData(AptAssetTexture texture, int width, int height, void *userData);

  private:
    AptAssetTexture mTexture;
    AptCharacter *mCharacter;
    int mWidth{0};
    int mHeight{0};
    void *mUserData;
};

struct AptLinkerThingy : AptSharedPtrRefCount
{
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

    AptFilePtr mFile;
    AptCIH *pTarget;
    bool mAttachedToMovie{false};

    AptFilePtr GetFile() const { return mFile; }
    AptCIH *GetTarget() const { return pTarget; }

    bool IsAttachedToMovie() const { return mAttachedToMovie; }
    void SetAttachedToMovie(bool attached) { mAttachedToMovie = attached; }

    AptLinkerThingy(AptFilePtr file, AptCIH *target) : mFile(file), pTarget(target) {}
};

struct AptLinker
{
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines

    using LinkerDataPtr  = AptSharedPtr<AptLinkerThingy>;
    using LinkerDataList = std::list<LinkerDataPtr, AptStlAllocator<LinkerDataPtr>>;
    LinkerDataList mLinkerDataList;

    FilePtrVector mLoadedFilesWaitingForLink;

  private:
    LinkerDataList::iterator findLinkerData(AptCIH *pCIH);

  public:
    void PrintLoadedFiles(bool bPrintImports = false, bool bPrintExports = false);
    AptCIH *ConvertToZombie(AptCIH *pCIH);

    void SwapOut(AptFilePtr f1, AptFilePtr f2);
    LinkerDataList::iterator findLinkerData(AptFilePtr pFile);
    bool isFileImported(AptFilePtr pFile);

    AptLinker() {}

    void Update();
    void Notify(AptFilePtr f);
    void Load(const AptNativeString &sFilename, const AptNativeString &sTarget, AptMovieClipLoader *pLoader = NULL);
    void CancelLoad(AptCIH *pCIH);
};

struct AptFileSavedInputState
{
    enum State
    {
        Invalid,
        ExpectedBySavedInputButUnavailable,
        LoadedButNotAtCheckpoint,
        ReadyToBeLinked,
        Linked,
    };

    State GetState() const { return mState; }
    void SetState(State state) { mState = state; }
    const AptNativeString &GetName() const { return mName; }

    AptFileSavedInputState(const AptNativeString &name, State state = Invalid) : mName(name), mState(state) {}
    AptFileSavedInputState() : mState(Invalid) {}

  private:
    AptNativeString mName;
    State mState;
};

using FileSavedInputStateVector = std::vector<AptFileSavedInputState, AptStlAllocator<AptFileSavedInputState>>;
struct AptSavedInputCheckpoints
{
    APT_NEW_DELETE_OPERATORS
    // Added Metric defines
  private:
    FileSavedInputStateVector mPending;

    void updateState(const AptNativeString &name, AptFileSavedInputState::State lookFor, AptFileSavedInputState::State setTo, AptFileSavedInputState::State ifNotFound);
    bool allStatesAre1(AptFileSavedInputState::State state0);
    bool allStatesAre2(AptFileSavedInputState::State state0, AptFileSavedInputState::State state1);
    bool allStatesAre3(AptFileSavedInputState::State state0, AptFileSavedInputState::State state1, AptFileSavedInputState::State state2);

  public:
    void Checkpoint(const AptNativeString &s);
    void FileLoaded(const AptNativeString &s);
    void AllLinked();
    bool CanLinkPendingFiles();
    bool CanContinueSavedInputs();
};
