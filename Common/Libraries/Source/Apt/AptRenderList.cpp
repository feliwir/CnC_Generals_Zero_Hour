#include "Apt.h"
#include "AptRenderList.h"
#include "AptRenderItem.h"
#include "AptRenderableGeometry.h"
#include "AptRenderableString.h"
#include "AptRenderableCustomControl.h"

#include <string.h>

#include APT_INC_THREAD_H
#include APT_INC_THREAD_MUTEX_H

#if (APT_CURR_DEBUG_MSG_LVL >= APT_DEBUG_MSG_ASSERT_LVL)
#include <string>
#endif

// The shrinking behavior of the AptRenderList will fragment memory.
// Disabling this behavior will simply keep the AptRenderElement lists
// at a highwater mark size, which is likely to be safer than fragmenting
// all over the place.  Left the code surrounding shrinking in, in case
// someone disagrees with this and wants it back.
#if !defined(APT_RENDER_LIST_ALLOW_SHRINKING)
#define APT_RENDER_LIST_ALLOW_SHRINKING 0
#endif

using AptRenderListLock     = APT_THREAD_NAMESPACE::Futex;
using AptRenderListAutoLock = APT_THREAD_NAMESPACE::AutoFutex;

int nRenderableGeometries     = 0,
    nRenderableCustomControls = 0,
    nRenderableStrings        = 0;

class AptRenderListArray
{
  public:
    APT_RI_NEW_DELETE_OPERATORS

    void **first{0},
        **last{0};
    unsigned capacity{0};

    AptRenderListArray(unsigned initCapacity = 0)
    {
        Reserve(initCapacity);
    }
    ~AptRenderListArray()
    {
        Clear();
    }
    unsigned Size() const
    {
        return (unsigned)(last - first);
    }
    void Clear()
    {
        if (first)
        {
            APT_FREE_BLOCK(first, capacity * sizeof(void *));
        }
        first = last = 0;
        capacity     = 0;
    }
    void SanityCheck()
    {
#if (APT_CURR_DEBUG_MSG_LVL >= APT_DEBUG_MSG_ASSERT_LVL)
        bool isSane = (last && first) || (!last && !first && capacity == 0);
        if (!isSane)
        {
            char message[128];
            SNPRINTF(message, sizeof(message), "AptRenderListArray is in a bad state: [0x%p], [0x%p], [%d]", first, last, capacity);
            APT_ASSERTM(isSane, message);
        }
#endif
    }
    void **Find(const void *q, void **start = 0)
    {
        if (!start)
            start = first;
        for (void **p = first, **e = last; p != e; ++p)
        {
            APT_ASSERT(p);
            if (*p == q)
                return p;
        }
        return 0;
    }
    void Reserve(unsigned count)
    {
        if (count > capacity)
        {
            // resize
            unsigned uPrevCapacity = capacity;
            capacity               = count;
            void **nl              = (void **)APT_MALLOC_BLOCK(capacity * sizeof(void *));
            APT_ASSERT(nl);
            size_t sz = last - first;
            if (first)
            {
                if (sz)
                    memcpy(nl, first, sz * sizeof(void *));
                APT_FREE_BLOCK(first, uPrevCapacity * sizeof(void *));
            }
            first = nl;
            last  = first + sz;
            SanityCheck();
        }
    }
    void Add(void *p)
    {
        if (last >= first + capacity)                // reallocate
            Reserve(capacity + (capacity >> 1) + 2); // capacity *= 1.5
        APT_ASSERT(last && first);
        *last++ = p;
        SanityCheck();
    }
    void Remove(const void *p)
    {
        SanityCheck();
        void **current   = first;
        void **newEnd    = current;
        void **cachedEnd = last; // make a local variable to avoid reloading value while looping
        while (current != cachedEnd)
        {
            if (*current != p)
            { // keep this in the new list
                *newEnd++ = *current++;
            }
            else
            { // omit this from the new list
                current++;
            }
        }
        last = newEnd;
    }
};

class AptRenderElement;

class AptRenderBuffer
{
    enum
    {
        SHRINK_INTERVAL = 10,
        SHRINK_MINIMUM  = 4,
        GROWTH_AMOUNT   = 100
    };
    static const float SHRINK_FACTOR;

