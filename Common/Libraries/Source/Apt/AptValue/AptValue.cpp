#include "_Apt.h"
#include "string/StringPool.h"
#include "Display/AptRenderingContext.h"
#include "AptFrameStack.h"
#include "AptGlobal.h"
#include "AptValue/AptValueVector.h"
#include "AptObject/AptStringObject.h"
#include "AptObject/AptDate.h"
#include "AptObject/AptMathObj.h"
#include "AptObject/AptKey.h"
#include "AptObject/AptLoadVars.h"
#include "AptObject/AptSound.h"
#include "AptObject/AptMouse.h"
#include "AptObject/AptError.h"
#include "AptObject/AptStage.h"
#include "AptObject/AptUtil.h"
#include "AptObject/AptGlobalObject.h"
#include "AptObject/AptGlobalExtensionObject.h"
#include "MainInline.h"

#if !defined(APT_ENABLE_INLINE)
#include "AptValue/AptValue.inl"
#endif

// gperf indices from objects.gperf, used by findChild() below to recognize reserved words.
static const int AptObject_target         = 1;
static const int AptObjectthis            = 2;
static const int AptObject_root           = 3;
static const int AptObjectKey             = 4;
static const int AptObjectMath            = 5;
static const int AptObject_level0         = 6;
static const int AptObject_level1         = 7;
static const int AptObject_level2         = 8;
static const int AptObject_level3         = 9;
static const int AptObject_level4         = 10;
static const int AptObject_level5         = 11;
static const int AptObject_level6         = 12;
static const int AptObject_level7         = 13;
static const int AptObject_level8         = 14;
static const int AptObject_level9         = 15;
static const int AptObjectparent          = 16;
static const int AptObjectextern          = 17;
static const int AptObjectsuper           = 18;
static const int AptObject_global         = 19;
static const int AptObjectMouse           = 20;
static const int AptObject_level10        = 21;
static const int AptObject_level11        = 22;
static const int AptObject_level12        = 23;
static const int AptObject_level13        = 24;
static const int AptObject_level14        = 25;
static const int AptObject_level15        = 26;
static const int AptObject_level16        = 27;
static const int AptObject_level17        = 28;
static const int AptObject_level18        = 29;
static const int AptObject_level19        = 30;
static const int AptObject_level20        = 31;
static const int AptObject_level21        = 32;
static const int AptObject_level22        = 33;
static const int AptObject_level23        = 34;
static const int AptObject_level24        = 35;
static const int AptObject_String         = 36;
static const int AptObject_Stage          = 37;
static const int AptObject_AlternateInput = 38;
static const int AptObject_AptUtil        = 39;

AptMatrix gIdentityMatrix;
AptCXForm gIdentityCXForm;
AptNativeFunction *gpCBsetInterval;
AptNativeFunction *gpCBclearInterval;
AptNativeFunction *gpCBisNaN;
AptNativeFunction *gpCBunescape;
AptNativeFunction *gpCBescape;
AptNativeFunction *gpCBboolean;
AptNativeFunction *gpASSetPropFlags;
AptNativeFunction *gpCBparseInt;
AptNativeFunction *gpCBparseFloat;

#if APT_USE_MATH_OBJECT
AptMathObj *gpGlobalMathObject;
#endif
AptKey *gpGlobalKeyObject;
#if defined(APT_USE_MOUSE)
AptMouse *gpGlobalMouseObject;
#endif
AptNone *gpUndefinedValue;
AptCIHNone *gpUndefinedCIH;
#if defined(APT_USE_STAGE_OBJECT)
AptStage *gpGlobalStageObject;
#endif
AptString *gpGlobalStringObject; // Global String Object added for Flash Player 7 / AS 2.0 support.

#if APT_USE_UTILITY
AptUtil *gpGlobalAptUtilObject;
#endif
#if defined(APT_ALTERNATE_INPUT)
AptAlternateInput *gpGlobalAltInputObject;
#endif

AptValue *gpGlobalObjectPrototype;
AptValue *gpGlobalMovieclipPrototype;

// Keeps the hash table of Object.registerClass() registrations; created lazily on first use.

int gbLeaksInPool;

// #define CHECK_REFCOUNT
#ifdef CHECK_REFCOUNT
AptValue *mValuesToTest[10] =
    {
        (AptValue *)0x81629408,
        (AptValue *)0,
        (AptValue *)0,
        (AptValue *)0,
        (AptValue *)0,
        (AptValue *)0,
        (AptValue *)0,
        (AptValue *)0,
        (AptValue *)0,
        (AptValue *)0};
#endif


bool AptValue::sbSuspendRefcountDeletions = false;

void (*AptValue::sReferenceRegistrationCb)(const AptValue *pFromRef, AptValue *&pToRef, const char *pReferenceName, int32_t bFlag) = NULL;

#if defined(APT_XML_EXTENDED_MEMORY_DUMP_SUPPORTED)
uint32_t AptValue::snCurrentAllocationNumber = 0;
#endif


/**
 * @param eType vtbl index (type) of the object.
 */
AptValue::AptValue(AptVirtualFunctionTable_Indices eType)
{
    // The allocator sets this bit on the raw block before we are constructed, but the bits of a
    // block that is not yet an object are indeterminate, so an optimiser is free to drop it when it
    // merges the stores below into one word. Being constructed is what "allocated" means, so say so.
    mValueBitfield.mbIsAllocated = 1;

    setVtblIndex(eType);
    setRefCount(0);
    setIsDefined(1);
    setGCMark(false);
    setGCRoot(0);
    SetAllowDelayedDeletion(true);

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    ClearDestroyedGC();
#endif
#if defined(APT_XML_EXTENDED_MEMORY_DUMP_SUPPORTED)
    mnID = snCurrentAllocationNumber++;
#endif
    //  All newly created values are flagged so they are automatically destroyed at the end of the
    //  frame if not used anymore. Extension objects and native functions are excluded because they
    //  should not be placed in the deferred vector.
    if (eType == AptVFT_Prototype || eType == AptVFT_ScriptFunction1 || eType == AptVFT_ScriptFunction2 || eType == AptVFT_NativeFunction || eType == AptVFT_Extension || eType == AptVFT_ExternalFunction)
    {
        ClearReleaseAtEnd();
    }
    else
    {
        SetReleaseAtEnd();
        AptGetLib()->mpValuesToRelease->PushValue(this);
    }
    SetMaxRefCountHit(false);
}

/**
 * CIH constructor: the pointer is not pushed onto the deferred release vector.
 * @param eType vtbl index (type) of the object.
 * @param eCIH always CO_CIH.
 */
AptValue::AptValue(AptVirtualFunctionTable_Indices eType, const CIH_ONLY eCIH)
{
    // See the other constructor: the allocator's pre-construction bit cannot be relied on.
    mValueBitfield.mbIsAllocated = 1;

    setVtblIndex(eType);
    setRefCount(0);
    setIsDefined(1);
    setGCMark(false);
    setGCRoot(0);
    SetAllowDelayedDeletion(false);
#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    ClearDestroyedGC();
#endif

#if defined(APT_XML_EXTENDED_MEMORY_DUMP_SUPPORTED)
    mnID = snCurrentAllocationNumber++;
#endif
    SetMaxRefCountHit(false);
    ClearReleaseAtEnd();
}

/**
 * @param bAllowed whether the object is allowed to be put on the deferred vector.
 */
void AptValue::SetAllowDelayedDeletion(bool bAllowed)
{
    mValueBitfield.mbAllowsDelayedDeletion = bAllowed ? 1 : 0;
}

/**
 * Sets the object's reference count, clamping to MAX_REFCOUNT.
 */
void AptValue::setRefCount(uint32_t n)
{
    // This does not fix the reason why we got to uMaxRefCount, but it prevents us from crashing
    // when ref counts would roll back to 0. This still asserts in Debug mode to help us address
    // possible ref counting issues.
    if (n > MAX_REFCOUNT)
    {
        SetMaxRefCountHit(true);
        n = MAX_REFCOUNT;
    }

    mValueBitfield.mnReferenceCount = n;
}

