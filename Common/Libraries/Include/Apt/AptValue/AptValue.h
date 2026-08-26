/**
 * Defines the building block objects used in apt. This includes the AptValue and the base types direcly inheriting from it. It also defined the basic data types needed by AptValue.
 */

#pragma once
#include "Apt.h"
#include "AptString/EAString.h"
#include "AptDefine.h"

// GetClassName is a windows define. Any game code that includes Apt headers will always have window headers included.
// So, we are undefining it here to keep the precompiler from replacing it everywhere we use it.
#if APT_PLATFORM_WINDOWS
#ifdef GetClassName
#undef GetClassName
#endif // GetClassName
#endif // APT_PLATFORM_WINDOWS

//--------------------------------------------------
// these indices are exported from Perl (as noted in comments below). Do not change values used by the exporter!!!
// These values also need to match up with AptValueSizesByVType define from Apt.cpp in order for dogma to work properly.
// Another array tied to these is gVFTNameStrings in Apt.cpp thish is used in debug prints.
enum AptVirtualFunctionTable_Indices
{
    AptVFT_xxx            = 0,
    AptVFT_StringValue    = 1, // Used in AptOutput.pm
    AptVFT_Property       = 2, // Used in AptOutput.pm
    AptVFT_None           = 3, // Used in AptOutput.pm
    AptVFT_Register       = 4, // Used in AptOutput.pm
    AptVFT_Boolean        = 5, // Used in AptOutput.pm
    AptVFT_Float          = 6, // Used in AptOutput.pm
    AptVFT_Integer        = 7, // Used in AptOutput.pm
    AptVFT_Lookup         = 8, // Used in AptOutput.pm
    AptVFT_NativeFunction = 9,
    AptVFT_FrameStack     = 10,
    AptVFT_Extern         = 11, // Used in AptOutput.pm
    AptVFT_CharacterInstHandle, // The Rest can be changed as needed.
    AptVFT_Sound,
    AptVFT_Array,
    AptVFT_Math,
    AptVFT_Key,
    AptVFT_Global,
    AptVFT_ScriptColour,
    AptVFT_Object,
    AptVFT_Prototype,
    AptVFT_Date,
    AptVFT_MovieClip,
    AptVFT_Mouse,
    AptVFT_XmlNode,
    AptVFT_Xml,
    AptVFT_XmlAttributes,
    AptVFT_LoadVars,
    AptVFT_TextFormat,
    AptVFT_Extension,
    AptVFT_GlobalExtension,
    AptVFT_Stage,
    AptVFT_Error,        // Added for Flash Player 7 / AS 2.0 support (Release 17.0)
    AptVFT_StringObject, // Added as an AptObject wrapper to AptString
    AptVFT_ScriptFunction1,
    AptVFT_ScriptFunction2,
    AptVFT_ScriptFunctionByteCodeBlock,
    AptVFT_CIHNone, // Added for ReplaceReferencesCb
    AptVFT_MovieClipLoader,
    AptVFT_AptUtil,
    AptVFT_ExternalFunction,
    AptVFT_AltInput,
    AptVFT_NumVFTs
};

//--------------------------------------------------
// Forward declarations.
class AptCharacterInst;
class AptCharacterShapeInst;
class AptCharacterSpriteInst;
class AptCharacterTextInst;
class AptCharacterStaticTextInst;
class AptCharacterMorphInst;
class AptCharacterButtonInst;
class AptCharacterAnimationInst;
class AptCharacterSpriteInstBase;
class AptCharacterLevelInst;

struct AptRenderingContext;
struct AptRect;
struct AptDisplayListState;

class AptCIH;
class AptFrameStack;
class AptObject;
class AptNativeHash;
class AptString;
class AptBoolean;
class AptExternalFunction;
class AptNativeFunction;
class AptScriptFunction;
class AptScriptFunctionBase;
class AptLookup;
class AptInteger;
class AptRegister;
class AptFloat;
class AptNativeHash;
class AptArray;
class AptSound;
class AptKey;
class AptMouse;
class AptMathObj;
class AptNone;
class AptCIHNone;
class AptScriptColour;
class AptObject;
class AptPrototype;
class AptDate;
class AptGlobal;
class AptTextFormat;
class AptMovieClip;
class AptMovieClipLoader;
class AptXmlNode;
class AptXml;
class AptXmlAttributes;
class AptValueVector;
class AptLoadVars;
class AptStage;
class AptAlternateInput;
class AptUtil;