  public:
    APT_RI_NEW_DELETE_OPERATORS

    AptRenderBuffer();
    ~AptRenderBuffer();

    void Add(const AptRenderInfo &info, const AptRenderItem *pRenderItem, IAptRenderable *pRenderable, AptRenderableGeometry *pBounds);
    void Close();
    bool IsClosed() const;
    bool IsRendering() const;
    void Clear();
    void Render(unsigned uLevelMask);
    void StartRender();
    void EndRender();
    void UnsafeClear();
    bool Contains(const AptFile *pFile) const;

    void Initialize();
    void Shutdown();

    unsigned GetBytesSize() const; // for reporting memory metrics only

  private:
    // AptRenderBuffer can alternate allocations between the default Non-GC pool
    // and a temporary allocation system.  It will only actually make use of
    // the alternate system if APT_USE_TEMPORARY_ALLOCATORS is 1.
    enum AllocLocation
    {
        ALLOC_DEFAULT,
#if APT_USE_TEMPORARY_ALLOCATORS
        ALLOC_ALTERNATE,
#endif
    };

    void Shrink(unsigned uSize);

    AptRenderElement *AllocList(unsigned capacity, AllocLocation allocLoc);
    void FreeList(AptRenderElement *list, unsigned capacity, AllocLocation allocLoc);

    AptRenderListLock mLock;
    AptRenderElement *mList{0};
    unsigned muSize{0},
        muCapacity{SHRINK_MINIMUM};
    volatile bool mbClosed{false},
        mbRendering{false};
    AllocLocation mAllocLocation{ALLOC_DEFAULT};

    // logic to delay shrinking of lists
    static unsigned suSizes[];
    static unsigned suCloseCount;
    static unsigned suShrinkThreshold;

    static void ClearShrinkThreshold();
    static void SetCloseSize(unsigned uSize);
};

void AptRenderBuffer::ClearShrinkThreshold()
{
    memset(suSizes, 0, SHRINK_INTERVAL * sizeof(unsigned));
    suCloseCount      = 0;
    suShrinkThreshold = SHRINK_MINIMUM;
}

void AptRenderBuffer::SetCloseSize(unsigned uSize)
{
    suSizes[suCloseCount++ % SHRINK_INTERVAL] = uSize;
    if (!(suCloseCount % SHRINK_INTERVAL))
    {
        unsigned uMax = SHRINK_MINIMUM;                // keep a minimum number of items, regardless
        for (unsigned i = 0; i < SHRINK_INTERVAL; i++) // get the max of suSizes[]
            if (suSizes[i] > uMax)
                uMax = suSizes[i];
        suShrinkThreshold = uMax;
    }
}

const float AptRenderBuffer::SHRINK_FACTOR = 1.5f; // change this as you please

unsigned AptRenderBuffer::suSizes[SHRINK_INTERVAL] = {0};
unsigned AptRenderBuffer::suCloseCount             = 0;
unsigned AptRenderBuffer::suShrinkThreshold        = SHRINK_MINIMUM;

class AptRenderListSet
{
  public:
    APT_RI_NEW_DELETE_OPERATORS

    enum
    {
        LIST_COUNT = 3
    };

    AptRenderListSet();
    ~AptRenderListSet();

    bool Open();
    void Close();
    void Add(const AptRenderInfo &info, const AptRenderItem *pRenderItem, IAptRenderable *pRenderable, AptRenderableGeometry *pBounds);
    void Render(unsigned uLevelMask);
    void StartRender();
    void EndRender();
    void Stop();

    void Destroy(AptFile *pFile);

    void Initialize();
    void Shutdown();

  private:
    AptRenderBuffer &GetList();
    AptRenderBuffer &GetNextList();
    AptRenderBuffer &GetRenderList();

    bool Contains(const AptFile *pFile) const;
    bool Find(const AptFile *pFile);
    void ReleaseFileQueue();
    bool AttemptDestroy(AptFile *pFile);

    AptRenderBuffer mLists[LIST_COUNT];
    AptRenderListLock mQueueLock;
    unsigned muCurrent{0};
    volatile unsigned muComplete{0},
        muRender{0};
    volatile bool mbStop{false};
    AptRenderListArray mFileFreeQueue;
};