/**
 * Sets the vtbl index (type) of the object.
 */
void AptValue::setVtblIndex(AptVirtualFunctionTable_Indices n)
{
    APT_ASSERT(n > AptVFT_xxx);
    APT_ASSERT(n < AptVFT_NumVFTs);
    mValueBitfield.meValueType = n;
}

/**
 * @param bDefined true to mark as defined, false to mark as undefined.
 */
void AptValue::setIsDefined(bool bDefined)
{
    mValueBitfield.mbIsDefined = bDefined ? 1 : 0;
}

/**
 * Sets the GC mark value for the object.
 */
void AptValue::setGCMark(bool bMark)
{
    mValueBitfield.mbHasRegisterReferenceMark = bMark ? 1 : 0;
}

/**
 * Sets the GC root counter for the object.
 */
void AptValue::setGCRoot(uint32_t nRoot)
{
    APT_ASSERT(nRoot <= MAX_GCROOT);
    mValueBitfield.mnGCRootCount = nRoot;
}

/**
 * Increments the GC root counter for the object.
 */
void AptValue::incGCRoot()
{
    APT_ASSERT(mValueBitfield.mnGCRootCount < MAX_GCROOT);

    if (mValueBitfield.mnGCRootCount < MAX_GCROOT)
        mValueBitfield.mnGCRootCount++;
}

/**
 * Decrements the GC root counter for the object.
 */
void AptValue::decGCRoot()
{
    APT_ASSERT(mValueBitfield.mnGCRootCount > 0);
    if (mValueBitfield.mnGCRootCount > 0)
        mValueBitfield.mnGCRootCount--;
}

/**
 * Sets the name of the class this AptValue represents.
 */
void AptValue::SetClassName(const char *name)
{
    AptNativeHash *pHash = const_cast<AptValue *>(this)->GetNativeHashVirtual();
    if (pHash)
    {
        AptPrototype *pPrototype = (AptPrototype *)pHash->GetPrototype();
        if (pPrototype)
            pPrototype->SetClassName(name);
    }
}

/**
 * @return the name of the class this AptValue represents, or NULL if unknown.
 */
const char *AptValue::GetClassName() const
{
    AptNativeHash *pHash = const_cast<AptValue *>(this)->GetNativeHashVirtual();
    if (pHash)
    {
        AptPrototype *pPrototype = (AptPrototype *)pHash->GetPrototype();
        const char *name         = pPrototype ? pPrototype->GetClassName() : 0;
        if (name)
            return name;
    }
    AptVirtualFunctionTable_Indices index = mValueBitfield.meValueType;
    switch (index)
    {
    case AptVFT_StringValue:
    case AptVFT_StringObject:
        return "String";
    case AptVFT_Property:
        return "Property";
    case AptVFT_CIHNone:
    case AptVFT_None:
        return "None";
    case AptVFT_Register:
        return "Register";
    case AptVFT_Boolean:
        return "Boolean";
    case AptVFT_Float:
        return "Float";
    case AptVFT_Integer:
        return "Integer";
    case AptVFT_Lookup:
        return "Lookup";
    case AptVFT_ScriptFunction1:
    case AptVFT_ScriptFunction2:
    case AptVFT_ScriptFunctionByteCodeBlock:
        return "Function";
    case AptVFT_NativeFunction:
        return "NativeFunction";
    case AptVFT_FrameStack:
        return "FrameStack";
    case AptVFT_Extern:
        return "Extern";
    case AptVFT_CharacterInstHandle:
        return "CharacterInstHandle";
    case AptVFT_Sound:
        return "Sound";
    case AptVFT_Array:
        return "Array";
    case AptVFT_Math:
        return "Math";
    case AptVFT_Key:
        return "Key";
    case AptVFT_Global:
        return "Global";
    case AptVFT_ScriptColour:
        return "ScriptColor";
    case AptVFT_Object:
        return "Object";
    case AptVFT_Prototype:
        return "Prototype";
    case AptVFT_Date:
        return "Date";
    case AptVFT_MovieClip:
        return "MovieClip";
    case AptVFT_Mouse:
        return "Mouse";
    case AptVFT_XmlNode:
        return "XmlNode";
    case AptVFT_Xml:
        return "Xml";
    case AptVFT_XmlAttributes:
        return "XmlAttributes";
    case AptVFT_LoadVars:
        return "LoadVars";
    case AptVFT_TextFormat:
        return "TextFormat";
    case AptVFT_Extension:
        return "Extension";
    case AptVFT_GlobalExtension:
        return "GlobalExtension";
    case AptVFT_Stage:
        return "Stage";
    case AptVFT_Error:
        return "Error";
    case AptVFT_MovieClipLoader:
        return "MovieClipLoader";
    case AptVFT_AptUtil:
        return "AptUtil";
    case AptVFT_ExternalFunction:
        return "ExternalFunction";
    default:
        return NULL;
    };
}

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)

void AptValue::SetDestroyedGC()
{
    mValueBitfield.mbDestroyedGC = 1;
}

void AptValue::ClearDestroyedGC()
{
    mValueBitfield.mbDestroyedGC = 0;
}

#endif

void AptValue::SetReleaseAtEnd()
{
    mValueBitfield.mbIsInDeferredVector = 1;
}

void AptValue::ClearReleaseAtEnd()
{
    mValueBitfield.mbIsInDeferredVector = 0;
}

/**
 * @return the underlying AptString for a string-typed value.
 */
AptString *AptValue::c_string() const
{
    APT_ASSERT(isString());
    AptVirtualFunctionTable_Indices t = getVtblIndex();

    if (t == AptVFT_StringValue)
    {
        return (AptString *)this;
    }
    else
    {
        return ((AptStringObject *)this)->mpStringObject;
    }
}

/**
 * Converts the AptValue to an integer.
 */
int AptValue::toInteger(void) const
{
    if (isUndefined())
    {
        return 0;
    }

    switch (getVtblIndex())
    {
    case AptVFT_StringObject:
    case AptVFT_StringValue:
    {
        AptString *pString    = c_string();
        AptNativeString *sBuf = pString->GetInternalString();
        if (sBuf->Size() > 2 && sBuf->operator[](0) == '0' && sBuf->operator[](1) == 'x') // Hex string values
        {
            return strtol(sBuf->c_str(), NULL, 16);
        }
        return atoi(sBuf->c_str());
    }
    break;
    case AptVFT_Boolean:
    {
        return ((AptBoolean *)this)->GetBool();
    }
    break;
    case AptVFT_Integer:
    {
        return ((AptInteger *)this)->GetInt();
    }
    break;
    case AptVFT_Float:
    {
        float fVal = (((AptFloat *)this)->GetFloat());
        APT_ASSERT((fVal <= (float)INT32_MAX && fVal >= (float)INT32_MIN) && "This float value does not lie in the integer value range; this may cause unexpected behaviour.");
        return (fVal > (float)INT32_MAX) ? INT32_MAX : (fVal < (float)INT32_MIN) ? INT32_MIN
                                                                                 : (int32_t)fVal;
    }
    break;
    default:
    {
        // Objects are "true" if they are defined (or in this case not undefined)
        return (this != gpUndefinedValue) ? 1 : 0;
    }
    }
}

/**
 * Converts the AptValue to a float.
 */
float AptValue::toFloat(void) const
{
    if (isUndefined())
    {
        return 0.f;
    }

    switch (getVtblIndex())
    {
    case AptVFT_StringValue:
    case AptVFT_StringObject:
    {
        return (float)Apt_atoff(c_string()->GetInternalString()->c_str());
    }
    break;
    case AptVFT_Boolean:

    {
        return c_boolean()->GetBool() ? 1.0f : 0.0f;
    }
    break;
    case AptVFT_Integer:

    {
        return (float)c_integer()->GetInt();
    }
    break;
    case AptVFT_Float:

    {
        return c_float()->GetFloat();
    }
    break;
    default:
    {
        // Objects are "true" if they are defined (or in this case not undefined)
        return (this != gpUndefinedValue) ? 1.0f : 0.0f;
    }
    }
}

