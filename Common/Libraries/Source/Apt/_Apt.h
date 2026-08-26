#pragma once

#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>
#include <time.h>

#include "Apt.h"
#include "AptDefine.h"
#include "AptLibrary.h"
#include "AptSharedPtr.h"
#include "_AptInternalDefines.h"
#include "_AptValue.h"
#include "string/StringPool.h"
#include "_AptLoad.h"
#include "_AptFile.h"
#include "_AptActions.h"
#include "AptObject/AptGlobalExtensionObject.h"

class IAptXmlImpl;
class AptAnimationTarget;
class AptBCRenderTreeManager;
struct AptLoader;
struct AptLinker;
struct AptFilter;

/** Bit flags on AptControlPlaceObject2/3 saying which optional fields are present. */
enum AptPlaceObjectFlags
{
    AptPlaceObjectFlag_None       = 0x00,
    AptPlaceObjectFlag_Move       = 0x01, // this place moves an existing object
    AptPlaceObjectFlag_Character  = 0x02, // there is a character tag (if no tag, must be a move)
    AptPlaceObjectFlag_Matrix     = 0x04, // there is a matrix (matrix)
    AptPlaceObjectFlag_CXForm     = 0x08, // there is a color transform (cxform with alpha)
    AptPlaceObjectFlag_Ratio      = 0x10, // there is a blend ratio (word)
    AptPlaceObjectFlag_Name       = 0x20, // there is an object name (string)
    AptPlaceObjectFlag_DefineClip = 0x40, // this shape should open or close a clipping bracket (character != 0 to open, character == 0 to close)
    AptPlaceObjectFlag_Actions    = 0x80, // has actions associated with it
};

/** Values match what is output by swfc. */
enum AptControlType
{
    AptControlType_DoAction         = 1,
    AptControlType_FrameLabel       = 2,
    AptControlType_PlaceObject2     = 3,
    AptControlType_RemoveObject2    = 4,
    AptControlType_BackgroundColour = 5,
    AptControlType_StartSound       = 6,
    AptControlType_StartSoundStream = 7,
    AptControlType_DoInitAction     = 8,
    AptControlType_PlaceObject3     = 9,
};

struct AptControlDoAction
{
    AptActionBlock actions;
};

struct AptControlDoInitAction
{
    int nSpriteID;
    AptActionBlock actions;
};

struct AptControlFrameLabel
{
    char *szLabel;
};

struct AptFloatArrayCXForm
{
    float scale[4];
    float translate[4];
};

struct AptUint32CXForm
{
    uint32_t nScale;
    uint32_t nBias;
};

struct AptControlPlaceObject2
{
    AptPlaceObjectFlags eFlags; // could be char
    int nDepth;                 // could be short
    int nCharacterID;           // could be short
    AptMatrix matrix;
    AptUint32CXForm ncxform;
    float fRatio;                // for morphs, could be short
    char *szName;                // ptr
    int nClipDepth;              // could be short
    AptEventActionSet *pActions; // ptr
};

struct AptControlPlaceObject3
{
    AptPlaceObjectFlags eFlags; // could be char
    int nDepth;                 // could be short
    int nCharacterID;           // could be short
    AptMatrix matrix;
    AptUint32CXForm ncxform;
    float fRatio;                // for morphs, could be short
    char *szName;                // ptr
    int nClipDepth;              // could be short
    AptEventActionSet *pActions; // ptr
    int32_t nBlendMode;
    uint32_t nNumFilters;
    AptFilter **ppFilters;
};

struct AptControlRemoveObject2
{
    int nDepth;
};

struct AptControlBackgroundColour
{
    unsigned int nColour;
};

struct AptControlSound
{
#if defined(APT_USE_SOUND_OBJECT)
    int nID;
#endif
};

/** One SWF control tag (place/remove object, actions, sound, ...) as emitted by swfc. */
struct AptControl
{
    AptControlType eType;
    union
    {
        AptControlDoAction action;
        AptControlDoInitAction initAction;
        AptControlFrameLabel frameLabel;
        AptControlPlaceObject2 placeObject2;
        AptControlPlaceObject3 placeObject3;
        AptControlRemoveObject2 removeObject2;
        AptControlBackgroundColour backgroundColour;
#if defined(APT_USE_SOUND_OBJECT)
        AptControlSound startSound;
        AptControlSound startSoundStream;
#endif
    };
};