static void CallLegacyTransformCallbacks(const AptRenderInfo &info_aligned)
{
    // Note we intend to deprecate pfnSetColourTransform and pfnSetVertexMatrix.
    //     This is just to suppport backwards compatibility.

    if (AptGetUserFuncs().pfnSetColourTransform)
    {
        alignas(16) AptCXForm colorxform;
        colorxform.scale     = info_aligned.mTransform.vColorMul4;
        colorxform.translate = info_aligned.mTransform.vColorAdd4;
        AptGetUserFuncs().pfnSetColourTransform(&colorxform);
    }
    if (AptGetUserFuncs().pfnSetVertexMatrix)
    {
#if defined(APT_3D)
        AptGetUserFuncs().pfnSetVertexMatrix(const_cast<AptMath::Mat44T *>(&info_aligned.mTransform.Pos44));
#else
        static AptMatrix m;
        AptMath::MatConvert(&m, &info_aligned.mTransform.Pos44);
        AptGetUserFuncs().pfnSetVertexMatrix(&m);
#endif // APT_3D
    }
}

class AptRenderElement
{
  public:
    APT_RI_NEW_DELETE_OPERATORS

    AptRenderElement(const AptRenderInfo &info, const AptRenderItem *pRenderItem, IAptRenderable *pRenderable, AptRenderableGeometry *pGeometry)
        : mInfo(info)
    {
        mpRenderable = SafeAddRef(pRenderable);
        mpGeometry   = SafeAddRef(pGeometry);
        APT_RI_INCSAFE(pRenderItem, this, "AptRenderElement");
        mpRenderItem = pRenderItem;
    }
    AptRenderElement(const AptRenderElement &u) : mpGeometry(0), mpRenderable(0), mpRenderItem(0)
    {
        CopyFrom(u);
    }
    AptRenderElement &operator=(const AptRenderElement &u)
    {
        if (this != &u)
            CopyFrom(u);
        return *this;
    }
    ~AptRenderElement()
    {
        mpRenderable = SafeRelease(mpRenderable);
        mpGeometry   = SafeRelease(mpGeometry);
        APT_RI_DECSAFE(mpRenderItem, this, "AptRenderElement");
        mpRenderItem = 0;
    }

    void RenderPrepare(unsigned uLevelMask) const
    {
        if (mpRenderable && ((1 << mInfo.mnLevel) & uLevelMask))
        {
            mpRenderable->RenderPrepare();
        }
    }

    void Render(unsigned uLevelMask) const
    {
        if (mpRenderItem)
        {
            // this will take care of loading all the textures before rendering from the SWF file in which
            // mpRenderItem->mpCharacter is present.
            mpRenderItem->LoadResourcesFromCharacter();
        }

        if (mpRenderable == NULL && mpGeometry == NULL)
        {
            // the data we pass down to AptAux is aligned to 16 bytes
            alignas(16) AptRenderInfo ri;
            ri = mInfo;

            APT_ASSERT(mInfo.mpEffect); // effect should be present in this case
            // that means this is a node related to UIEffect, now check if maskoperation is add or subtract
            // which determines if we want to push an UIEffect or pop it.
            if (mInfo.meMaskOper == (AptMaskRenderOperation)AptEffectOperation_Apply)
            {
                // call callback function to push the UIEffect
                APT_ASSERT(AptGetUserFuncs().pfnPushEffect);
                AptGetUserFuncs().pfnPushEffect(ri);
            }
            else
            {
                APT_ASSERT(mInfo.meMaskOper == (AptMaskRenderOperation)AptEffectOperation_Unapply); // it has to be pop operation in this case
                // call callback function to pop the UIEffect
                APT_ASSERT(AptGetUserFuncs().pfnPopEffect);
                AptGetUserFuncs().pfnPopEffect(ri);
            }
        }

        if (mpRenderable && ((1 << mInfo.mnLevel) & uLevelMask))
        {
            // the data we pass down to AptAux is aligned to 16 bytes
            alignas(16) AptRenderInfo ri;
            ri = mInfo;
            // I'm not very happy to put these callbacks here
            // for fear that might slow down the render thread.  These are
            // legacy callbacks and it would be nice to be rid of them.
            CallLegacyTransformCallbacks(ri);
            mpRenderable->Render(ri, mpGeometry);
        }
    }
    bool Contains(const AptFile *pFile) const
    {
        return ((mpRenderable && mpRenderable->GetFile() == pFile) ||
                (mpGeometry && mpGeometry->GetFile() == pFile));
    }