/**
 * Converts the AptValue to a boolean. Note that this conversion is slightly different for Flash 7
 * files.
 */
bool AptValue::toBool(void) const
{
    switch (getVtblIndex())
    {
    case AptVFT_StringValue:
    case AptVFT_StringObject:
    {
        if (AptGetSwfVersion() >= 7)
        {
            // Much simpler conversion in Flash 7 thankfully.
            return !c_string()->str.IsEmpty();
        }
        else
        {
            // Must convert to Int or string in Flash 6 (which sux).
            // Hex numbers (must be integers) are handled first; if not hex, convert to a float.
            AptString *pString    = c_string();
            AptNativeString *sBuf = pString->GetInternalString();
            if (sBuf->Size() > 2 && sBuf->operator[](0) == '0' && sBuf->operator[](1) == 'x')
            {
                return strtol(sBuf->c_str(), NULL, 16) != 0;
            }
            else
            {
                return Apt_atoff(c_string()->GetInternalString()->c_str()) != 0.0f;
            }
        }
    }
    case AptVFT_Boolean:
    {
        return c_boolean()->GetBool();
    }
    case AptVFT_Integer:
    {
        return c_integer()->GetInt() != 0;
    }
    case AptVFT_Float:
    {
        return c_float()->GetFloat() != 0.0f;
    }
    default:
    {
        // Objects are "true" if they are defined (or in this case not undefined)
        return (this != gpUndefinedValue);
    }
    }
}

void AptValue::toString(char *szBuf) const
{
    AptNativeString sBuf;
    toString(sBuf);
    strcpy(szBuf, sBuf.ConstRawPtr());
}

/**
 * @return the string value of the AptValue for every AptVFT type.
 */
void AptValue::toString(AptNativeString &sBuf) const
{
    if (isUndefined())
    {
        if (AptGetSwfVersion() >= 7)
        {
            sBuf = *StringPool::GetString(SC_Undefined);
        }
        else
        {
            sBuf.Clear();
        }
        return;
    }

    // XXX be careful that all the sprintfs below are able to fit here!
    char szTemp[128];

    switch (getVtblIndex())
    {
    case AptVFT_StringValue:
    case AptVFT_StringObject:
    {
        sBuf = *(c_string()->GetInternalString());
        break;
    }
    case AptVFT_Boolean:
    {
        if (((AptBoolean *)this)->GetBool())
        {
            sBuf = *StringPool::GetString(SC_true);
        }
        else
        {
            sBuf = *StringPool::GetString(SC_false);
        }
        break;
    }
    case AptVFT_Integer:
    {
        sprintf(szTemp, "%d", ((AptInteger *)this)->GetInt());
        sBuf = szTemp;
        break;
    }
    case AptVFT_Float:
    {
        AptFloat *pF = (AptFloat *)this;
        if (fmod(pF->GetFloat(), 1.f) == 0.f)
        {
            sprintf(szTemp, "%d", (int)pF->GetFloat());
        }
        else
        {
            sprintf(szTemp, "%f", pF->GetFloat());
        }
        sBuf = szTemp;
        break;
    }
    case AptVFT_Array:
    {
        AptArray *pA = this->c_array();
        pA->toString(sBuf);
        break;
    }
#if defined(APT_USE_SOUND_OBJECT)
    case AptVFT_Sound:
    {
        sBuf = "[sound]";
        break;
    }
#endif
    case AptVFT_NativeFunction:
    {
        sBuf = "[native function]";
        break;
    }
    case AptVFT_ScriptFunction1:
    case AptVFT_ScriptFunction2:
    case AptVFT_ScriptFunctionByteCodeBlock:
    {
        sBuf = "[function]";
        break;
    }
    case AptVFT_Global:
#if APT_USE_MATH_OBJECT
    case AptVFT_Math:
#endif
    case AptVFT_Key:
    case AptVFT_Object:
#if APT_USE_SCRIPTCOLOUR_OBJECT
    case AptVFT_ScriptColour:
#endif
    case AptVFT_TextFormat:
    case AptVFT_Stage:
#if APT_USE_LOADVARS_OBJECT
    case AptVFT_LoadVars:
#endif
    case AptVFT_Mouse:
    {
        sBuf = "[object Object]";
        break;
    }
    case AptVFT_Xml:
    case AptVFT_XmlAttributes:
    case AptVFT_XmlNode:
    {
        sBuf = "[object]";
        break;
    }
    case AptVFT_Prototype:
    {
        sBuf = "[object (prototype)]";
        break;
    }
    case AptVFT_Date:
    {
        AptDate *pA = this->c_date();
        pA->toString(sBuf);
        break;
    }
    case AptVFT_MovieClip:
    {
        sBuf = "[MovieClip]";
        break;
    }
    case AptVFT_MovieClipLoader:
    {
        sBuf = "[MovieClipLoader]";
        break;
    }
    case AptVFT_Register:
    {
        sBuf = "[Register]";
        break;
    }
    case AptVFT_Lookup:
    {
        sBuf = "[Lookup]";
        break;
    }
    case AptVFT_Extern:
    {
        sBuf = "[Extern]";
        break;
    }
    case AptVFT_FrameStack:
    {
        sBuf = "[FrameStack]";
        break;
    }
    case AptVFT_Extension:
    {
        sBuf = "[Extension]";
        break;
    }
    case AptVFT_GlobalExtension:
    {
        sBuf = "[GlobalExtension]";
        break;
    }
    case AptVFT_Error: // Error Object added for Flash Player 7 / AS 2.0 support.
    {
        AptError *pError = (AptError *)this;
        sBuf             = pError->msMessage;
        break;
    }
    case AptVFT_CharacterInstHandle:
    {
        // GetName only works on these types.
        AptCIH *pCIH = c_cih();
        if (pCIH->GetCharacterInst() == NULL)
            sBuf = pCIH->GetInstanceName();
        else if (pCIH->IsSpriteInstBase() || pCIH->IsButtonInst() || pCIH->IsDynamicTextInst() || pCIH->IsStaticTextInst() || pCIH->IsLevelInst() || pCIH->IsImageInst())
            AptActionInterpreter::getName(c_cih(), sBuf);
        else if (pCIH->IsShapeInst())
            sBuf = "Shape Instance";
        else
            sBuf = "No Instance Name";
        break;
    }
    case AptVFT_ExternalFunction:
    {
        sBuf = "[external function]";
        break;
    }
    default:
    {
        sprintf(szTemp, "[Type=0x%X]", getVtblIndex());
        sBuf = szTemp;
        break;
    }
    }
}

void AptValue::dumpToString(AptNativeString &sBuf)
{
    AptNativeString string_value;
    AptNativeHash *pNativeHash = GetNativeHashVirtual();
    if (pNativeHash)
    {
        AptHashItem *pHashItem = pNativeHash->GetFirstItem();
        sBuf += "{";
        while (pHashItem != NULL)
        {
            sBuf += pHashItem->Key.c_str();
            sBuf += "=";

            pHashItem->mValue->toString(string_value);
            sBuf += string_value;

            pHashItem = pNativeHash->GetNextItem(pHashItem);
            if (pHashItem != NULL)
            {
                sBuf += ",";
            }
        }
        sBuf += "}";
    }
}

static AptValue *_constructorObject(AptValue *pThis, int nParams)
{
    return gpUndefinedValue;
}

/**
 * Called when Object.registerClass() is invoked; registers the hash object with the GC as a root
 * so it remains traceable.
 */
