#pragma once
#include <cstddef>

// the feature gates below (e.g. APT_USE_MOUSE) depend on the platform defines
#include "AptPlatform.h"

#ifndef NDEBUG
#define APT_DEBUG
#endif

#if !defined(APT_ALLOCATION_TRACKING)
// #define   APT_ALLOCATION_TRACKING                 1
#endif
#if !defined(APT_XML_EXTENDED_MEMORY_DUMP_SUPPORTED)
// #define   APT_XML_EXTENDED_MEMORY_DUMP_SUPPORTED  1   /* Provides a unique value to each allocated AptValue, this value should not be duplicated even across multiple dumps. */
#endif
#if !defined(APT_INC_DEC_MESSAGES)
// #define   APT_INC_DEC_MESSAGES                    1
#endif

// this will use the new class initialization order fix where parent class constructors for movie clip get
// called before child class constructors for movie clips get called
#if !defined(APT_USE_NEW_CLASS_INIT_ORDER)
// #define APT_USE_NEW_CLASS_INIT_ORDER                1
#endif

/**
 * AptCXForm uses two AptColorHelper objects to maintain the color transform.
 * By default, AptColorHelper represents colors as 4 bytes (ARGB) in the range of 0-255.
 * However, you can achieve much more accurate emulation of Flash color transformations by defining "APT_USE_FLASH_COLOR_RANGE".
 * By doing this, the color "scale" values range from 0.0 to 100.0, and the "translate" values range from -255.0 to 255.0,
 * just like Flash. However, when APT_USE_FLASH_COLOR_RANGE is defined, the color components are represented by 4 floats,
 * so performance may differ from the default.
 */
#if !defined(APT_USE_FLASH_COLOR_RANGE)
#define APT_USE_FLASH_COLOR_RANGE 1
#endif

#if defined(APT_DEBUG)
// Any of these can be defined in Release if you want, but are only on by default in debug builds.

// Disabling this one to make our memory allocations more in line on both Debug and Opt
// #if !defined(APT_VERIFY_NON_GC_ALLOC_SIZES)
//    #define APT_VERIFY_NON_GC_ALLOC_SIZES             1  // Adds 4 bytes per NonGC allocation.
// #endif
#if !defined(DOGMA_EXTRA_MEMORY_COUNTERS)
#define DOGMA_EXTRA_MEMORY_COUNTERS 1
#endif
#if !defined(APT_XML_MEMORY_DUMP_SUPPORTED)
#define APT_XML_MEMORY_DUMP_SUPPORTED 1 // Allows XML based dumps of all Apt objects. Requires APT_USE_REGISTER_CALLBACKS.
#endif                                  // Object ID's are unique within a given dump, but may be duplicated in another dump. */
#if !defined(ALLOW_PLAY_INPUTS)
#define ALLOW_PLAY_INPUTS 1 // allows to play inputs
#endif
#if !defined(ALLOW_SAVE_INPUTS)
#define ALLOW_SAVE_INPUTS 1 // allows to save inputs
#endif
#endif

#define ALLOW_PLAY_INPUTS 1 // allows to play inputs
#define ALLOW_SAVE_INPUTS 1 // allows to save inputs

// Mouse code in Apt is compiled out unless told no to.  PC uses the mouse by default.
#if defined(APT_PLATFORM_WINDOWS) || defined(APT_PLATFORM_LINUX)
#if !defined(APT_USE_MOUSE)
#define APT_USE_MOUSE 1
#endif
#endif

#if defined(DO_COVERAGE)
#define APT_USE_BUTTONS 1
#endif

// Added - for 0.18.03 version
// these #defines decide whether code related to those actionscript objects will be compiled in or not.
// Class definitions/constructors-destructors will still be there but all other functionality will be compiled out
// in debug mode it will assert if #define is not defined and AS object is used.
#if !defined(APT_USE_DATE_OBJECT)
// #define     APT_USE_DATE_OBJECT                 1
#endif
#if !defined(APT_USE_XML_OBJECT)
#define APT_USE_XML_OBJECT 1
#endif
#if !defined(APT_USE_STAGE_OBJECT)
// #define     APT_USE_STAGE_OBJECT                1
#endif
#if !defined(APT_USE_BUTTONS)
#define APT_USE_BUTTONS 1
#endif
#if !defined(APT_USE_REPEATED_INPUT_OPTI)
#define APT_USE_REPEATED_INPUT_OPTI 1
#endif

