#include "_Apt.h"
#include "AptObject/AptObject.h"
#include "MainInline.h"

#if !defined(APT_ENABLE_INLINE)
#include "AptObject/AptObject.inl"
#endif


/**
 * @return the number of elements stored in the hash table.
 */
unsigned int AptValueWithHash::GetNumAptValues()
{
    unsigned int uNumKeys = 0;

    for (AptHashItem *pInitItem = mNativeHash.GetFirstItem(); pInitItem; pInitItem = mNativeHash.GetNextItem(pInitItem))
    {
        uNumKeys++;
    }

    return uNumKeys;
}

/**
 * Gets the (key, value) pair at @p uKeyIndex in the hash table.
 * @param uKeyIndex index into the hash table
 * @param keyName receives the key name
 * @return the value at @p uKeyIndex
 */
const AptValue *AptValueWithHash::GetKeyValuePair(unsigned int uKeyIndex, AptNativeString &keyName)
{
    APT_ASSERT(uKeyIndex < GetNumAptValues());

    unsigned int uCurrentIndex = 0;
    const AptValue *pAptValue  = NULL;
    keyName                    = "";

    for (AptHashItem *pInitItem = mNativeHash.GetFirstItem(); pInitItem; pInitItem = mNativeHash.GetNextItem(pInitItem))
    {
        if (uKeyIndex == uCurrentIndex)
        {
            keyName   = pInitItem->Key;
            pAptValue = pInitItem->mValue;
            break;
        }
        uCurrentIndex++;
    }

    return pAptValue;
}

void *AptPrototype::operator new(size_t size)
{
    return APT_GC_NEW(size);
}

void AptPrototype::operator delete(void *p, size_t size)
{
    AptValueGC::VerifyAptValueGC();
    APT_GC_DELETE(p, size);
}

/**
 * Cleans up all references.
 */
void AptPrototype::DestroyGCPointers()
{
    APT_DECSAFE(mp__constructor__);
    mp__constructor__ = NULL;
    AptValueWithHash::DestroyGCPointers();
}

/**
 * Registers all references.
 */
void AptPrototype::RegisterReferences()
{
    if (APT_REFERENCES_REGISTERED(this))
        return;

    AptValueWithHash::RegisterReferences();

    APT_REGISTER_REFERENCE_SAFE(mp__constructor__, "SuperConstructor", APT_REFREG_IS_APTVALUE);
    return;
}

/**
 * Overloaded to handle looking up the super constructor.
 * @param pContext not used, part of the virtual function prototype
 * @param pName name of the variable to look for
 * @return the AptValue for the name
 */
AptValue *AptPrototype::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    if (pName->Equal("__constructor__"))
    {
        return GetSuperConstructor();
    }
    else
    {
        return AptValueWithHash::objectMemberLookup(pContext, pName);
    }
}

/**
 * Overloaded to handle setting the super constructor.
 * @param pContext not used, part of the virtual function prototype
 * @param pName name of the property to be set
 * @param pValue AptValue to be set for the property
 * @return true if the property is set inside the object
 */
bool AptPrototype::objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue)
{
    if (pName->Equal("__constructor__"))
    {
        SetSuperConstructor(pValue);
        return (true);
    }
    else
    {
        return AptValueWithHash::objectMemberSet(pContext, pName, pValue);
    }
}

/**
 * Sets a new super constructor.
 * @param pNewSuperConstructor pointer to the new super constructor
 */
void AptPrototype::SetSuperConstructor(AptValue *pNewSuperConstructor)
{
    AptValue *pOldSuperConstructor = mp__constructor__;
    mp__constructor__              = pNewSuperConstructor;

    APT_INCSAFE(pNewSuperConstructor);
    APT_DECSAFE(pOldSuperConstructor);
}

void *AptObject::operator new(size_t size)
{
    return APT_GC_NEW(size);
}

void AptObject::operator delete(void *p, size_t size)
{
    AptValueGC::VerifyAptValueGC();
    APT_GC_DELETE(p, size);
}

/**
 * @param pContext the context passed in
 * @param pName the member name
 * @return the AptValue of the member, or 0 if unknown
 */
AptValue *AptObject::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    if (*pName == "registerClass")
    {
        return AptGetLib()->mpObjRegistrationFunc;
    }
    return 0;
}

/**
 * Registers all references.
 */
void AptObject::RegisterReferences()
{
    if (APT_REFERENCES_REGISTERED(this))
        return;

    AptValueWithHash::RegisterReferences();

    return;
}

/**
 * Cleans up all references in preparation for object deletion.
 */
void AptObject::DestroyGCPointers()
{
    AptValueWithHash::DestroyGCPointers();
    return;
}

/**
 * Sets the interfaces currently implemented by this object.
 * @param paImplementedObjects array of objects
 * @param nNumObjects number of elements in @p paImplementedObjects
 */
void AptObject::SetImplementedObjects(AptArray *paImplementedObjects, int nNumObjects)
{
#if APT_TRACK_IMPLEMENTED_INTERFACES
    AptNativeString sTmp("__INTERFACEs__");
    mNativeHash.Set(&sTmp, paImplementedObjects);
    mnImplementedObjects = nNumObjects;
#endif
}

/**
 * Gets the list of objects this implements. The returned array should not be
 * held onto for any length of time.
 * @param nOutNumObjects receives the length of the returned array
 * @return array of implemented objects
 */