    void DestroyEffect()
    {
        if (mpRenderable == NULL && mpGeometry == NULL && mInfo.meMaskOper == (AptMaskRenderOperation)AptEffectOperation_Unapply)
        {
            // that means this is a pop UI effect AptRenderElement, so we call callback function to destroy the UIEffect
            APT_ASSERT(AptGetUserFuncs().pfnDestroyEffect);
            APT_ASSERT(mInfo.mpEffect);
            AptGetUserFuncs().pfnDestroyEffect(mInfo.mpEffect);
        }
    }

    void Reset()
    {
        mInfo        = AptRenderInfo();
        mpGeometry   = 0;
        mpRenderable = 0;
        mpRenderItem = 0;
    }

  private:
    void CopyFrom(const AptRenderElement &u);

    AptRenderInfo mInfo;
    AptRenderableGeometry *mpGeometry{0};
    IAptRenderable *mpRenderable{0};
    const AptRenderItem *mpRenderItem{0};
};

// Due to Technical Limitations with the Apt Memory System Not Allowing Memory Alignment to be Honored
//  switching to Futex Usage Requires us to spawn objects in the Global Space. As such...
//  new Initialize() and Shutdown() functions were added to emulate Construction and Destruction
//  of this object to allow it to reset to its initial state.

// now we declare any objects that we want of above types.
AptRenderListLock gRenderListLock;
AptRenderListSet gRenderListSet;
AptRenderListSet *volatile gpRenderListSet = NULL;

// getting some errors with first 2 ones somehow - TODO fix them later.

void AptRenderElement::CopyFrom(const AptRenderElement &u)
{
    if (this == &u)
        return;
    mInfo = u.mInfo;
    APT_RI_INCSAFE(u.mpRenderItem, this, "AptRenderElement");
    if (u.mpRenderItem != mpRenderItem)
    {
        APT_RI_DECSAFE(mpRenderItem, this, "AptRenderElement");
        mpRenderItem = u.mpRenderItem;
    }
    SafeAddRef(u.mpRenderable);
    if (mpRenderable != u.mpRenderable)
    {
        SafeRelease(mpRenderable);
        mpRenderable = u.mpRenderable;
    }
    SafeAddRef(u.mpGeometry);
    if (mpGeometry != u.mpGeometry)
    {
        SafeRelease(mpGeometry);
        mpGeometry = u.mpGeometry;
    }
}

AptRenderListSet::AptRenderListSet()
{
}

bool AptRenderListSet::Open()
{
    Close();
    muCurrent++;
    AptRenderBuffer &pList = GetList();
    if (pList.IsRendering())
    {
        muCurrent--;
        return false;
    }
    pList.Clear();
    return true;
}

AptRenderBuffer::AptRenderBuffer()
{
}

AptRenderBuffer::~AptRenderBuffer()
{
    UnsafeClear();

    if (mList != NULL)
    {
        FreeList(mList, muCapacity, mAllocLocation);
        mList = NULL;
    }
}

void AptRenderBuffer::Initialize()
{
    muSize         = 0;
    muCapacity     = SHRINK_MINIMUM;
    mbClosed       = false;
    mbRendering    = false;
    mAllocLocation = ALLOC_DEFAULT;
    mList          = AllocList(muCapacity, ALLOC_DEFAULT);
    ClearShrinkThreshold();
}

void AptRenderBuffer::Shutdown()
{
    UnsafeClear();
    FreeList(mList, muCapacity, mAllocLocation);
    mList = NULL;
}

AptRenderElement *AptRenderBuffer::AllocList(unsigned capacity, AllocLocation allocLoc)
{
#if APT_USE_TEMPORARY_ALLOCATORS
    switch (allocLoc)
    {
    case ALLOC_DEFAULT:
        return (AptRenderElement *)(APT_MALLOC_BLOCK(capacity * sizeof(AptRenderElement)));
    case ALLOC_ALTERNATE:
        return (AptRenderElement *)(APT_MALLOC_TEMP(capacity * sizeof(AptRenderElement)));
    default:
        APT_ASSERT(false); // invalid allocation location
        break;
    }
    return NULL;
#else
    return (AptRenderElement *)(APT_MALLOC_RENDERLIST(capacity * sizeof(AptRenderElement)));
#endif
}