// ##############################################################################
//
/**
 * @brief Class AptValue is the basic type for Apt. Pretty much everything we use in
 * Apt inherits from this object definition. (this is also a purely virtual class
 * in that you cannot instantiate an AptValue Directly.
 */
class AptValue
{
  public:
    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // Public functions. These are the functions available for external use.

    // Added Metric defines

    //--------------------------------------------------------------------------
    // Conversions to native Types
    void toString(char *szBuf) const;
    void toString(AptNativeString &sBuf) const;
    int toInteger() const;
    float toFloat() const;
    bool toBool() const; // Added to standardize the conversion to booleans (Release 17.0)

    const AptNativeString *Get_ToString(AptNativeString &sPotentialBuffer);
    void Append_ToString(AptNativeString &sString) const;

    // Increment / Decrement messages. Turn them on with APT_INC_DEC_MESSAGES
    // Users, please do not call these functions directly, use the APT_INC and APT_DEC macros
    // to ensure the correct parameters are passed in all builds!
#if defined(APT_INC_DEC_MESSAGES)
    virtual void AddRef(const char *szFuncName, const char *szFileName, int nLineNumber);
    virtual void Release(const char *szFuncName, const char *szFileName, int nLineNumber);
#else
    virtual void AddRef();
    virtual void Release();
#endif

    //--------------------------------------------------------------------------
    // Basic Getter / Setters

    /** @brief Get the reference count */
    APT_FORCE_INLINE uint32_t getRefCount() const
    {
        return mValueBitfield.mnReferenceCount;
    }

    /** @brief Get the vtbl index (type) of the AptValue object */
    APT_FORCE_INLINE AptVirtualFunctionTable_Indices getVtblIndex() const
    {
        return mValueBitfield.meValueType;
    }

    /** @brief Returns whether or not the object is defined */
    APT_FORCE_INLINE bool getIsDefined() const
    {
        return mValueBitfield.mbIsDefined != 0;
    }

    /** @brief returns true if the object is marked for garbage collection */
    APT_FORCE_INLINE bool getGCMark() const
    {
        return mValueBitfield.mbHasRegisterReferenceMark != 0;
    }

    /** @brief Get the value of the GCRoot counter */
    APT_FORCE_INLINE uint32_t getGCRoot() const
    {
        return mValueBitfield.mnGCRootCount;
    }

    /** @brief Checks if the flag that MaxRef was hit */
    APT_FORCE_INLINE bool GetMaxRefCountHit() const
    {
        return mValueBitfield.mnMaxRefCountHit != 0;
    }

    /** @brief Sets the flag that MaxRef was hit */
    APT_FORCE_INLINE void SetMaxRefCountHit(bool bFlag)
    {
        mValueBitfield.mnMaxRefCountHit = (uint32_t)(bFlag ? 1 : 0);
    }

    /** @brief returns true if the object is in the deffered vector */
    APT_FORCE_INLINE bool IsReleaseAtEnd() const
    {
        return mValueBitfield.mbIsInDeferredVector != 0;
    }

    // Setters, these are not inlined because they need to aquire mutexes and such in decoupled mode.
    void setRefCount(uint32_t n);
    void setVtblIndex(AptVirtualFunctionTable_Indices n);
    void setIsDefined(bool bDefined);
    void setGCMark(bool bMark);
    void setGCRoot(uint32_t nRoot);
    void SetReleaseAtEnd();
    void ClearReleaseAtEnd();
    void incGCRoot();
    void decGCRoot();

    //--------------------------------------------------------------------------
    // Typecasts (these do type checking)
    // These cannot be implemented here because then we would need to expose all of the types.
    // Instead, we have them in a .inl file.
    APT_INLINE AptLookup *c_lookup() const;
    APT_INLINE AptInteger *c_integer() const;
    APT_INLINE AptRegister *c_register() const;
    APT_INLINE AptFloat *c_float() const;
    AptString *c_string() const; // Not inlined because non-trivial
    APT_INLINE AptBoolean *c_boolean() const;
    APT_INLINE AptScriptFunctionBase *c_scriptfunction() const;
    APT_INLINE AptNativeFunction *c_nativefunction() const;
    APT_INLINE AptExternalFunction *c_externalfunction() const;
    APT_INLINE AptCIH *c_cih(bool bUndefOK = false) const;

    AptArray *c_array() const
    {
        APT_ASSERT(isArray());
        return (AptArray *)this;
    }