struct AptFrame
{
    int nControls;
    AptControl **apControls;
};

struct AptCharacterButtonRecord
{
#if defined APT_USE_BUTTONS
    AptCharacterButtonRecordState eStates;
    AptCharacter *pCharacter;
    int nLayer;
    AptMatrix matrix;
    AptFloatArrayCXForm cxform;
#endif
};

struct AptCharacterButtonSound
{
#if defined APT_USE_BUTTONS
    AptCharacter *pOverUpToIdle;
    AptCharacter *pIdleToOverUp;
    AptCharacter *pOverUpToOverDown;
    AptCharacter *pOverDownToOverUp;
#endif
};

struct AptCharacterGlyphEntry
{
    short nIndex;
    short nAdvance;
};

struct AptCharacterStaticTextRecords
{
    int nFontID;
    AptFloatArrayCXForm cxform;
    float fXOffset;
    float fYOffset;
    float fScale;
    int nGlyphs;
    AptCharacterGlyphEntry *aGlyphs;
};

struct AptImport
{
    char *szFile;
    char *szName;
    int nID;
    AptFilePtr file;
};

struct AptExport
{
    char *szName;
    int nID;
};

#include "AptTarget.h"

/** Only to be called by APT_INC/APT_DEC to keep external increment/decrement thread-safe. */
extern void AptUpdateLock();
extern void AptUpdateUnlock();

/** Convenience auto-lock class for AptUpdateLock()/AptUpdateUnlock(). */
class AptUpdateAutoLock
{
  public:
    inline AptUpdateAutoLock()
    {
        AptUpdateLock();
    }
    inline ~AptUpdateAutoLock()
    {
        AptUpdateUnlock();
    }
};

/**
 * Atomic-style read/write/increment helpers (equate to native operations in non-decoupled mode).
 * @note these do NOT do pointer arithmetic: they implicitly cast all values to uint32_t/intptr_t,
 * e.g. APT_ADD32(x, 4) adds 4 to the uint32_t value of x, not 4*sizeof(*x) as pointer arithmetic would.
 */
#define APT_READ32(ref) (ref)
#define APT_WRITE32(ref, value) (*(uint32_t *)&ref) = ((uint32_t)value)
#define APT_ADD32(ref, value) (*(uint32_t *)&ref) += ((uint32_t)value)
#define APT_SUB32(ref, value) (*(uint32_t *)&ref) -= ((uint32_t)value)
#define APT_INC32(ref) (++(*(uint32_t *)&ref))
#define APT_DEC32(ref) (--(*(uint32_t *)&ref))
#define APT_CONDITIONAL_EXCHANGE32(ref, compareValue, newvalue) \
    if ((ref) == (compareValue))                                \
        (ref) = (newvalue);
#define APT_READ_PTR(ref) (ref)
#define APT_WRITE_PTR(ref, value) (*(intptr_t *)&ref) = ((intptr_t)value)
#define APT_ADD_PTR(ref, value) (*(intptr_t *)&ref) += ((intptr_t)value)
#define APT_SUB_PTR(ref, value) (*(intptr_t *)&ref) -= ((intptr_t)value)
#define APT_INC_PTR(ref) (++(*(intptr_t *)&ref))
#define APT_DEC_PTR(ref) (--(*(intptr_t *)&ref))
#define APT_CONDITIONAL_EXCHANGE_PTR(ref, compareValue, newvalue) \
    if ((ref) == (compareValue))                                  \
        (ref) = (newvalue);

/** Rebases a pointer loaded from a relocatable .apt file onto its runtime base address. */
template <class T>
static inline T *AptResolve(T *ptr, void *pBase)
{
    if (ptr != 0)
    {
        return (T *)((uintptr_t)ptr + (uintptr_t)pBase);
    }
    return ptr;
}

/** Inverse of AptResolve(): turns a runtime pointer back into a file-relative offset. */
template <class T>
static inline T *AptUnResolve(T *ptr, void *pBase)
{
    if (ptr != 0)
    {
        return (T *)((uintptr_t)ptr - (uintptr_t)pBase);
    }
    return ptr;
}

#define APT_RESOLVE(_)            \
    {                             \
        _ = AptResolve(_, pBase); \
    }
#define APT_UNRESOLVE(_)            \
    {                               \
        _ = AptUnResolve(_, pBase); \
    }