static AptValue *_gObjRegistrationFunc(AptValue *pThis, int nParams)
{
    if (nParams != 2)
        return AptBoolean::Create(false);

    AptValue *pParam1 = gAptActionInterpreter.stackAt(0);
    // assuming that first parameter has to be a quoted string
    if (pParam1->getIsDefined() && pParam1->isString())
    {
        AptValue *pParam2 = gAptActionInterpreter.stackAt(1);
        if (pParam1->isUndefined())
            return AptBoolean::Create(false);

        if (!AptGetLib()->mpObjRegistrationHash)
        {
            // first create AptGetLib()->mpObjRegistrationHash
            AptGetLib()->mpObjRegistrationHash = new AptNativeHash(APT_OBJECTHASHSIZE);
        }
        if (pParam2->isNone())
        {
            // unregister the class - so remove the entry from hash
            AptGetLib()->mpObjRegistrationHash->Unset(pParam1->c_string()->GetInternalString());
        }
        else
        {
            AptGetLib()->mpObjRegistrationHash->Set(pParam1->c_string()->GetInternalString(), pParam2);
            pParam2->SetClassName(pParam1->c_string()->GetInternalString()->GetBuffer());
        }
        return AptBoolean::Create(true);
    }
    return AptBoolean::Create(false);
}

AptPrototype *gpObjectPrototype   = NULL; // Prototype of "Object", cached to prevent constant lookups.
AptPrototype *gpFunctionPrototype = NULL; // Prototype of "Function", cached to prevent constant lookups.

/**
 * Helper for _constructBuiltInObjects().
 * @param sc StringPool enum for the name of the built-in object to set up.
 * @param proto object to set as __proto__ in the built-in object, or NULL to leave __proto__ alone.
 * @return the class/constructor that was created.
 */
AptValue *_constructBuiltInObject(StringCode sc, AptValue *proto = NULL)
{
    const AptNativeString *pName = StringPool::GetString(sc);
    AptPrototype *pPrototype     = new AptPrototype();
    pPrototype->SetClassName(pName->c_str());
    pPrototype->setGCRoot(1);

    AptValue *pBuiltIn = new AptNativeFunction(_constructorObject);
    pBuiltIn->GetNativeHashVirtual()->SetPrototype(pPrototype);
    if (proto)
    {
        pBuiltIn->GetNativeHashVirtual()->Set__Proto__(proto);
    }
    pBuiltIn->setGCRoot(1);
    AptGetLib()->mpGlobalGlobalObject->Set(pName, pBuiltIn);

    return pBuiltIn;
}

/**
 * Builds the native functions/prototypes for all built-in ActionScript types.
 */
static void _constructBuiltInObjects(void)
{
    //  Clean the vector for temporary construction
    AptGetLib()->mpValuesToRelease->ReleaseValues();

    // create prototype objects for all these builtin object
    // also point their __proto__ properties to Object.prototype

    AptValue *pObject       = _constructBuiltInObject(SC_Object);
    gpGlobalObjectPrototype = gpObjectPrototype = static_cast<AptPrototype *>(pObject->GetNativeHashVirtual()->GetPrototype());
    _constructBuiltInObject(SC_Array, gpObjectPrototype);
#if defined(APT_USE_SOUND_OBJECT)
    _constructBuiltInObject(SC_Sound, gpObjectPrototype);
#endif
#if APT_USE_SCRIPTCOLOUR_OBJECT
    _constructBuiltInObject(SC_Color, gpObjectPrototype);
#endif
    _constructBuiltInObject(SC_Date, gpObjectPrototype);
    _constructBuiltInObject(SC_MovieClipLoader, gpObjectPrototype);
    _constructBuiltInObject(SC_TextFormat, gpObjectPrototype);
    pObject                    = _constructBuiltInObject(SC_MovieClip, gpObjectPrototype);
    gpGlobalMovieclipPrototype = static_cast<AptPrototype *>(pObject->GetNativeHashVirtual()->GetPrototype());
    _constructBuiltInObject(SC_XML, gpObjectPrototype);
#if APT_USE_LOADVARS_OBJECT
    _constructBuiltInObject(SC_LoadVars, gpObjectPrototype);
#endif
    _constructBuiltInObject(SC_Error, gpObjectPrototype);
    _constructBuiltInObject(SC_String, gpObjectPrototype);

    AptValue *pFunction = _constructBuiltInObject(SC_Function);
    gpFunctionPrototype = static_cast<AptPrototype *>(pFunction->GetNativeHashVirtual()->GetPrototype());
    gpFunctionPrototype->GetNativeHashVirtual()->Set__Proto__(gpObjectPrototype); // Functions ARE objects.
    pFunction->GetNativeHashVirtual()->Set__Proto__(gpFunctionPrototype);         // In Flash, Function.__proto__ == Function.prototype.

    // The above objects get deleted when AptGetLib()->mpGlobalGlobalObject gets deleted.

    // Also create the basic native functions of the Object class. This function cannot be declared
    // with the NATIVE macro because AptNativeFunction is derived from AptObject, which would make it
    // recursive. So functions of the 'Object' class are defined globally and returned from
    // objectMemberLookup.
    AptGetLib()->mpObjRegistrationFunc = new AptNativeFunction(_gObjRegistrationFunc);
    AptGetLib()->mpObjRegistrationFunc->setGCRoot(1);
    APT_INC(AptGetLib()->mpObjRegistrationFunc);

    //  Clean the vector for temporary construction
    AptGetLib()->mpValuesToRelease->ReleaseValues();
}

/**
 * Sets up the Apt Value global landscape.
 */
void AptValueInitialize(void)
{
    APT_ASSERT(AptVFT_NumVFTs <= 0x003f && "Number of VFT indexes must be expanded to fit the number of types");

    gpUndefinedValue = new AptNone();

    AptBoolean::Initialize();
    AptLookup::Initialize();
    AptRegister::Initialize();

    AptCharacterHelper::Initialize();

    gpUndefinedCIH = new AptCIHNone();
    gpUndefinedCIH->SetInstanceName("EmptyCIH");

    AptGetLib()->mpRenderingContext = new AptRenderingContext();

    AptGetLib()->mpExternValue = new AptExtern();

    // Global objects like Math, Stage, Mouse, Key are reference counted.

#if APT_USE_MATH_OBJECT
    gpGlobalMathObject = new AptMathObj;
    APT_INC(gpGlobalMathObject);
#endif
    gpGlobalKeyObject = new AptKey;
    APT_INC(gpGlobalKeyObject);
#if defined(APT_USE_MOUSE)
    gpGlobalMouseObject = new AptMouse;
    APT_INC(gpGlobalMouseObject);
#endif

#if APT_USE_UTILITY
    gpGlobalAptUtilObject = new AptUtil;
    APT_INC(gpGlobalAptUtilObject);
#endif

#if defined(APT_ALTERNATE_INPUT)
    gpGlobalAltInputObject = new AptAlternateInput;
    APT_INC(gpGlobalAltInputObject);
#endif

#if defined(APT_USE_STAGE_OBJECT)
    gpGlobalStageObject = new AptStage;
    APT_INC(gpGlobalStageObject);
#endif

    AptGetLib()->mpGlobalGlobalObject = new AptGlobal;
    APT_INC(AptGetLib()->mpGlobalGlobalObject);

    AptGetLib()->mpGlobalExtensionObject = new AptGlobalExtensionObject;
    APT_INC(AptGetLib()->mpGlobalExtensionObject);

    // Global String Object added for Flash Player 7 / AS 2.0 support.
    gpGlobalStringObject = AptString::Create();
    gpGlobalStringObject->setGCRoot(1);
    APT_INC(gpGlobalStringObject); // Prevent deletion!

    AptGetLib()->mpObjRegistrationHash = NULL; // this is initialized only if object.registerclass gets called.

    _constructBuiltInObjects();

    AptColorHelperScale &vColorMul4     = gIdentityCXForm.scale;
    AptColorHelperTranslate &vColorAdd4 = gIdentityCXForm.translate;

    vColorMul4.SetValuef(AptColorHelper::Red, 255.f);
    vColorMul4.SetValuef(AptColorHelper::Green, 255.f);
    vColorMul4.SetValuef(AptColorHelper::Blue, 255.f);
    vColorMul4.SetValuef(AptColorHelper::Alpha, 255.f);
    vColorAdd4.SetValuef(AptColorHelper::Red, 0.f);
    vColorAdd4.SetValuef(AptColorHelper::Green, 0.f);
    vColorAdd4.SetValuef(AptColorHelper::Blue, 0.f);
    vColorAdd4.SetValuef(AptColorHelper::Alpha, 0.f);

    gIdentityMatrix.a  = 1.f;
    gIdentityMatrix.b  = 0.f;
    gIdentityMatrix.c  = 0.f;
    gIdentityMatrix.d  = 1.f;
    gIdentityMatrix.tx = 0.f;
    gIdentityMatrix.ty = 0.f;

    // setInterval, clearInterval, isNaN, unescape, escape, Boolean, ASSetPropFlags, parseInt,
    // parseFloat are all reference counted.

    gpCBsetInterval = new AptNativeFunction(AptActionInterpreter::cbCallMethod_setInterval);
    gpCBsetInterval->setGCRoot(1);
    APT_INC(gpCBsetInterval);
    gpCBclearInterval = new AptNativeFunction(AptActionInterpreter::cbCallMethod_clearInterval);
    gpCBclearInterval->setGCRoot(1);
    APT_INC(gpCBclearInterval);
    gpCBisNaN = new AptNativeFunction(AptActionInterpreter::cbCallMethod_isNaN);
    gpCBisNaN->setGCRoot(1);
    APT_INC(gpCBisNaN);
    gpCBunescape = new AptNativeFunction(AptActionInterpreter::cbCallMethod_unescape);
    gpCBunescape->setGCRoot(1);
    APT_INC(gpCBunescape);
    gpCBescape = new AptNativeFunction(AptActionInterpreter::cbCallMethod_escape);
    gpCBescape->setGCRoot(1);
    APT_INC(gpCBescape);
    gpCBboolean = new AptNativeFunction(AptActionInterpreter::cbCallMethod_boolean);
    gpCBboolean->setGCRoot(1);
    APT_INC(gpCBboolean);
    // Added for Flash Player 7 and AS 2.0 support.
    gpASSetPropFlags = new AptNativeFunction(AptActionInterpreter::cbCallMethod_ASSetPropFlags);
    gpASSetPropFlags->setGCRoot(1);
    APT_INC(gpASSetPropFlags);
    gpCBparseInt = new AptNativeFunction(AptActionInterpreter::cbCallMethod_parseInt);
    gpCBparseInt->setGCRoot(1);
    APT_INC(gpCBparseInt);
    gpCBparseFloat = new AptNativeFunction(AptActionInterpreter::cbCallMethod_parseFloat);
    gpCBparseFloat->setGCRoot(1);
    APT_INC(gpCBparseFloat);

    // From Apt-0.16.00 onwards registers are initialized in regStack.init(), and
    // gAptActionInterpreter.apRegisters is no longer used in any operation.
    memset(gAptActionInterpreter.apRegisters, 0, sizeof(gAptActionInterpreter.apRegisters));

    //  Clean the vector for temporary construction
    AptGetLib()->mpValuesToRelease->ReleaseValues();
}

