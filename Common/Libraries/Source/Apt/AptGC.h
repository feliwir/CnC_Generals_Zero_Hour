/**
 * This header contains the definition of the Garbage Collector.
 */

#pragma once
#include "AptValue/AptValue.h"

class AptValue;

class AptGC
{
  public:
    //  Prototypes for GC AptValue
    //  Prototypes for other traceable objects
    using GetObjectNumGCPointersCallback = int (*)(void *const pObject);
    using GetObjectGCPointerCallback     = AptValue *(*)(void *const pObject, const int nIndex);

    static void Initialize();

    static bool IsValueGarbageCollected(const AptValue *const pValue);

    static void CleanUnreachable();
    static void CleanAll();

    struct GCLinkedList
    {
        AptValue *mPrev;
        AptValue *mNext;
    };

    static void ReplaceReferences(AptValue *pValue, AptValue *pNewObj, AptValue **pAllocatedValues = NULL, int32_t nItemsInArray = 0);
#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)
    static void PrintZombieExternalReferenceMapXML(AptCIH *pParent);
#endif

  private:
    struct RegisteredTraceObject;

    AptGC();
    explicit AptGC(const AptGC &Other);
    ~AptGC();
    AptGC &operator=(const AptGC &Other);

    static void sReferenceRegistrationCb(const AptValue *pFromRef, AptValue *&pToRef, const char *pReferenceName, int32_t bFlag);

#if defined(APT_XML_MEMORY_DUMP_SUPPORTED)
    // This function needs to be able to traverse the object list, it won't modify anything.
    friend void PrintObjectMapXML(const char *psDumpName);
#endif
};