// SavedInputPlayback now lives in AptLibrary.h, next to the library member
// that holds it.

struct ClipEventType
{
    int nFlag;
    StringCode eName;
};

#if defined(APT_USE_MOUSE)
#define CLIP_EVENT_ARRAY_SIZE 17
#else
#define CLIP_EVENT_ARRAY_SIZE 6
#endif

/** The ActionScript "undefined" value. A single global instance is shared by all scopes. */
class AptNone : public AptValueNoGC
{
  public:
    APT_VALUE_NEW_DELETE_OPERATORS

    AptNone() : AptValueNoGC(AptVFT_None)
    {
        setIsDefined(0);
        setRefCount(MAX_REFCOUNT);
    }

    // Optimizes the global instance by doing nothing on Add/Release.
#if defined(APT_INC_DEC_MESSAGES)
    virtual void AddRef(const char *szFuncName, const char *szFileName, int nLineNumber) {}
    virtual void Release(const char *szFuncName, const char *szFileName, int nLineNumber) {}
#else
    virtual void AddRef() {}
    virtual void Release() {}
#endif

  protected:
    APT_INLINE
    virtual ~AptNone()
    {
        //  Do nothing...
    }
};

/** Backing object for the ActionScript "extern" pseudo-scope, used to set external variables. */
class AptExtern : public AptValueNoGC
{
  public:
    APT_VALUE_NEW_DELETE_OPERATORS

    AptExtern() : AptValueNoGC(AptVFT_Extern)
    {
        setGCRoot(1);
        setRefCount(MAX_REFCOUNT);
    }

    virtual bool objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue);

    // Optimizes the global instance by doing nothing on Add/Release.
#if defined(APT_INC_DEC_MESSAGES)
    virtual void AddRef(const char *szFuncName, const char *szFileName, int nLineNumber) {}
    virtual void Release(const char *szFuncName, const char *szFileName, int nLineNumber) {}
#else
    virtual void AddRef() {}
    virtual void Release() {}
#endif

    APT_INLINE
    virtual ~AptExtern()
    {
        //  Do nothing...
    }
};

extern ClipEventType _aClipEvents[];

/** Removes pTarget from the zombie vector, if present; used when the object is about to be deleted. */
bool AptRemoveCIHFromZombieVector(AptCIH *pTarget);

/** Sets the SWF version of the animation currently being interpreted, affecting some opcode semantics. */
void AptSetSwfVersion(int32_t p_version);

#if !defined(APT_ALTERNATE_INPUT)
#define APT_VALUESIZES_ARRAYSIZE 42
#else
#define APT_VALUESIZES_ARRAYSIZE 43
#endif
extern const uint8_t AptValueSizesByVType[APT_VALUESIZES_ARRAYSIZE];

/** @return the byte size of the concrete AptValue subclass behind pValue. */
uint32_t AptGetSizeOfAptValue(const AptValue *pValue);

/** @return the concrete AptValue type of pValue, as a string. */
const char *AptGetTypeOfAptValue(const AptValue *pValue);

// Compiled bytecode for the flash.filters.* classes; see filters/filters_little.cpp.
extern unsigned char g_arrFiltersAptFile[];
extern unsigned char g_arrFiltersConstFile[];

/** Mersenne Twister PRNG, seeded automatically on first use. */
uint32_t AptRand(void);

bool _AptValidate(void);

/** Requests a garbage-collection pass at the end of the current frame. */
void AptPartialGarbageCollection();

/** Marks an AptFile's userdata as belonging to the flash.filters.* bytecode preload, not a real load. */
#define APT_FILTERS_FILE_USERDATA 0x12345679

/** Returns the currently set sim target. */
APT_INLINE AptTarget *GetTargetSim();

/** Returns the currently set render target. */
APT_INLINE AptTarget *GetTargetRender();

/** Returns the currently set target based on thread id. */
AptTarget *GetTarget();

AptCIH *_AptGetAnimationAtLevel(int32_t nLevel);
void AptUpdateZombieVector(bool bClean = false);

float Apt_atoff(const char *_s);
extern int32_t AptGetSwfVersion();

extern void AptRegisterGlobalReferences();
extern void AptPreloadFilterSWFFile();

void AptInitializeGC();
void AptCommonInitialize(const AptInitParams *pAptInitParms);
void AptCommonShutdown();
void CleanAllNativeFunctions();