/**
 * Undoes what was done in AptValueInitialize(), releasing memory so a subsequent init succeeds.
 */
void AptValueShutdown(int bQuiet)
{
    if (AptGetLib()->mpObjRegistrationHash)
    {
        AptGetLib()->mpObjRegistrationHash->DestroyGCPointers();
        delete AptGetLib()->mpObjRegistrationHash;
        AptGetLib()->mpObjRegistrationHash = NULL;
    }

    AptGetLib()->mpGlobalExtensionObject->DestroyGCPointers();
    APT_DEC(AptGetLib()->mpGlobalExtensionObject);
    AptGetLib()->mpGlobalExtensionObject = NULL;
    AptGetLib()->mpGlobalGlobalObject->DestroyGCPointers();
    APT_DEC(AptGetLib()->mpGlobalGlobalObject);
    AptGetLib()->mpGlobalGlobalObject = NULL;
    // Always force delete GlobalObject first, then destroy pointers for the global Math etc.
    // objects. This mirrors the AptValueInitialize() sequence.

#if APT_USE_MATH_OBJECT
    gpGlobalMathObject->DestroyGCPointers();
    APT_DEC(gpGlobalMathObject);
    gpGlobalMathObject = NULL;
#endif
    gpGlobalKeyObject->DestroyGCPointers();
    APT_DEC(gpGlobalKeyObject);
    gpGlobalKeyObject = NULL;
#if defined(APT_USE_MOUSE)
    gpGlobalMouseObject->DestroyGCPointers();
    APT_DEC(gpGlobalMouseObject);
    gpGlobalMouseObject = NULL;
#endif

#if APT_USE_UTILITY
    gpGlobalAptUtilObject->DestroyGCPointers();
    APT_DEC(gpGlobalAptUtilObject);
    gpGlobalAptUtilObject = NULL;
#endif

#if defined(APT_ALTERNATE_INPUT)
    gpGlobalAltInputObject->DestroyGCPointers();
    APT_DEC(gpGlobalAltInputObject);
    gpGlobalAltInputObject = NULL;
#endif

#if defined(APT_USE_STAGE_OBJECT)
    gpGlobalStageObject->DestroyGCPointers();
    APT_DEC(gpGlobalStageObject);
    gpGlobalStageObject = NULL;
#endif

    gpGlobalStringObject->DestroyGCPointers();
    APT_DEC(gpGlobalStringObject);
    gpGlobalStringObject = NULL;
    delete AptGetLib()->mpRenderingContext;

    gpCBsetInterval->DestroyGCPointers();
    APT_DEC(gpCBsetInterval);
    gpCBsetInterval = NULL;
    gpCBclearInterval->DestroyGCPointers();
    APT_DEC(gpCBclearInterval);
    gpCBclearInterval = NULL;
    gpCBisNaN->DestroyGCPointers();
    APT_DEC(gpCBisNaN);
    gpCBisNaN = NULL;
    gpCBunescape->DestroyGCPointers();
    APT_DEC(gpCBunescape);
    gpCBunescape = NULL;
    gpCBescape->DestroyGCPointers();
    APT_DEC(gpCBescape);
    gpCBescape = NULL;
    gpCBboolean->DestroyGCPointers();
    APT_DEC(gpCBboolean);
    gpCBboolean = NULL;
    // Added for Flash Player 7 / AS 2.0 support.
    gpASSetPropFlags->DestroyGCPointers();
    APT_DEC(gpASSetPropFlags);
    gpASSetPropFlags = NULL;

    AptGetLib()->mpObjRegistrationFunc->DestroyGCPointers();
    APT_DEC(AptGetLib()->mpObjRegistrationFunc);
    AptGetLib()->mpObjRegistrationFunc = NULL;

    gpCBparseInt->DestroyGCPointers();
    APT_DEC(gpCBparseInt);
    gpCBparseInt = NULL;

    gpCBparseFloat->DestroyGCPointers();
    APT_DEC(gpCBparseFloat);
    gpCBparseFloat = NULL;
}

void AptValueShutdownRemaining()
{
    gpUndefinedValue->ForceDelete();
    AptBoolean::Shutdown();
    AptLookup::Shutdown();
    AptRegister::Shutdown();
    AptCharacterHelper::Shutdown();
    AptGetLib()->mpExternValue->ForceDelete();
}

/**
 * Encodes all the properties of this AptValue into an AptNativeString, concatenated with '&'.
 * Called from LoadVars and from the custom-control render case for AptCIH::render.
 */
AptNativeString AptValue::urlEncode()
{

    AptNativeString sProperties;
    if (!this->getIsDefined())
    {
        return sProperties;
    }

    AptNativeHash *pObjHash = GetNativeHashVirtual();
    if (!pObjHash)
        return sProperties;

    AptNativeString sValueBuf;

    for (AptHashItem *pInitItem = pObjHash->GetFirstItem(); pInitItem; pInitItem = pObjHash->GetNextItem(pInitItem))
    {
        // TODO: theoretically, every hash key is supposed to have a DontEnum/DontDelete flag; we
        // don't have that, so hardcode some dontenum's here.
        if ((pInitItem->Key.EqualNoCase(*StringPool::GetString(SC___proto__))) ||
            (pInitItem->Key.EqualNoCase(*StringPool::GetString(SC_prototype))))
        {
            continue;
        }

        pInitItem->mValue->toString(sValueBuf);
        sProperties += pInitItem->Key;
        sProperties += "=";
        sProperties += sValueBuf;
        sProperties += "&";
    }
    sProperties.EndWithRemove("&");

    return sProperties;
}

