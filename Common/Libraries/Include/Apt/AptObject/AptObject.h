/**
 * Implements "Object" from ActionScript; the basis for most classes in Apt that can be instantiated
 * as Objects in Flash.
 *
 * Background on Objects, Prototypes, constructors, and __proto__:
 * - Object.prototype refers to the implementation of the class (its members etc.), distinct from
 *   the entries in the Object's own hash.
 * - Object.__proto__ refers to the prototype (implementation) of the superclass. This is generally
 *   read-only in Flash from the subclass (though Apt itself doesn't enforce that). When looking up
 *   a member, Apt starts at the local scope and walks up the __proto__ chain until some object in
 *   the chain has the member.
 * - Object.prototype.__constructor__ is the pointer to the superclass constructor object, i.e. the
 *   script executed when the super object's constructor is called via super(). Accessible only in
 *   ActionScript 2.0+ / Flash 7+; not usable directly in Flash 6 / ActionScript 1.0.
 *
 * Each object's prototype chain forms a parallel structure to its __proto__ chain: an object and
 * its prototype both have their own prototype/__proto__, connected via __constructor__, so that a
 * SubObject's prototype chain mirrors its own inheritance chain up through Base Object.
 */

#pragma once
#include "AptValue/AptValue.h"
#include "AptNativeHash.h"

//  Initial size of the hash-table
#define APT_OBJECTHASHSIZE 8

// ##############################################################################
//
/**
 * @brief Class AptValueWithHash is a base class for all garbage collected items that
 * have a hash, it implements the basic hash functions (which were previously
 * duplicated for every object)
 */
#include "AptNativeHash.h"

class AptValueWithHash : public AptValueGC
{
    APT_ACCESS_INTERNAL :

                          AptValueWithHash(AptVirtualFunctionTable_Indices eType, int nHashSize) : AptValueGC(eType), mNativeHash(nHashSize) {
                                                                                                   };

    virtual AptNativeHash *GetNativeHashVirtual();
    virtual bool ContainsNativeHashVirtual() const;
    virtual void RegisterReferences();
    virtual void DestroyGCPointers();

    APT_INLINE AptValue *Lookup(const AptNativeString *const pKey) const;
    APT_INLINE void Set(const AptNativeString *const pKey, AptValue *const pValue);

  protected:
    virtual ~AptValueWithHash() {};
    AptNativeHash mNativeHash;

    // AptNativeHash needs to be able to access protected methods.
    // Had to add class keyword for psp compiler.
    friend class AptNativeHash;

  public:
    unsigned int GetNumAptValues();
    const AptValue *GetKeyValuePair(unsigned int uKeyIndex, AptNativeString &keyName);
};

// ##############################################################################
//
/**
 * @brief Class AptValueWithHash is a base class for all global garbage collected,
 * (but non-reference counted) items that  have a hash, it implements the basic
 * hash functions (which were previously duplicated for every object).
 */
class AptValueGlobalWithHash : public AptValueWithHash
{
    APT_ACCESS_INTERNAL :

                          AptValueGlobalWithHash(AptVirtualFunctionTable_Indices eType, int nHashSize) : AptValueWithHash(eType, nHashSize) {
                                                                                                         };

    // This Optimizes global objects by doing nothing on Add/Release
#if defined(APT_INC_DEC_MESSAGES)
    virtual void AddRef(const char *szFuncName, const char *szFileName, int nLineNumber) {}  //  Do nothing...
    virtual void Release(const char *szFuncName, const char *szFileName, int nLineNumber) {} //  Do nothing...
#else
    virtual void AddRef() {}  //  Do nothing...
    virtual void Release() {} //  Do nothing...
#endif
};
/*------------------------------------------------------------------------------------------------------*\

\*------------------------------------------------------------------------------------------------------*/

// ################################################################################################
/**
 * @brief AptPrototype -- This implements classes in Actionscript. They are placed into prototypes.
 * and hierarchy chains are built with these object.
 */
class AptPrototype : public AptValueWithHash
{
    APT_ACCESS_INTERNAL : static void *operator new(size_t size);
    static void operator delete(void *p, size_t size);


    AptPrototype() : AptValueWithHash(AptVFT_Prototype, APT_OBJECTHASHSIZE),
                     mp__constructor__(NULL)
    {
        SetAllowDelayedDeletion(false);
    }

    virtual void DestroyGCPointers();
    virtual void RegisterReferences();

    virtual const char *GetClassName() const;
    virtual void SetClassName(const char *name);

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;
    virtual bool objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue);

    APT_INLINE
    AptValue *GetSuperConstructor() const
    {
        return mp__constructor__;
    }

    void SetSuperConstructor(AptValue *pNewSuperConstructor);

  protected:
    virtual ~AptPrototype()
    {
        //  nothing to do...
    }

  private:
    AptValue *mp__constructor__;
#if defined(APT_RECORD_SCOPE_INFO)
    AptNativeString mClassName;
#endif

  private:
    // Do not call. Garbage collected objects cannot be created as C++ arrays.
    static void *operator new[](size_t size);
    static void operator delete[](void *p);
};

// ################################################################################################
/** @brief AptObject -- The basics of a dynamic script object. */
class AptObject : public AptValueWithHash
{
    APT_ACCESS_INTERNAL : static void *operator new(size_t size);
    static void operator delete(void *p, size_t size);


    AptObject(AptVirtualFunctionTable_Indices eType, int nSize = APT_OBJECTHASHSIZE) : AptValueWithHash(eType, nSize)

    {
    }

    // Common interface to "hasClass" functions from AptValue (now virtual).
    virtual void SetHasClass(int bHasClass)
    {
        mbHasClass = (bHasClass) ? 1u : 0u;
    }

    // Common interface to "hasClass" functions from AptValue (now virtual).
    virtual bool GetHasClass() const
    {
        return mbHasClass != 0u;
    }

    inline void setInMainInst(int bInMainInst)
    {
        mbIsInMainInst = (bInMainInst) ? 1u : 0u;
    }

    inline int getInMainInst() const
    {
        return static_cast<int>(mbIsInMainInst);
    }

    virtual AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;

    virtual void RegisterReferences();
    virtual void DestroyGCPointers();

    APT_INLINE void Set__Proto__(AptValue *const pValue);
    APT_INLINE void SetPrototype(AptValue *const pValue);

    void SetImplementedObjects(AptArray *paImplementedObjects, int nNumObjects);
    AptArray *GetImplementedObjects(int *nOutNumObjects) const;
    bool DoesImplementObject(AptValue *pObject) const;

    AptObject *Clone(bool bRecursive = false) const;

  protected:
    virtual ~AptObject()
    {
    }

    uint32_t mnImplementedObjects : 8 {0}; // Max 256 implemented Interfaces
    uint32_t mbHasClass : 1 {0};
    uint32_t mbIsInMainInst : 1 {0};

  private:
    // Do not call. Garbage collected objects cannot be created as C++ arrays.
    static void *operator new[](size_t size);
    static void operator delete[](void *p);
};