AptArray *AptObject::GetImplementedObjects(int *nOutNumObjects) const
{
#if APT_TRACK_IMPLEMENTED_INTERFACES
    *nOutNumObjects = mnImplementedObjects;
    if (mnImplementedObjects > 0)
    {
        AptNativeString sTmp("__INTERFACES__");
        return mNativeHash.Lookup(&sTmp)->c_array();
    }
    else
#endif
    {
        return NULL;
    }
}

/**
 * Looks through the list of implemented objects, including the __proto__ chain.
 * @param pPrototype object to look for
 * @return true if the object implements the interface
 */
bool AptObject::DoesImplementObject(AptValue *pPrototype) const
{
    AptValue *pProto = mNativeHash.Get__Proto__();

    while (pProto)
    {
        if (pProto == pPrototype)
        {
            return true;
        }

        if (pProto->ContainsNativeHashVirtual())
        {
            pProto = pProto->GetNativeHashVirtual()->Get__Proto__();
        }
        else
        {
            break;
        }
    }

#if APT_TRACK_IMPLEMENTED_INTERFACES
    if (mnImplementedObjects > 0)
    {
        AptNativeString sTmp("__INTERFACES__");
        AptArray *pTmp = mNativeHash.Lookup(&sTmp)->c_array();
        APT_ASSERT(pTmp != NULL);
        for (uint32_t i = 0; i < mnImplementedObjects; i++)
        {
            if (pTmp->GetAt(i) == pPrototype)
            {
                return true;
            }
        }
    }
#endif

    return false;
}

/**
 * Makes a deep copy of the object by copying every key-value pair in its hash
 * table into a newly created object. Key strings are not copied, only their
 * reference count is incremented.
 * @param bRecursive if true, recurses into nested arrays
 * @return the newly copied object
 */
AptObject *AptObject::Clone(bool bRecursive) const
{
    AptObject *pNewObject = new AptObject(AptVFT_Object);

    APT_ASSERT(pNewObject);

    // copy internal members
    pNewObject->SetHasClass(this->GetHasClass());
    pNewObject->setInMainInst(this->getInMainInst());

    // have to cast this pointer to AptObject * even though AptObject is derived from AptValueWithHash because function is defined as const
    AptNativeHash *pObjHash    = ((AptObject *)this)->GetNativeHashVirtual();
    AptNativeHash *pNewObjHash = pNewObject->GetNativeHashVirtual();

    // copy proto and prototypes
    pNewObjHash->Set__Proto__(pObjHash->Get__Proto__());
    pNewObjHash->SetPrototype(pObjHash->GetPrototype());

    for (AptHashItem *pInitItem = pObjHash->GetFirstItem(); pInitItem; pInitItem = pObjHash->GetNextItem(pInitItem))
    {
        // Right now we only make a copy of int, float, bool, string.
        AptValue *pOldValue = pInitItem->mValue;
        if (pOldValue->isInteger())
        {
            pNewObjHash->Set(&pInitItem->Key, AptInteger::Create(pOldValue->c_integer()->GetInt()));
        }
        else if (pOldValue->isFloat())
        {
            pNewObjHash->Set(&pInitItem->Key, AptFloat::Create(pOldValue->c_float()->GetFloat()));
        }
        else if (pOldValue->isBoolean())
        {
            pNewObjHash->Set(&pInitItem->Key, AptBoolean::Create(pOldValue->c_boolean()->GetBool()));
        }
        else if (pOldValue->isString())
        {
            // Copy the string contents rather than calling StringPool::GetFromPool, otherwise
            // StringPool::Teardown asserts at shutdown.
            AptString *pNewString = AptString::Create(pOldValue->c_string()->GetInternalString()->c_str());
            pNewObjHash->Set(&pInitItem->Key, pNewString);
        }
        else if (pOldValue->isArray())
        {
            AptArray *pArray    = pOldValue->c_array();
            AptArray *pNewArray = new AptArray;
            int32_t nLen        = static_cast<int32_t>(pArray->length());
            for (int32_t i = 0; i < nLen; ++i)
            {
                AptValue *pVal = pArray->GetAt(i);
                if (bRecursive)
                {
                    if (pVal->isInteger())
                    {
                        pNewArray->set(i, AptInteger::Create(pVal->c_integer()->GetInt()));
                    }
                    else if (pVal->isFloat())
                    {
                        pNewArray->set(i, AptFloat::Create(pVal->c_float()->GetFloat()));
                    }
                    else
                    {
                        APT_ASSERT(false && "Cannot copy other AptValue types from AptArray");
                    }
                }
                else
                {
                    pNewArray->set(i, pVal);
                }
            }
            pNewObjHash->Set(&pInitItem->Key, pNewArray);
        }
        else if (pOldValue == gpUndefinedValue)
        {
            pNewObjHash->Set(&pInitItem->Key, gpUndefinedValue);
        }
        else
        {
            APT_ASSERT(false && "Not supporting any other AptValue types for clone operation");
        }
    }
    return pNewObject;
}

/**
 * @return the name of the class this prototype represents.
 */
const char *AptPrototype::GetClassName() const
{
#if defined(APT_RECORD_SCOPE_INFO) && APT_RECORD_SCOPE_INFO != 0
    return mClassName.GetBuffer();
#else
    return 0;
#endif
}

/**
 * Sets the name of the class this prototype represents.
 */
void AptPrototype::SetClassName(const char *name)
{
#if defined(APT_RECORD_SCOPE_INFO)
    if (mClassName.IsEmpty() && name && *name)
        mClassName = name;
#endif
}