/**
 * URL-encodes custom render properties: properties that start with '_' and are not "_type" and not
 * a function.
 */
AptNativeString AptValue::urlEncodeCustomRender()
{

    AptNativeString sProperties;
    if (!this->getIsDefined())
    {
        return sProperties;
    }

    AptNativeHash *pObjHash = GetNativeHashVirtual();
    if (!pObjHash)
        return sProperties;

    AptNativeString sValueBuf;

    for (AptHashItem *pInitItem = pObjHash->GetFirstItem(); pInitItem; pInitItem = pObjHash->GetNextItem(pInitItem))
    {
        AptNativeString strKey = pInitItem->Key;
        if (strKey[0] == '_')
        {
            const char *pStrBuffer = strKey.c_str() + 1;
            if (strcmp(pStrBuffer, "_proto__") == 0)
            {
                continue;
            }
            if (strcmp(pStrBuffer, "type") == 0)
            {
                continue;
            }

            AptValue *pValue = pInitItem->mValue;
            if (!pValue->isNativeFunction() && !pValue->isExternalFunction())
            {
                pValue->toString(sValueBuf);
                sProperties += strKey;
                sProperties += "=";
                sProperties += sValueBuf;
                sProperties += "&";
            }
        }
    }
    sProperties.EndWithRemove("&");
    return sProperties;
}

/**
 * Looks up @p pName as either a reserved word (_root, this, super, _global, Key, Math, etc.) or a
 * member of @p pWith (or this, if @p pWith is NULL).
 * @param bIsMember true if looking up a member of a class rather than a free variable; suppresses
 * reserved-word lookup except for _global members.
 */
AptValue *AptValue::findChild(const AptNativeString *pName, AptValue *pWith, bool bIsMember)
{
    AptValue *pCur = pWith ? pWith : this;

    int gperfIndex = -1;

#if !defined(APT_ALLOW_CLASS_MEMBER_RESERVED_WORDS_LOOKUP)
    // Don't look in the reserved words if the variable is a member of a class.
    // But do look in reserved words if we're looking for a member of _global.
    if (!bIsMember || pCur == AptGetLib()->mpGlobalGlobalObject)
    {

        gperfIndex = AptGetLib()->mpGlobalGlobalObject->GetGperfIndex(pName);
    }
#else
    // This mode allows classes to have members that shadow reserved words, matching some third
    // party content that (incorrectly, versus real Flash) relies on this behaviour.
    {

        gperfIndex = AptGetLib()->mpGlobalGlobalObject->GetGperfIndex(pName);
#if (APT_CURR_DEBUG_MSG_LVL >= APT_DEBUG_MSG_ASSERT_LVL)
        if (bIsMember && pCur != AptGetLib()->mpGlobalGlobalObject && -1 != gperfIndex)
        {
            const int ERROR_BUFFER_SIZE = 1024;
            char errorBuffer[ERROR_BUFFER_SIZE];
            SNPRINTF(errorBuffer, ERROR_BUFFER_SIZE, "Found data behavior inconsistency attempting to find child with name \"%s\", likely a reserved word - see comments in AptValue.cpp around line %d.", pName->c_str(), __LINE__);
            errorBuffer[ERROR_BUFFER_SIZE - 1] = '\0';
            APT_FAIL(errorBuffer);
        }
#endif
    }
#endif

    {
        if (gperfIndex >= 0)
        {
            switch (gperfIndex)
            {
            case AptObjectthis:
            {
                // Hack for super: check if pValue is actually a super of 'this'; if not, return pValue.
                AptValue *pValue = gAptActionInterpreter.thisStack.top();
                AptValue *pThis  = this;
                if (pValue == pThis)
                    return pThis;

                if (gAptActionInterpreter.withStack.size() > 0 && !gAptActionInterpreter.withStack.top()->isUndefined())
                {
                    AptValue *pWithStkTop = gAptActionInterpreter.withStack.top();
                    APT_ASSERT(pWithStkTop != NULL);
                    APT_ASSERT(pWithStkTop->ContainsNativeHashVirtual());

                    if (pValue == pWithStkTop)
                    {
                        return pValue;
                    }

                    AptNativeHash *pNativeHash = pWithStkTop->GetNativeHashVirtual();
                    AptValue *pTempValue;

                    while (pNativeHash)
                    {
                        pTempValue = pNativeHash->Get__Proto__();
                        if (pTempValue)
                        {
                            if (pTempValue == pValue)
                            {
                                // pObject is in the class hierarchy of withStack.top()!
                                return pWithStkTop;
                            }
                            pNativeHash = pTempValue->GetNativeHashVirtual();
                        }
                        else
                        {
                            break;
                        }
                    }
                }

                AptNativeHash *pNativeHash = pThis->GetNativeHashVirtual();
                while (pNativeHash)
                {
                    AptValue *pValue1 = pNativeHash->Get__Proto__();
                    if (pValue1)
                    {
                        if (pValue1 == pValue)
                            return this;
                        else
                            pNativeHash = pValue1->GetNativeHashVirtual();
                    }
                    else
                        break;
                }

                return gAptActionInterpreter.thisStack.top();
            }

            break;
            case AptObject_root:
            {
                AptCIH *pParentAnim = NULL;
                if (pCur->isCIH())
                {
                    pParentAnim = pCur->c_cih();
                }
                else
                {
                    if (gAptActionInterpreter.mpCurrentFunction &&
                        gAptActionInterpreter.mpCurrentFunction->mpParentAnim &&
                        gAptActionInterpreter.mpCurrentFunction->mpParentAnim->isCIH())
                    {
                        // pCur is not a CIH: find the current executing function's parent animation
                        // and get its root from there instead.
                        pParentAnim = gAptActionInterpreter.mpCurrentFunction->mpParentAnim;
                    }
                }

                if (pParentAnim)
                {
                    // repeatedly set pParentAnim to its own display list parent,
                    // until we reach the root
                    while (pParentAnim->GetDisplayListParent())
                    {
                        pParentAnim = pParentAnim->GetDisplayListParent();
                    }
                    return pParentAnim;
                }
                return gpUndefinedValue;
            }

            break;
            case AptObject_global:
            {
                return AptGetLib()->mpGlobalGlobalObject;
            }

            break;
            case AptObjectKey:
            {
                // This case could probably be moved into AptGlobal::Lookup().
                return gpGlobalKeyObject;
            }
            break;
            case AptObjectMouse:
            {
#if defined(APT_USE_MOUSE)
                return gpGlobalMouseObject;
#else
                return gpUndefinedValue;
#endif
            }
            break;
            case AptObject_AlternateInput:
            {
                // This case could probably be moved into AptGlobal::Lookup().
#if defined(APT_ALTERNATE_INPUT)
                return gpGlobalAltInputObject;
#else
                APT_ASSERT(0 && "APT-Warning-AlternateInput object cannot be used as it is compiled out, make sure to compile with APT_ALTERNATE_INPUT defined as 1");
#endif
            }
            break;
            case AptObjectMath:
            {
                // This case could probably be moved into AptGlobal::Lookup().
#if APT_USE_MATH_OBJECT
                return gpGlobalMathObject;
#else
                APT_ASSERT(0 && "APT-Warning-Math object cannot be used as it is compiled out, make sure to compile with APT_USE_MATH_OBJECT defined as 1");
                return gpUndefinedValue;
#endif
            }

            // Added for Flash Player 7 / AS 2.0 support: a real global String object.
            break;
            case AptObject_String:
            {
                return gpGlobalStringObject;
            }

            break;
            case AptObject_Stage:
            {
                // This case could probably be moved into AptGlobal::Lookup().
#if defined(APT_USE_STAGE_OBJECT)
                return gpGlobalStageObject;
#else
                APT_ASSERT(0 && "APT-Warning-Stage object cannot be used as it is compiled out, make sure to compile with APT_USE_STAGE_OBJECT defined in Aptdefine.h");
                return gpUndefinedValue;
#endif
            }

            case AptObject_AptUtil:
            {
                // This case could probably be moved into AptGlobal::Lookup().
#if APT_USE_UTILITY
                return gpGlobalAptUtilObject;
#else
                APT_ASSERT(0 && "APT-Warning-AptUtil object cannot be used as it is compiled out, make sure to compile with APT_USE_UTILITY defined in Aptdefine.h");
                return gpUndefinedValue;
#endif
            }

            break;
            case AptObjectextern:
            {
                return AptGetLib()->mpExternValue;
            }

            //  super lookup
            break;
            case AptObjectsuper:
            {
                AptValue *pStackTop = gAptActionInterpreter.thisStack.top();

                if (pStackTop->ContainsNativeHashVirtual())
                {
                    AptNativeHash *pNativeHash = pStackTop->GetNativeHashVirtual();
                    AptValue *pValue           = pNativeHash->Get__Proto__();
                    if (pValue)
                    {
                        if ((pStackTop != this) && !pStackTop->isCIH() && (pStackTop->isMCInParentChain()))
                        {
                            if ((pStackTop->isObject() && pStackTop->c_object()->getInMainInst()))
                            {
                                pNativeHash = pValue->GetNativeHashVirtual();
                                if (pNativeHash)
                                {
                                    pValue = pNativeHash->Get__Proto__();
                                    if (pValue)
                                    {
                                        return pValue;
                                    }
                                }
                            }
                            return pValue;
                        }
                        else if (pStackTop->isPrototype())
                        {
                            return pValue;
                        }

                        // Hacks a problem with AS 2.0 objects where this would call every other
                        // super constructor. This is here because of how the constructors run in
                        // Apt, which is itself hacky. This turns off the hack (below) added for
                        // AS 1.0 that gets the __proto__'s __proto__, which isn't needed for AS 2.0
                        // objects.
                        if (pValue->isPrototype() && pValue->c_prototype()->GetSuperConstructor())
                        {
                            // Fix so we do not skip the super constructors or functions in the
                            // inheritance hierarchy, or call them multiple times at the same level.
                            // withStack is used when there is a super hierarchy in functions we are
                            // executing.
                            if ((gAptActionInterpreter.withStack.size() > 0) && (gAptActionInterpreter.withStack.top() == pStackTop))
                            {
                                if (pStackTop->isCIH())
                                {
                                    if (!pStackTop->c_cih()->IsInCtor()) // set through AptCIH::AssociateInstToClass
                                    {
                                        return (pValue->GetNativeHashVirtual()->Get__Proto__());
                                    }
                                }
                                else if (pStackTop->isObject())
                                {
                                    if (pStackTop->GetHasClass())
                                    {
                                        return (pValue->GetNativeHashVirtual()->Get__Proto__());
                                    }
                                }
                            }
                            return pValue;
                        }
                        else
                        {
                            pNativeHash = pValue->GetNativeHashVirtual();
                            if (pNativeHash)
                            {
                                pValue = pNativeHash->Get__Proto__();
                                if (pValue)
                                {
                                    return pValue;
                                }
                            }
                        }
                    }
                }
                return gpUndefinedValue;
            }

            break;
            case AptObjectparent:
            {
                if (pCur->isCIH())
                {
                    return pCur->c_cih()->GetDisplayListParent();
                }
            }

            break;
            case AptObject_target:
            {
                return NULL;
            }
            case AptObject_level0:
            case AptObject_level1:
            case AptObject_level2:
            case AptObject_level3:
            case AptObject_level4:
            case AptObject_level5:
            case AptObject_level6:
            case AptObject_level7:
            case AptObject_level8:
            case AptObject_level9:
            case AptObject_level10:
            case AptObject_level11:
            case AptObject_level12:
            case AptObject_level13:
            case AptObject_level14:
            case AptObject_level15:
            case AptObject_level16:
            case AptObject_level17:
            case AptObject_level18:
            case AptObject_level19:
            case AptObject_level20:
            case AptObject_level21:
            case AptObject_level22:
            case AptObject_level23:
            case AptObject_level24:
            {
                int32_t nLevel = static_cast<int32_t>(atoi(pName->c_str() + 6));

                return _AptGetAnimationAtLevel(nLevel);
            }
            break;

            case AptGlobal::setIntervalIdx:
            case AptGlobal::clearIntervalIdx:
            {
                return AptGetLib()->mpGlobalGlobalObject->Lookup(gperfIndex);
            }

            default:
                APT_ASSERT(NOT_REACHED);
                break;
            }
            return NULL;
        }
        else
        {
            if (pCur->isUndefined())
            {
                return NULL;
            }
            else
            {
                AptNativeHash *pNativeHash = pCur->GetNativeHashVirtual();

#if defined APT_USE_BUTTONS
                // Fixed instance where we would not be able to access certain members, i.e. we needed to chain.
                if (pNativeHash == NULL && (pCur->isCIH() && pCur->c_cih()->IsButtonInst()))
                {
                    pNativeHash = pCur->c_cih()->GetNativeHash(); // buttons now have a hash
                }
#endif

                while (pNativeHash)
                {
                    AptValue *pRet = pNativeHash->Lookup(pName);
                    if (pRet)
                        return pRet;
                    AptValue *pProto = pNativeHash->Get__Proto__();
                    if (pProto)
                    {
                        pNativeHash = pProto->GetNativeHashVirtual();
                    }
                    else
                    {
                        break;
                    }
                }

                // If this is known to be a member, abort now.  Otherwise, look in _global stuff.
                if (bIsMember)
                {
                    return NULL;
                }

                AptValue *pRet = AptGetLib()->mpGlobalExtensionObject->Lookup(pName);
                // Omit global scope if pWith is set and doesn't match, so APT with() output matches Flash.
                if (pRet == NULL && this != pWith)
                {
                    pRet = AptGetLib()->mpGlobalGlobalObject->Lookup(pName);
                }
                return pRet;
            }
        }
    }
}