// Extended the changes made above, allowing more features to be compiled out.
// If you do not want the feature, comment out the #define below so it will not be compiled into the library.
#if !defined(APT_USE_SOUND_OBJECT)
#define APT_USE_SOUND_OBJECT 1 // this removes all support for Sound objects.
#endif
#if !defined(APT_3D)
#define APT_3D 1 // Provides _z, _zscale, _xrotation, _yrotation properties for movieclips
#endif
#if !defined(APT_RENDER_FLAGS)
// #define     APT_RENDER_FLAGS                    1   // provides _renderflags property for movieclip.
#endif
// This define will turn on code that uses ZID to be passed to custom control instead of strings.
// 3 new callbacks will be added because of this - pfnCreateCustomControlZid, pfnUpdateCustomControlZid and pfnDestroyCustomControlZid
// pfnCreateCustomControlZid is called only ones for a customcontrol movieclip when it is firt time found as custom control
// Aux lib should create required data structure for that custom control and should return
// the void pointer to it. pfnDestroyCustomControlZid will be called from AptRenderItemcustomcontrol destructor.
// pfnUpdateCustomControlZid will be called AptCIH::ProcessCustomControls
#if !defined(APT_CUSTOM_CONTROL_USE_ZID)
#define APT_CUSTOM_CONTROL_USE_ZID 1
#endif

//------------------------------------------------------------------------------
// Debugging macros to help standardize debug code.

#define APT_DEBUG_NONE 0    /* No Debugging */
#define APT_DEBUG_MINIMUM 1 /* Minimal Debugging Information */
#define APT_DEBUG_NORMAL 2  /* Normal Debugging Information  */
#define APT_DEBUG_CUSTOM 3  /* Used for Temporary / Custom Tests during development. Completed tasks should use other levels. */
#define APT_DEBUG_MAXIMUM 4 /* Internal Testing, Little or no concern for memory / time */

// APT_DEBUG_LEVEL - Defaults to Normal in a debug build, none, in a release build
#if !defined(APT_DEBUG_LEVEL)
#if defined(APT_DEBUG)
#define APT_DEBUG_LEVEL (APT_DEBUG_NORMAL)
#else
#define APT_DEBUG_LEVEL (APT_DEBUG_NONE)
#endif
#endif

// Added new level APT_DEBUG_MSG_ASSERT_LVL that can be used to log ASSERTs in release mode.
#define APT_DEBUG_MSG_NONE_LVL 0
#define APT_DEBUG_MSG_ASSERT_LVL 1
#define APT_DEBUG_MSG_DEFAULT_LVL 2
#define APT_DEBUG_MSG_NORMAL_LVL APT_DEBUG_MSG_DEFAULT_LVL
#define APT_DEBUG_MSG_WARNING_LVL 3
#define APT_DEBUG_MSG_DEBUG_LVL 4

// value of APT_CURR_DEBUG_MSG_LVL now decides in what conditions APT_ASSERT and APT_DEBUGPRINT are defined.
// Previously it used to be APT_DEBUG_LEVEL, but there is lot of extra code added for APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
// so it is hard to use in release mode to turn on ASSERTs.
#if !defined(APT_CURR_DEBUG_MSG_LVL)
#if defined(APT_DEBUG)
#define APT_CURR_DEBUG_MSG_LVL APT_DEBUG_MSG_NORMAL_LVL
#else
// In release/ship mode this should be set to APT_DEBUG_MSG_NONE_LVL so it will not generate any code for ASSERTs, APT_DEBUGPRINT
// but to turn on asserts in release mode, set it to minimum of APT_DEBUG_MSG_ASSERT_LVL
#define APT_CURR_DEBUG_MSG_LVL APT_DEBUG_MSG_NONE_LVL
#endif
#endif

//------------------------------------------------------------------------------
// APT_ASSERT macro