void AptRenderBuffer::FreeList(AptRenderElement *list, unsigned capacity, AllocLocation allocLoc)
{
#if APT_USE_TEMPORARY_ALLOCATORS
    switch (allocLoc)
    {
    case ALLOC_DEFAULT:
        APT_FREE_BLOCK(list, capacity * sizeof(AptRenderElement));
        break;

    case ALLOC_ALTERNATE:
        APT_FREE_TEMP(list, capacity * sizeof(AptRenderElement));
        break;

    default:
        APT_ASSERT(false); // invalid allocation location
        break;
    }
#else
    APT_FREE_RENDERLIST(list, capacity * sizeof(AptRenderElement));
#endif
}

unsigned AptRenderBuffer::GetBytesSize() const
{
    return muCapacity * sizeof(AptRenderElement);
}

bool AptRenderBuffer::Contains(const AptFile *pFile) const
{
    for (unsigned i = 0; i < muSize; i++)
        if (mList[i].Contains(pFile))
            return true;
    return false;
}

void AptRenderBuffer::Add(const AptRenderInfo &info, const AptRenderItem *pRenderItem, IAptRenderable *pRenderable, AptRenderableGeometry *pBounds)
{
    if (muSize >= muCapacity)
    {
        unsigned uPrevCapacity = muCapacity;
        muCapacity += GROWTH_AMOUNT;
        AllocLocation prevAllocLoc = mAllocLocation;
#if APT_USE_TEMPORARY_ALLOCATORS
        mAllocLocation = (mAllocLocation == ALLOC_DEFAULT) ? ALLOC_ALTERNATE : ALLOC_DEFAULT;
#endif
        AptRenderElement *pNewList = AllocList(muCapacity, mAllocLocation);
        memcpy(pNewList, mList, muSize * sizeof(AptRenderElement));
        FreeList(mList, uPrevCapacity, prevAllocLoc);
        mList = pNewList;
    }
    mList[muSize].Reset();
    mList[muSize] = AptRenderElement(info, pRenderItem, pRenderable, pBounds);
    muSize++;
}

void AptRenderBuffer::UnsafeClear()
{
    APT_ASSERT(!mbRendering);
    if (mbRendering)
    {
        APT_ASSERT(0 && "$lockless attempting to clear list during rendering same list (this shouldn't happen)..\n");
        return;
    }
    // we will only destroy the ones which are valid in the list, remaining ones are empty so don't call the destructor on them.
    for (unsigned i = 0; i < muSize; i++)
    {
        mList[i].DestroyEffect();
        mList[i].~AptRenderElement();
    }
    muSize   = 0;
    mbClosed = false;
}

void AptRenderBuffer::Clear()
{
    AptRenderListAutoLock lock(mLock);
    UnsafeClear();
}

void AptRenderBuffer::Close()
{
    SetCloseSize(muSize);
    if ((muSize + 1) * SHRINK_FACTOR < suShrinkThreshold && muCapacity > suShrinkThreshold)
        Shrink(suShrinkThreshold);
    mbClosed = true;
}

void AptRenderBuffer::Shrink(unsigned uCapacity)
{
#if APT_RENDER_LIST_ALLOW_SHRINKING
    APT_ASSERT(uCapacity >= muSize && "attempting to shrink the capacity of a render buffer below its active size!");
    if (uCapacity > muSize && muCapacity > uCapacity)
    {
        unsigned uPrevCapacity     = muCapacity;
        muCapacity                 = uCapacity;
        AllocLocation prevAllocLoc = mAllocLocation;
#if APT_USE_TEMPORARY_ALLOCATORS
        mAllocLocation = (mAllocLocation == ALLOC_DEFAULT) ? ALLOC_ALTERNATE : ALLOC_DEFAULT;
#endif
        AptRenderElement *pNewList = AllocList(muCapacity, mAllocLocation);
        memcpy(pNewList, mList, muSize * sizeof(AptRenderElement));
        FreeList(mList, uPrevCapacity, prevAllocLoc);
        mList = pNewList;
    }
#endif
}