/**
 * @return true if MovieClip.prototype is in this object's __proto__ hierarchy chain.
 */
int AptValue::isMCInParentChain() const
{
    int iRet = 0;

    AptValue *pPrototypeMovieClip = gpGlobalMovieclipPrototype;

    AptValue *pPrototypeObject = gpGlobalObjectPrototype;

    if (this == pPrototypeMovieClip)
        return 1;
    AptNativeHash *pNativeHash = ((AptValue *)this)->GetNativeHashVirtual();
    while (pNativeHash)
    {
        AptValue *pValue = pNativeHash->Get__Proto__();
        if (pValue)
        {
            if (pValue == pPrototypeMovieClip)
                return 1;
            else if (pValue == pPrototypeObject)
                return 0;

            pNativeHash = pValue->GetNativeHashVirtual();
        }
        else
            break;
    }
    return iRet;
}

/**
 * @return true if this object is capable of being the type of a dynamic scriptObject.
 */
bool AptValue::CanCreateScriptObject() const
{
    switch (getVtblIndex())
    {
    case AptVFT_Object:
    case AptVFT_NativeFunction:
    case AptVFT_ScriptFunction1:
    case AptVFT_ScriptFunction2:
    case AptVFT_StringValue:
    case AptVFT_StringObject:
#if defined(APT_USE_SOUND_OBJECT)
    case AptVFT_Sound:
#endif
    case AptVFT_Array:
    case AptVFT_Date:
    case AptVFT_TextFormat:
#if APT_USE_SCRIPTCOLOUR_OBJECT
    case AptVFT_ScriptColour:
#endif
    case AptVFT_MovieClip:
    case AptVFT_MovieClipLoader:
    case AptVFT_Xml:
#if APT_USE_LOADVARS_OBJECT
    case AptVFT_LoadVars:
#endif
    case AptVFT_ExternalFunction:
    case AptVFT_Error:
        return true;
    default:
        return false;
    }
}

/**
 * Increments the reference counter.
 * Users should call the APT_INC/APT_DEC macros rather than this function directly, to ensure the
 * correct parameters are passed in all builds.
 */