    APT_INLINE AptSound *c_sound() const;
    APT_INLINE AptKey *c_key() const;
    APT_INLINE AptMouse *c_mouse() const;
    APT_INLINE AptGlobal *c_global() const;
    APT_INLINE AptMathObj *c_math() const;
    APT_INLINE AptScriptColour *c_scriptcolour() const;
    APT_INLINE AptObject *c_object() const;
    APT_INLINE AptPrototype *c_prototype() const;
    APT_INLINE AptDate *c_date() const;
    APT_INLINE AptTextFormat *c_textformat() const;
    APT_INLINE AptMovieClip *c_movieClip() const;
    APT_INLINE AptXmlNode *c_xmlnode() const;
    APT_INLINE AptXml *c_xml() const;
    APT_INLINE AptXmlAttributes *c_xmlattributes() const;
    APT_INLINE AptLoadVars *c_loadvars() const;
    APT_INLINE AptStage *c_stage() const;
    APT_INLINE AptMovieClipLoader *c_movieClipLoader() const;
    APT_INLINE AptAlternateInput *c_altinput() const;
    APT_INLINE AptUtil *c_util() const;

    //--------------------------------------------------------------------------
    // these return true if the object is of the specified type.

    APT_FORCE_INLINE bool isXmlNode() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_XmlNode;
    }
    APT_FORCE_INLINE bool isXml() const
    {
        APT_ASSERT(this);
        return (getVtblIndex() == AptVFT_Xml);
    }
    APT_FORCE_INLINE bool isXmlAttributes() const
    {
        APT_ASSERT(this);
        return (getVtblIndex() == AptVFT_XmlAttributes);
    }
    APT_FORCE_INLINE bool isLoadVars() const
    {
        APT_ASSERT(this);
        return (getVtblIndex() == AptVFT_LoadVars);
    }
    APT_FORCE_INLINE bool isNone() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_None;
    }
    APT_FORCE_INLINE bool isUndefined() const
    {
        APT_ASSERT(this);
        return !getIsDefined();
    }
    APT_FORCE_INLINE bool isLookup() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Lookup && !isUndefined();
    }
    APT_FORCE_INLINE bool isString() const
    {
        APT_ASSERT(this);
        AptVirtualFunctionTable_Indices t = getVtblIndex();
        return ((t == AptVFT_StringValue) || (t == AptVFT_StringObject)) && !isUndefined();
    }
    APT_FORCE_INLINE bool isBoolean() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Boolean && !isUndefined();
    }
    APT_FORCE_INLINE bool isInteger() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Integer && !isUndefined();
    }
    APT_FORCE_INLINE bool isRegister() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Register && !isUndefined();
    }
    APT_FORCE_INLINE bool isFloat() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Float && !isUndefined();
    }
    APT_FORCE_INLINE bool isNativeFunction() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_NativeFunction && !isUndefined();
    }
    APT_FORCE_INLINE bool isScriptFunction() const
    {
        APT_ASSERT(this);
        AptVirtualFunctionTable_Indices eType = getVtblIndex();
        return eType >= AptVFT_ScriptFunction1 && eType <= AptVFT_ScriptFunctionByteCodeBlock && !isUndefined();
    }
    APT_FORCE_INLINE bool isExternalFunction() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_ExternalFunction && !isUndefined();
    }
    APT_FORCE_INLINE bool isFunction() const
    {
        APT_ASSERT(this);
        return isScriptFunction() || isNativeFunction() || isExternalFunction();
    }
    APT_FORCE_INLINE bool isExtern() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Extern && !isUndefined();
    }
    APT_FORCE_INLINE bool isFrameStack() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_FrameStack && !isUndefined();
    }
    APT_FORCE_INLINE bool isArray() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Array && !isUndefined();
    }
    APT_FORCE_INLINE bool isSound() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Sound && !isUndefined();
    }
    APT_FORCE_INLINE bool isKey() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Key && !isUndefined();
    }
    APT_FORCE_INLINE bool isMouse() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Mouse && !isUndefined();
    }
    APT_FORCE_INLINE bool isMath() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Math && !isUndefined();
    }
    APT_FORCE_INLINE bool isScriptColour() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_ScriptColour && !isUndefined();
    }
    APT_FORCE_INLINE bool isCIH(bool bUndefOK = false) const
    {
        APT_ASSERT(this);
        AptVirtualFunctionTable_Indices eType = getVtblIndex();
        return ((eType == AptVFT_CharacterInstHandle) && (bUndefOK || (!isUndefined()))) || eType == AptVFT_CIHNone;
    }
    APT_FORCE_INLINE bool isObject() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Object && !isUndefined();
    }
    APT_FORCE_INLINE bool isPrototype() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Prototype && !isUndefined();
    }
    APT_FORCE_INLINE bool isDate() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Date && !isUndefined();
    }
    APT_FORCE_INLINE bool isMovieClipLoader() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_MovieClipLoader && !isUndefined();
    }
    APT_FORCE_INLINE bool isTextFormat() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_TextFormat && !isUndefined();
    }
    APT_FORCE_INLINE bool isMovieClip() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_MovieClip && !isUndefined();
    }
    APT_FORCE_INLINE bool isStage() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_Stage && !isUndefined();
    }
    APT_FORCE_INLINE bool isAptUtil() const
    {
        APT_ASSERT(this);
        return getVtblIndex() == AptVFT_AptUtil && !isUndefined();
    }

    APT_FORCE_INLINE bool isNumber() const
    {
        APT_ASSERT(this);
        return isFloat() || isInteger();
    }

    void dumpToString(AptNativeString &sBuf);

    // These exist for opt call stack scope information
    virtual const char *GetClassName() const;
    virtual void SetClassName(const char *name);

    //$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$
    // The following functions are present for internal use. Please do not use them in your games!
    // The functionality of these functions can change from apt one version to another.
    // APT_ACCESS_INTERNAL translates into "public" when building the apt library and "protected" in other cases.
    APT_ACCESS_INTERNAL :

        //--------------------------------------------------------------------------
        // Common Native Hash Functions (Inherited Types can override)
        virtual AptNativeHash *GetNativeHashVirtual()
    {
        return (NULL);
    }

    virtual bool ContainsNativeHashVirtual() const
    {
        return (false);
    }

    //--------------------------------------------------------------------------
    // Common Class functionality (Inherited Types can override)
    virtual bool GetHasClass() const
    {
        return false;
    }

    //--------------------------------------------------------------------------
    // Common Class functionality (Inherited Types can override)
    virtual void SetHasClass(int)
    {
        return;
    }

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    // These functions are used to validate the state of the object.
    // SetDestroyedGC is deliberately not inlined: APT_INLINE made it disappear from the headers
    // in a way that caused link errors for some consumers.
    void SetDestroyedGC();

    APT_INLINE void ClearDestroyedGC();
    APT_INLINE bool IsDestroyedGC() const
    {
        return (bool)(mValueBitfield.mbDestroyedGC != 0);
    }