bool AptRenderBuffer::IsClosed() const
{
    return mbClosed;
}

bool AptRenderBuffer::IsRendering() const
{
    return mbRendering;
}

void AptRenderBuffer::StartRender()
{
    mLock.Lock();
    mbRendering = true;
}

void AptRenderBuffer::EndRender()
{
    mbRendering = false;
    mLock.Unlock();
}

void AptRenderBuffer::Render(unsigned uLevelMask)
{
    if (mbClosed)
    {
        for (unsigned i = 0; i < muSize; i++)
            mList[i].RenderPrepare(uLevelMask);

        for (unsigned i = 0; i < muSize; i++)
            mList[i].Render(uLevelMask);
    }
    else
    {
        // fail..
        APT_ASSERT(0 && "$lockless attempting to render when frame is not available.");
    }
}

AptRenderListSet::~AptRenderListSet()
{
    Stop();
}

void AptRenderListSet::Add(const AptRenderInfo &info, const AptRenderItem *pRenderItem, IAptRenderable *pRenderable, AptRenderableGeometry *pBounds)
{
    if (!mbStop)
        GetList().Add(info, pRenderItem, pRenderable, pBounds);
}

void AptRenderListSet::StartRender()
{
    // actually, we don't do anything here..
}

void AptRenderListSet::EndRender()
{
    GetRenderList().EndRender();
    muRender = 0;
}

void AptRenderListSet::Close()
{
    AptRenderBuffer &pList = GetList();
    if (!pList.IsClosed())
        pList.Close();
    muComplete = muCurrent;
    ReleaseFileQueue();
    /*  // for gathering memory metrics
        unsigned uSize = 0;
        printf("\nRender Buffer Metrics:\n");
        for (unsigned i = 0; i < LIST_COUNT; i++)
        {
            unsigned uListSize = mLists[i].GetBytesSize();
            uSize += uListSize;
            printf("sizeof(RenderBuffer[%u]) = %u\n", i, uListSize);
        }
        printf("total                    = %u\n", uSize);
    */
}

AptRenderBuffer &AptRenderListSet::GetList()
{
    return mLists[muCurrent % LIST_COUNT];
}

AptRenderBuffer &AptRenderListSet::GetNextList()
{
    return mLists[(muCurrent + 1) % LIST_COUNT];
}

AptRenderBuffer &AptRenderListSet::GetRenderList()
{
    return mLists[muRender % LIST_COUNT];
}

bool AptRenderListSet::Contains(const AptFile *pFile) const
{
    for (const auto &list : mLists)
        if (list.Contains(pFile))
            return true;
    return false;
}

bool AptRenderListSet::Find(const AptFile *pFile)
{
    void **p = mFileFreeQueue.Find((const void *)pFile);
    return p != 0;
}

bool AptRenderListSet::AttemptDestroy(AptFile *pFile)
{
    if (pFile && !Contains(pFile))
    {
        delete pFile;
        return true;
    }
    return false;
}

void AptRenderListSet::Destroy(AptFile *pFile)
{
    // This lock used to be inside the if check, but the "Find" function accesses
    // the mFileFreeQueue so it seems like it should be outside the if check.
    // This MAY help fix a very very rare threading issue that has been seen on
    // Madden and NCAA where the same memory address is deleted twice.  But since
    // It's so rare, we don't actually know.
    AptRenderListAutoLock lock(mQueueLock);
    if (pFile && !Find(pFile) && !AttemptDestroy(pFile))
    {
        mFileFreeQueue.Add(pFile);
    }
}

void AptRenderListSet::Initialize()
{
    muCurrent  = 0;
    muComplete = 0;
    muRender   = 0;
    mbStop     = false;

    for (auto &list : mLists)
    {
        list.Initialize();
    }
}

void AptRenderListSet::Shutdown()
{
    for (auto &list : mLists)
    {
        list.Shutdown();
    }
    AptRenderListAutoLock lock(mQueueLock);
    mFileFreeQueue.Clear();
}