void AptAssert(const char *statement, const char *message, const char *file, int line);

#if (APT_CURR_DEBUG_MSG_LVL >= APT_DEBUG_MSG_ASSERT_LVL)

#define APT_ASSERT(_Statement)                                \
    {                                                         \
        if (!(_Statement))                                    \
        {                                                     \
            AptAssert(#_Statement, NULL, __FILE__, __LINE__); \
        }                                                     \
    }

#define APT_ASSERTM(_Statement, _Msg)                         \
    {                                                         \
        if (!(_Statement))                                    \
        {                                                     \
            AptAssert(#_Statement, _Msg, __FILE__, __LINE__); \
        }                                                     \
    }

#define APT_FAIL(_Msg) AptAssert(NULL, _Msg, __FILE__, __LINE__);

#define NOT_REACHED 0

#else

#define APT_ASSERT(_Statement)        /* Nothing */
#define APT_ASSERTM(_Statement, _Msg) /* Nothing */
#define APT_FAIL(_Statement)          /* Nothing */

#endif

//------------------------------------------------------------------------------
// Basic Typedefs
#ifndef TYPE_DEFINED
#define TYPE_DEFINED
using uint32_t = unsigned int;
using int32_t  = signed int;
#endif

//------------------------------------------------------------------------------
// APT_INLINE
#if defined(APT_DEBUG) || !defined(APTLIBRARY)
#undef APT_ENABLE_INLINE
#else
#define APT_ENABLE_INLINE
#endif

#if !defined(APT_ENABLE_INLINE)

/** In Debug builds or external inclusions, don't inline anything. */
#define APT_FORCE_INLINE
#define APT_INLINE

#else // defined(APT_DEBUG) || !defined(APTLIBRARY)

// these are the default values, they will be overridden as needed below.
#define APT_FORCE_INLINE inline
#define APT_INLINE inline

// This mess defines force inline macros differently per platform as necessary.
#if defined(_MSC_VER)
// Undef the default value and give it a compiler specific value.
#undef APT_INLINE
#define APT_INLINE /*__declspec(dllexport)*/ inline
#undef APT_FORCE_INLINE
#define APT_FORCE_INLINE /*__declspec(dllexport)*/ __forceinline
#endif

#if defined(APT_PLATFORM_PS3)
#if !defined(EA_COMPILER_SN)
#undef APT_FORCE_INLINE
#define APT_FORCE_INLINE inline __attribute__((always_inline)) __attribute__((used)) /*__attribute__ ((visibility("default")))*/
#undef APT_INLINE
#define APT_INLINE inline __attribute__((used)) /*__attribute__ ((visibility("default")))*/
#else
#undef APT_FORCE_INLINE
#define APT_FORCE_INLINE inline __attribute__((always_inline)) __attribute__((used))
#undef APT_INLINE
#define APT_INLINE inline __attribute__((used))
#endif
#endif

#if defined(EA_COMPILER_CLANG)
#undef APT_FORCE_INLINE
#define APT_FORCE_INLINE inline __attribute__((always_inline)) __attribute__((used))
#undef APT_INLINE
#define APT_INLINE inline __attribute__((used))
#endif

#endif // #if defined(APT_DEBUG) || !defined(APTLIBRARY)

//------------------------------------------------------------------------------
// APT_ACCESS_INTERNAL: Used to stop external access to internal values.
#if defined(APTLIBRARY)
#define APT_ACCESS_INTERNAL public
#else
#define APT_ACCESS_INTERNAL protected
#endif

// GetClassName is a windows define. Any game code that includes Apt headers will always have window headers included.
// So, we are undefining it here to keep the precompiler from replacing it everywhere we use it.
#if APT_PLATFORM_WINDOWS
#ifdef GetClassName
#undef GetClassName
#endif // GetClassName
#endif // APT_PLATFORM_WINDOWS

//-----------------------------------------------------------------------------------------------
class DOGMA_PoolManager;
class AptValueGC_PoolManager;

AptValueGC_PoolManager *GetGCPoolManager();
DOGMA_PoolManager *GetNonGCPoolManager();


// This is used some places to conditionally add FILe, LINE, it's dirty I admit, but it's probably better
// then #defines everywhere...
#if defined APT_ALLOCATION_TRACKING
#define APT_ALLOC_PARAMS(size) size, __FILE__, __LINE__
#else
#define APT_ALLOC_PARAMS(size) size
#endif

#if defined(APT_ALLOCATION_TRACKING)

// These are used by the array allocators.
void *AptNonGCAllocSaveSize(size_t nSize, const char *szName, int nLine);
void AptNonGCFreeSavedSize(void *p);

#define APT_NONGC_NEW(bytes) (GetNonGCPoolManager()->TrackedAllocate(bytes, __FILE__, __LINE__))
#define APT_NONGC_DELETE(ptr, bytes) (GetNonGCPoolManager()->TrackedDeallocate((void *)ptr, bytes))
#define APT_GC_NEW(bytes) (GetGCPoolManager()->TrackedAllocateAptValueGC(bytes, __FILE__, __LINE__))
#define APT_GC_DELETE(ptr, bytes) (GetGCPoolManager()->TrackedDeallocateAptValueGC((AptValueGC *)ptr, bytes))

#elif defined(APT_VERIFY_NON_GC_ALLOC_SIZES)

// These are used by the array allocators.
void *AptNonGCAllocSaveSize(size_t nSize);
void AptNonGCFreeSavedSize(void *p);

#define APT_NONGC_NEW(bytes) (GetNonGCPoolManager()->SizeVerifyAllocate(bytes))
#define APT_NONGC_DELETE(ptr, bytes) (GetNonGCPoolManager()->SizeVerifyDeallocate((void *)ptr, bytes))
#define APT_GC_NEW(bytes) (GetGCPoolManager()->AllocateAptValueGC(bytes))
#define APT_GC_DELETE(ptr, bytes) (GetGCPoolManager()->DeallocateAptValueGC((AptValueGC *)ptr, bytes))

#else

// These are used by the array allocators.
void *AptNonGCAllocSaveSize(size_t nSize);
void AptNonGCFreeSavedSize(void *p);

#define APT_NONGC_NEW(bytes) (GetNonGCPoolManager()->Allocate(bytes))
#define APT_NONGC_DELETE(ptr, bytes) (GetNonGCPoolManager()->Deallocate((void *)ptr, bytes))
#define APT_GC_NEW(bytes) (GetGCPoolManager()->AllocateAptValueGC(bytes))
#define APT_GC_DELETE(ptr, bytes) (GetGCPoolManager()->DeallocateAptValueGC((AptValueGC *)ptr, bytes))

#endif // #if defined(APT_ALLOCATION_TRACKING)

//-----------------------------------------------------------------------------------------------
// Custom new and delete operators for Apt classes
//
#define APT_VALUE_NEW_DELETE_OPERATORS                                                                 \
    static void *operator new(size_t size) { return APT_NONGC_NEW(size); }                             \
    static void operator delete(void *p, size_t size) { APT_NONGC_DELETE(p, size); }                   \
    static void *operator new[](size_t size) { return AptNonGCAllocSaveSize(APT_ALLOC_PARAMS(size)); } \
    static void operator delete[](void *p)                                                             \
    {                                                                                                  \
        AptValueNoGC::VerifyAptValueNoGC();                                                            \
        AptNonGCFreeSavedSize(p);                                                                      \
    }

#define APT_VALUE_GC_NEW_DELETE_OPERATORS                                           \
    static void *operator new(size_t size) { return APT_GC_NEW(size); }             \
    static void operator delete(void *p, size_t size) { APT_GC_DELETE(p, size); }   \
    static void *operator new[](size_t)                                             \
    {                                                                               \
        APT_FAIL("Garbage collected Objects should never be created in Arrays!!!"); \
        return (void *)0x00000000;                                                  \
    }                                                                               \
    static void operator delete[](void *) { AptValueGC::VerifyAptValueGC(); }

//-----------------------------------------------------------------------------------------------
#include "DogmaAllocator.h"

#define APT_NEW_DELETE_OPERATORS                                                                       \
  public:                                                                                              \
    static void *operator new(size_t size) { return APT_NONGC_NEW(size); }                             \
    static void operator delete(void *p, size_t size) { APT_NONGC_DELETE(p, size); }                   \
    static void *operator new[](size_t size) { return AptNonGCAllocSaveSize(APT_ALLOC_PARAMS(size)); } \
    static void operator delete[](void *p) { AptNonGCFreeSavedSize(p); }                               \
    static void *operator new(size_t t, void *p) { return p; }

#define NATIVE_MEMBER_FUNCTION_DECL(_name_)                          \
    static AptValue *sMethod_##_name_(AptValue *pThis, int nParams); \
    static AptNativeFunction *psMethod_##_name_;

#define NATIVE_MEMBER_FUNCTION(_class_, _name_)        \
    AptNativeFunction *_class_::psMethod_##_name_ = 0; \
    AptValue *_class_::sMethod_##_name_(AptValue *pThis, int nParams)

//------------------------------------------------------------------------------
// Increment / Decrement messages. Turn them on with APT_INC_DEC_MESSAGES.
// NOTE: These macros are ONLY intended to be used from within the Apt runtime source code.
//       Users should use APT_INC and APT_DEC instead.
#if defined(__GNUC__) && defined(APT_INC_DEC_MESSAGES)
#define _APT_DEC(_) (_)->Release(__PRETTY_FUNCTION__, __FILE__, __LINE__);
#define _APT_DECSAFE(_)                                        \
    if (_)                                                     \
    {                                                          \
        (_)->Release(__PRETTY_FUNCTION__, __FILE__, __LINE__); \
    }
#define _APT_INC(_) (_)->AddRef(__PRETTY_FUNCTION__, __FILE__, __LINE__);
#define _APT_INCSAFE(_)                                       \
    if (_)                                                    \
    {                                                         \
        (_)->AddRef(__PRETTY_FUNCTION__, __FILE__, __LINE__); \
    }
#elif defined(_MSC_VER) && defined(APT_INC_DEC_MESSAGES)
#define _APT_DEC(_) (_)->Release(__FUNCTION__, __FILE__, __LINE__);
#define _APT_DECSAFE(_)                                 \
    if (_)                                              \
    {                                                   \
        (_)->Release(__FUNCTION__, __FILE__, __LINE__); \
    }
#define _APT_INC(_) (_)->AddRef(__FUNCTION__, __FILE__, __LINE__);
#define _APT_INCSAFE(_)                                \
    if (_)                                             \
    {                                                  \
        (_)->AddRef(__FUNCTION__, __FILE__, __LINE__); \
    }
#else
#define _APT_DEC(_) (_)->Release();
#define _APT_DECSAFE(_) \
    if (_)              \
    {                   \
        (_)->Release(); \
    }
#define _APT_INC(_) (_)->AddRef();
#define _APT_INCSAFE(_) \
    if (_)              \
    {                   \
        (_)->AddRef();  \
    }
#endif

#if !defined(APTLIBRARY)

extern void AptUpdateLock(); // Only to be called by APT_INC/APT_DEC.
extern void AptUpdateUnlock();

//
// For external users: These are the APT_INC and APT_DEC macros you should use. They are thread-safe.
//
#define APT_DEC(_)         \
    {                      \
        AptUpdateLock();   \
        _APT_DEC(_);       \
        AptUpdateUnlock(); \
    }
#define APT_DECSAFE(_)     \
    {                      \
        AptUpdateLock();   \
        _APT_DECSAFE(_);   \
        AptUpdateUnlock(); \
    }
#define APT_INC(_)         \
    {                      \
        AptUpdateLock();   \
        _APT_INC(_);       \
        AptUpdateUnlock(); \
    }
#define APT_INCSAFE(_)     \
    {                      \
        AptUpdateLock();   \
        _APT_INCSAFE(_);   \
        AptUpdateUnlock(); \
    }

#else

// Used internally when building the Apt runtime.
#define APT_DEC(_) _APT_DEC(_)
#define APT_DECSAFE(_) _APT_DECSAFE(_)
#define APT_INC(_) _APT_INC(_)
#define APT_INCSAFE(_) _APT_INCSAFE(_)

#endif // APTLIBRARY