#endif


    //--------------------------------------------------------------------------
    // Added for Flash Player 7 / AS 2.0 support (Release 17.0)
    bool CanCreateScriptObject() const;

    //--------------------------------------------------------------------------
    // Common Class functionality (Inherited Types can override)

    AptValue *findChild(const AptNativeString *pName, AptValue *pWith, bool bIsMember = false);

    virtual AptValue *objectMemberLookup(AptValue *const, const AptNativeString *const) const
    {
        return (NULL);
    }
    virtual bool objectMemberSet(AptValue *const, const AptNativeString *const, AptValue *const)
    {
        return (false);
    }

    //--------------------------------------------------------------------------
    // Misc functions.
    int isMCInParentChain() const; // check if MovieClip.prototype is in hierarchy chain
    AptNativeString urlEncode();
    AptNativeString urlEncodeCustomRender();

    //--------------------------------------------------------------------------
    // This tells us whether or not the object should be put on the deferred vector.
    void SetAllowDelayedDeletion(bool bAllowed);

    //--------------------------------------------------------------------------
    // returns whether or not to place on deferred vector or delete immediately
    bool GetAllowDelayedDeletion()
    {
        return mValueBitfield.mbAllowsDelayedDeletion != 0;
    }

    //--------------------------------------------------------------------------
    // Does necessary cleanup and deletes, allows certain object to delete actuall
    // deletion and pool themselves.
    virtual void DeleteThis()
    {
        delete this;
    }

    //--------------------------------------------------------------------------
    // This is usually called before DestroyGC pointers, I don't see why it is
    // needed personally, but it is here.
    virtual void PreDestroy()
    {
        //  Do nothing...
    }

    //--------------------------------------------------------------------------
    // This is called before we delete the object (but only if we want it to
    // clean up it's pointers)
    virtual void DestroyGCPointers()
    {
#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
        {
            SetDestroyedGC();
        }
#endif
    }

    //  Method called for global object not reference counted
    virtual void ForceDelete()
    {
        PreDestroy();
        DestroyGCPointers();
        delete this;
    }

    int32_t IncrementAptValue();
    int32_t DecrementAptValue();

    static void (*sReferenceRegistrationCb)(const AptValue *pFromRef, AptValue *&pToRef, const char *pReferenceName, int32_t bFlag);

    virtual bool IsGarbageCollected() const = 0;
    virtual void RegisterReferences()       = 0;

    const static uint32_t MAX_REFCOUNT = 0xfff;
    const static uint32_t MAX_GCROOT   = 0x3f;

    union
    {
        struct
        {
            uint32_t mbIsAllocated : 1; // This MUST match up with the bit in AptValueGC_MemItem or bad things will happen!
            uint32_t mbHasRegisterReferenceMark : 1;
            uint32_t mbIsInDeferredVector : 1;
            uint32_t mbDestroyedGC : 1;
            uint32_t mbIsDefined : 1;             // Used for debugging purposes
            uint32_t mbAllowsDelayedDeletion : 1; // true = Object can be placed on the Deferred Vector.
            uint32_t mnReferenceCount : 12;
            uint32_t mnGCRootCount : 6;    // Stole a bit to create mnMaxRefCountHit
            uint32_t mnMaxRefCountHit : 1; // gpObjectPrototype deleted early due to MAX_REFCOUNT being reached
            enum AptVirtualFunctionTable_Indices meValueType : 7;

        } mValueBitfield;

        uint32_t mnValueData;
    };