void AptRenderListSet::ReleaseFileQueue()
{
    AptRenderListAutoLock lock(mQueueLock);
    bool found = false;
    for (AptFile **p = (AptFile **)mFileFreeQueue.first; p != (AptFile **)mFileFreeQueue.last; ++p)
    {
        if ((*p)->mRefCount == 0)
        {
            if (AttemptDestroy(*p))
            {
                found = true;
                *p    = 0; // set it to null to mark it..
            }
        }
        else
        { // object got resurrected after it was placed on the destroy list, so no longer belongs on the list -- remove it
            found = true;
            *p    = 0; // set it to null to mark it..
        }
    }
    if (found)
        mFileFreeQueue.Remove(0); // now remove all the nulls..
}

void AptRenderListSet::Render(unsigned uLevelMask)
{
    muRender = muComplete;
    GetRenderList().StartRender();
    AptRenderBuffer *pRenderList = &GetRenderList();
    if (pRenderList && pRenderList->IsClosed())
        pRenderList->Render(uLevelMask);
    // else, render is lagging behind sim..
}

void AptRenderListSet::Stop()
{
    mbStop = true;
    for (auto &list : mLists)
        list.Clear();
    ReleaseFileQueue();
}

APT_THREAD_NAMESPACE::Futex AptRenderList::mRestartMutex;
void AptRenderList::Initialize()
{
#ifndef APT_SINGLE_THREADED
    AptRenderListAutoLock lock(gRenderListLock);
    if (gpRenderListSet == NULL)
    {
        gpRenderListSet = &gRenderListSet;
        gpRenderListSet->Initialize();
    }
#endif
}

bool AptRenderList::Open()
{
#ifndef APT_SINGLE_THREADED
    if (!gpRenderListSet)
        return false;
    return gpRenderListSet->Open();
#else
    return true;
#endif
}

void AptRenderList::Destroy(AptFile *pFile)
{
    if (pFile)
    {
#ifndef APT_SINGLE_THREADED
        if (gpRenderListSet)
            gpRenderListSet->Destroy(pFile);
        else
            delete pFile;
#else
        delete pFile;
#endif
    }
}

void AptRenderList::Close()
{
#ifndef APT_SINGLE_THREADED
    if (gpRenderListSet)
        gpRenderListSet->Close();
#endif
}

void AptRenderList::StartRender()
{
#ifndef APT_SINGLE_THREADED
    mRestartMutex.Lock();
    if (gpRenderListSet)
        gpRenderListSet->StartRender();
#endif
}

void AptRenderList::Shutdown()
{
#ifndef APT_SINGLE_THREADED
    AptRenderListAutoLock lock(gRenderListLock);
    if (gpRenderListSet)
    {
        gpRenderListSet->Shutdown();
        gpRenderListSet = NULL;
    }
#endif
}

void AptRenderList::Render(unsigned uLevelMask)
{
#ifndef APT_SINGLE_THREADED
    AptRenderListAutoLock lock(gRenderListLock);
    if (gpRenderListSet)
        gpRenderListSet->Render(uLevelMask);
#endif
}

void AptRenderList::EndRender()
{
#ifndef APT_SINGLE_THREADED
    if (gpRenderListSet)
        gpRenderListSet->EndRender();
    mRestartMutex.Unlock();
#endif
}

void AptRenderList::Stop()
{
#ifndef APT_SINGLE_THREADED
    if (gpRenderListSet)
        gpRenderListSet->Stop();
#endif
}

void AptRenderList::Restart()
{
#ifndef APT_SINGLE_THREADED
    mRestartMutex.Lock();
    Shutdown();
    Initialize();
    mRestartMutex.Unlock();
#endif
}

void AptRenderList::Add(const AptRenderInfo &info, const AptRenderItem *pRenderItem, IAptRenderable *pRenderable, AptRenderableGeometry *pBounds)
{
#ifndef APT_SINGLE_THREADED
    if (gpRenderListSet)
        gpRenderListSet->Add(info, pRenderItem, pRenderable, pBounds);
#else
    if (pRenderable)
    {
        alignas(16) AptRenderInfo info_aligned;
        info_aligned = info;
        CallLegacyTransformCallbacks(info_aligned);
        pRenderable->Render(info_aligned, pBounds);
    }
#endif
}