#if defined(APT_INC_DEC_MESSAGES)

// This is a regular expression to find the messages associated with 032551B4
// ^(~(OBJ)|(OBJ,:a+,:a+,Th~(032551B4))).#\n
// Usually replace matches with nothing.

static long RefCountCallCounter                   = 0;
AptVirtualFunctionTable_Indices gVftTrackThisType = AptVFT_NumVFTs; /* Set this to whatever data type you want to track. */
AptValue *gpTrackThisObject                       = NULL;           /* Set this to the address of the object you want to track. */

void AptValue::AddRef(const char *szFuncName, const char *szFileName, int nLineNumber)
#else

void AptValue::AddRef()
#endif
{
// Increment / Decrement messages, turned on with APT_INC_DEC_MESSAGES.
#if defined(APT_INC_DEC_MESSAGES)
    AptVirtualFunctionTable_Indices mYtype = getVtblIndex();
    if (mYtype == gVftTrackThisType || this == gpTrackThisObject)
    {
        AptNativeString temp;
        toString(temp);
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "OBJ,Inc,Ck%8.8X,Th%8.8X,RCt%4.4X,%s,%s:%d\n", RefCountCallCounter++, this, getRefCount(), temp.c_str(), szFuncName, nLineNumber);
    }
#endif

#ifdef CHECK_REFCOUNT
    int32_t i;
    for (i = 0; i < sizeof(mValuesToTest) / sizeof(mValuesToTest[0]); ++i)
    {
        if (this == mValuesToTest[i])
        {
            break; //  You can put a breakpoint here in debug
        }
    }
#endif
    IncrementAptValue();
}

/**
 * Atomically increments the reference count, clamping to MAX_REFCOUNT.
 */
int32_t AptValue::IncrementAptValue()
{
    uint32_t n = getRefCount() + 1;
    if (n > MAX_REFCOUNT)
    {
        SetMaxRefCountHit(true);
        n = MAX_REFCOUNT;
    }
    mValueBitfield.mnReferenceCount = n;
    return n;
}

/**
 * Atomically decrements the reference count.
 */
int32_t AptValue::DecrementAptValue()
{
    uint32_t n                      = getRefCount() - 1;
    mValueBitfield.mnReferenceCount = n;
    return n;
}

/**
 * Decrements the ref count on the AptValue. If it reaches zero it is either deleted right away or
 * pushed onto the deferred vector, depending on current vector capacity. If
 * sbSuspendRefcountDeletions is true, garbage collected objects are not actually deleted here (used
 * mainly by the garbage collector).
 */
#if defined(APT_INC_DEC_MESSAGES)
void AptValue::Release(const char *szFuncName, const char *szFileName, int nLineNumber)
#else
void AptValue::Release()
#endif
{
    if (GetMaxRefCountHit()) // Don't try to release if the max ref count was hit; we don't know how many references there were.
        return;

    int32_t nRefCount = DecrementAptValue();

// Increment / Decrement messages, turned on with APT_INC_DEC_MESSAGES.
#if defined(APT_INC_DEC_MESSAGES)
    AptVirtualFunctionTable_Indices mYtype = getVtblIndex();
    if (mYtype == gVftTrackThisType || this == gpTrackThisObject)
    {
        AptNativeString temp;
        toString(temp);
        APT_DEBUGPRINT(APT_DEBUG_MSG_DEFAULT_LVL, "OBJ,Dec,Ck%8.8X,Th%8.8X,RCt%4.4X,%s,%s:%d\n", RefCountCallCounter++, this, nRefCount + 1, temp.c_str(), szFuncName, nLineNumber);
    }
#endif

#ifdef CHECK_REFCOUNT
    int32_t i;
    for (i = 0; i < sizeof(mValuesToTest) / sizeof(mValuesToTest[0]); ++i)
    {
        if (this == mValuesToTest[i])
        {
            break; //  You can put a breakpoint here in debug
        }
    }
#endif

    APT_ASSERT(nRefCount + 1 > 0);

    // Restructured so that _ForceDeleteIfNecessary (the minority case) is a separate helper
    // function, avoiding a load-hit-store in the common path.
    if (nRefCount == 0)
    {
        _ForceDeleteIfNecessary();
    }
}

/**
 * Helper for Release() that handles the uncommon deletion path, letting the compiler avoid a
 * load-hit-store on the link register in the common case (caused by the virtual functions
 * IsGarbageCollected() and ForceDelete()).
 */
void AptValue::_ForceDeleteIfNecessary()
{
    if (sbSuspendRefcountDeletions == false || IsGarbageCollected() == false)
    {
        if ((!GetMaxRefCountHit()) && GetAllowDelayedDeletion())
        {
            //  There is a chance we want to delete this object at the end of the frame
            if (IsReleaseAtEnd() == false)
            {
                // here we check first if the vector is full otherwise just delete it right away.
                if (!AptGetLib()->mpValuesToRelease->IsVectorFull())
                {
                    //  Flag not already set, we set the flag
                    //  So we will try to delete this instane at the end of the frame
                    SetReleaseAtEnd();
                    AptGetLib()->mpValuesToRelease->PushValue(this);
                }
                else
                {
                    this->ForceDelete();
                }
            }
        }
        else
        {
            ForceDelete();
        }
    }
}

/**
 * @return the native hash of this object.
 */
AptNativeHash *AptValueWithHash::GetNativeHashVirtual()
{
    return (&mNativeHash);
}

/**
 * @return true, since this class has a native hash.
 */
bool AptValueWithHash::ContainsNativeHashVirtual() const
{
    return (true);
}

/**
 * Calls the parent version, then tells the native hash to register its references.
 */
void AptValueWithHash::RegisterReferences()
{
    if (APT_REFERENCES_REGISTERED(this))
        return;

    // Not defined up the inheritance tree at this base-class level, so not called here.
    // AptValueGC::RegisterReferences();

    mNativeHash.RegisterReferences(this);
    return;
}

/**
 * Calls the parent version, then tells the native hash to destroy its pointers.
 */
void AptValueWithHash::DestroyGCPointers()
{
    // Go up the chain.
    AptValueGC::DestroyGCPointers();

    mNativeHash.DestroyGCPointers();
}

/**
 * Sets an external variable via the game's pfnSetExternVariable callback.
 */
bool AptExtern::objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue)
{
    AptNativeString sBuf;
    pValue->toString(sBuf);
    AptGetUserFuncs().pfnSetExternVariable(pName->c_str(), sBuf);
    return (true);
}

/** Helper functions for more efficient string handling. */
const AptNativeString *AptValue::Get_ToString(AptNativeString &sPotentialBuffer)
{
    APT_ASSERT(this);
    const AptNativeString *pStr = &sPotentialBuffer;
    if (isString())
        pStr = c_string()->GetInternalString();
    else
        toString(sPotentialBuffer);
    return pStr;
}

/** Helper functions for more efficient string handling. */
void AptValue::Append_ToString(AptNativeString &sString) const
{
    APT_ASSERT(this);
    if (isString())
        sString += *(c_string()->GetInternalString());
    else if (sString.IsEmpty())
        toString(sString);
    else
    {
        AptNativeString sAppend;
        toString(sAppend);
        sString += sAppend;
    }
}

#if defined(EA_COMPILER_CLANG) || defined(_MSC_VER)
/**
 * Forces explicit instantiation of these member function pointers to avoid a linker error on some
 * toolchains.
 */
namespace AVOIDLINKERROR
{
typedef AptInteger *(AptValue::*P1)() const;
P1 p1 = &AptValue::c_integer;

typedef AptFloat *(AptValue::*P2)() const;
P2 p2 = &AptValue::c_float;

typedef AptBoolean *(AptValue::*P3)() const;
P3 p3 = &AptValue::c_boolean;

typedef AptObject *(AptValue::*P4)() const;
P4 p4 = &AptValue::c_object;

typedef bool (AptBoolean::*P5)() const;
P5 p5 = &AptBoolean::GetBool;

} // namespace AVOIDLINKERROR
#endif