#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)
    // Added so that we can have XML dumps without the extra 4 bytes.
    // Basically the idea is that unless you are trending object progression throughout time
    // you really don't need a globally unique AptValueID, it just needs to be unique within a
    // single dump. So if you need unigue ID's across multiple dumps, turn on APT_XML_EXTENDED_MEMORY_DUMP_SUPPORTED
    // and eat the four extra bytes per object, if you just want to look at dumps as independent states,
    // Then the pointer value will be unique within the dump :)
    uintptr_t GetUniqueID() const
    {
#if defined(APT_XML_EXTENDED_MEMORY_DUMP_SUPPORTED)
        return mnID;
#else
        return (uintptr_t)this;
#endif
    }
#endif

  protected:
    enum CIH_ONLY
    {
        CO_CIH,
    };

    AptValue(AptVirtualFunctionTable_Indices eType);
    AptValue(AptVirtualFunctionTable_Indices eType, const CIH_ONLY eCIH);

    APT_INLINE virtual ~AptValue()
    {
#if APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL
        APT_ASSERT(IsDestroyedGC() == true);
#endif

    }

#if defined(APT_XML_EXTENDED_MEMORY_DUMP_SUPPORTED)
    static uint32_t snCurrentAllocationNumber;
    uint32_t mnID;
#endif

  private:
    static bool sbSuspendRefcountDeletions;
    friend class AptGC;

    void _ForceDeleteIfNecessary(); // Helper function for Release() to allow the compiler to optimize better.

};

#include "AptValueGCAllocator.h"

// ##############################################################################
//
/**
 * @brief Class AptValueGC is a base class for all garbage collected items.
 * This keeps us from duplicating a bunch of functions everywhere.
 */
class AptValueGC : public AptValue
{
    APT_ACCESS_INTERNAL :
        // Added Metric defines

        AptValueGC(AptVirtualFunctionTable_Indices eType) : AptValue(eType) {
                                                            };

    AptValueGC(AptVirtualFunctionTable_Indices eType, const CIH_ONLY eCIH) : AptValue(eType, eCIH) {
                                                                             };

    virtual bool IsGarbageCollected() const
    {
        return (true);
    }

  protected:
    virtual ~AptValueGC() {};
    /** @brief Called by derived classes to ensure they are based on AptValueGC. should be optimized out in release. */
    static void VerifyAptValueGC()
    {
    }
};

// ##############################################################################
//
/**
 * @brief Class AptValueNoGC is a base class for all non garbage collected items.
 * This keeps us from duplicating a bunch of functions everywhere.
 */
class AptValueNoGC : public AptValue
{
    APT_ACCESS_INTERNAL :
        // Added Metric defines

        AptValueNoGC(AptVirtualFunctionTable_Indices eType) : AptValue(eType) {
                                                              };

    // Not Garbage Collected.
    virtual bool IsGarbageCollected() const
    {
        return (false);
    }

    // Does nothing.
    virtual void RegisterReferences()
    {
        return;
    }

  protected:
    /** @brief Called by derived classes to ensure they are based on AptValueGC. should be optimized out in release. */
    static void VerifyAptValueNoGC()
    {
    }
};

#define TO_STRING(VALUE, PSTR)   \
    AptNativeString PSTR##_buf_; \
    const AptNativeString *PSTR = VALUE->Get_ToString(PSTR##_buf_);
