/*H************************************************************************************************/
/**

    @file    AptExtObject.h

    @brief
        Base class for extending Apt with C++ code.

    @note
        None.

    Copyright
        (c) 2004 Electronic Arts Inc.

    Version    1.0        03/29/04 First Version
    Version    1.1        11/01/04 Updated with a means of keeping track of the inherited
                                          object size. This is required by the uber-slick Garbage
                                          collectOR. The theory behind it is that we hook the new
                                          operator to store the size of the object into a global
                                          variable, and then store it as a member in the
                                          constructor.
    Version    1.2        02/10/08 Added new macros to reduce the size of code generated in
                                          Initialize(). See APTEXT_FUNCTION_INIT_ARRAY_DECL.

    Version    1.3        06/08/09 Removed APT_INLINE from static method declarations.
*/
/************************************************************************************************H*/

#pragma once

/**
\Module AptSDK
*/
//@{

/*** Include files ********************************************************************************/

#include "AptValue/AptValue.h"
#include "AptValue/AptInteger.h"
#include "AptValue/AptFloat.h"
#include "AptValue/AptBoolean.h"
#include "AptValue/AptString.h"

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*F************************************************************************************************/
/**
    Function    APTEXT_FUNCTION_DECL

    @brief
        Macro for declaring an Apt C++ function in an AptExtObject derived class

    @param      _name_      - the name to reference the function by in Actionscript

    @note
        None.

    Version    1.0        04/02/04 First Version

*/
/************************************************************************************************F*/
#define APTEXT_FUNCTION_DECL(_name_)                                 \
    static AptValue *sMethod_##_name_(AptValue *pThis, int nParams); \
    static AptNativeFunction *psMethod_##_name_;

/*F************************************************************************************************/
/**
    Function    APTEXT_FUNCTION

    @brief
        Macro for defining an Apt C++ function in an AptExtObject derived class

    @param      _class_     - the name of the AptExtObject derived class the function is a member of
    @param      _name_      - the name of the function used in the corresponding APT_EXT_FUNCTION_DECL

    @note
        None.

    Version    1.0        04/02/04 First Version

*/
/************************************************************************************************F*/
#define APTEXT_FUNCTION(_class_, _name_)               \
    AptNativeFunction *_class_::psMethod_##_name_ = 0; \
    AptValue *_class_::sMethod_##_name_(AptValue *pThis, int nParams)

/*F************************************************************************************************/
/**
    Function    APTEXT_FUNCTION_INIT

    @brief
        Deprecated! See APTEXT_FUNCTION_INIT_ARRAY_DECL below.
        Macro for initializing an Apt C++ function.  You should call this once for each function
        in your version of Initialize();

    @param      _name_      - the name of the function used in the corresponding APT_EXT_FUNCTION_DECL

    @note
        Deprecated! See APTEXT_FUNCTION_INIT_ARRAY_DECL below.


    Version    1.0        04/02/04 First Version
    Version    1.1        02/10/08 Note as obsolete.

*/
/************************************************************************************************F*/
#define APTEXT_FUNCTION_INIT(_name_)                                               \
    psMethod_##_name_ = CreateNewAptFunction((AptExtFunctionPtr)sMethod_##_name_); \
    SetFunction(#_name_, psMethod_##_name_);

/*F************************************************************************************************/
/**
    Function    APTEXT_FUNCTION_INIT_ARRAY_START

    @brief
        Declares the beginning of a list of methods for an AptExtObject-derived class.
        You should call this once at the beginning of your Initialize() method.

    @param      _name_      - the name of the function used in the corresponding APT_EXT_FUNCTION_DECL

    @note
        none

    Version    1.0        02/10/08 First Version

*/
/************************************************************************************************F*/
#define APTEXT_FUNCTION_INIT_ARRAY_START \
    {                                    \
        static AptExtFunctionRecord arrAptExtFunctionRecords[] = {

/*F************************************************************************************************/
/**
    Function    APTEXT_FUNCTION_INIT_ARRAY_DECL

    @brief
        Macro for initializing an Apt C++ function.  You should call this once for each function
        in your version of Initialize().  You must wrap all instances of APTEXT_FUNCTION_INIT_ARRAY_DECL
        within APTEXT_FUNCTION_INIT_ARRAY_START and APTEXT_FUNCTION_INIT_ARRAY_END.

    @param      _name_      - the name of the function used in the corresponding APT_EXT_FUNCTION_DECL

    @note
        none

    Version    1.0        02/10/08 First Version

*/
/************************************************************************************************F*/
#define APTEXT_FUNCTION_INIT_ARRAY_DECL(_name_) {psMethod_##_name_, (AptExtFunctionPtr)sMethod_##_name_, (const char *)#_name_},

/*F************************************************************************************************/
/**
    Function    APTEXT_FUNCTION_INIT_ARRAY_END

    @brief
        Declares the end of a list of methods for an AptExtObject-derived class.
        You should call this once at the end of your Initialize() method.

    Input
        none

    @note
        none

    Version    1.0        02/10/08 First Version

*/
/************************************************************************************************F*/
#define APTEXT_FUNCTION_INIT_ARRAY_END                                                                             \
    }                                                                                                              \
    ;                                                                                                              \
    const int nAptExtFunctionRecordCount = sizeof(arrAptExtFunctionRecords) / sizeof(arrAptExtFunctionRecords[0]); \
    for (int i = 0; i < nAptExtFunctionRecordCount; i++)                                                           \
    {                                                                                                              \
        AptNativeFunction *psMethod = arrAptExtFunctionRecords[i].psMethod;                                        \
        AptExtFunctionPtr sMethod   = arrAptExtFunctionRecords[i].sMethod;                                         \
        const char *name            = arrAptExtFunctionRecords[i].name;                                            \
        psMethod                    = CreateNewAptFunction((AptExtFunctionPtr)sMethod);                            \
        SetFunction(name, psMethod);                                                                               \
    }                                                                                                              \
    }

/*** Type Definitions *****************************************************************************/

// forward declarations
class AptNativeFunction;
class AptNativeHash;

// typedef so we don't have to expose Apt native function prototype
using AptExtFunctionPtr = void *;

/** @brief Base class for an Apt Extension object

    Classes derived from AptExtObject can be registered with Apt, and then used in ActionScript as
    objects in _global scope.  To implement an AptExtObject derived class, do the following:

    1.  Use the macro APTEXT_FUNCTION_DECL to declare C++ extension functions in your
        class definition.

    2.  Use the macro APTEXT_FUNCTION to define C++ extension functions in your class'
        C++ file.

    3.  In youd class' constructor, call the AptExtObject constructor and pass it the
        initial hash table size to use.

    4.  Implement GetName to return the name the object can be referenced by in actionscript.

    5.  Implement Initialize.  The body of Initialize should do the following: Call
        APTEXT_FUNCTION_INIT_ARRAY_START; call APTEXT_FUNCTION_INIT_ARRAY_DECL once
        for each C++ extension method declared with APTEXT_FUNCTION_DECL; then end the list
        of methods by calling APTEXT_FUNCTION_INIT_ARRAY_END. See example below.


    To register the AptExtObject class with Apt, after calling AptInitialize, call
    AptRegisterExtension with a pointer to an object of your class type.  You will not have
    to track the object or destroy it.  Apt will clean it up for you on shutdown.
    Here is an example registration.

    Example MyAptExtClass.h file
    @verbatim
    #ifndef _MYAPTEXTCLASS_H_
    #define _MYAPTEXTCLASS_H_

    #include "AptExtObject.h"

    #define APTEXT_MYOBJECT_NAME        ("MyAptExtObject")
    #define APTEXT_MYOBJECT_NUMFNCS     (2)

    class MyAptExtClass : public AptExtObject
    {
    public:
        MyAptExtClass()
            : AptExtObject(APTEXT_MYOBJECT_NUMFNCS)
        {}

        virtual ~MyAptExtClass() {}

        // declare C++ extension functions
        APTEXT_FUNCTION_DECL(MyFirstFunction)
        APTEXT_FUNCTION_DECL(MySecondFunction)

        virtual const char *GetName(void) { return (APTEXT_MYOBJECT_NAME); }
        virtual void Initialize(void);
    };

    #endif // _MYAPTEXTCLASS_H_
    @endverbatim

    Example MyAptExtClass.cpp file
    @verbatim
    #include "MyAptExtClass.h"

    void MyAptExtClass::Initialize(void)
    {
        // initialize C++ extension functions
        APTEXT_FUNCTION_INIT_ARRAY_START

            APTEXT_FUNCTION_INIT_ARRAY_DECL(MyFirstFunction)
            APTEXT_FUNCTION_INIT_ARRAY_DECL(MySecondFunction)

        APTEXT_FUNCTION_INIT_ARRAY_END
    }

    // in actionscript, MyFirstFunction takes 1 parameter and checks to see
    // if it is equal to 5.  it returns true or false.
    APTEXT_FUNCTION(MyAptExtClass, MyFirstFunction)
    {
        // get the parameter
        AptValue *pParam = GetParam(0);
        AptValue *pRetVal = NULL;
        bool8_t bIsEqualTo5 = false;

        // is it an integer
        APT_ASSERT(pParam != NULL);
        if (pParam->isInteger())
        {
            int32_t iParam = pParam->toTnteger();
            bIsEqualTo5 = (iParam == 5);
        }

        // create the return value
        pRetVal = AptBoolean::Create(bIsEqualTo5);
        return pRetVal;
    }

    // in actionscript, MySecondFunction takes an object as a parameter.  It looks
    // up a count member in the object, and returns another object with count value
    // in it.  If the object passed in does not have the count, it returns the
    // undefined object
    APTEXT_FUNCTION(MyAptExtClass, MyFirstFunction)
    {
        // get the parameter
        AptValue *pInputObject = GetParam(0);
        AptValue *pRetVal = NULL;

        // lookup the count
        APT_ASSERT(pInputObject != NULL);
        AptNativeString sCount("m_sCount");
        AptValue *pCountValue = GetVariable(pInputObject, &sCount);
        if (pCountValue != NULL)
        {
            // we found the count value, so create our return object and
            // add the count to it.
            pRetVal = CreateNewAptObject(1);
            SetVariable(pRetVal, &sCount, pCountValue);
        }
        else
        {
            // we didn't find the count value, so return undefined
            pRetVal = GetUndefinedValue();
        }

        return pRetVal;
    }
    @endverbatim

    Example code to register the object
    @verbatim
    ...
    AptInitialize(NULL);
    AptRegisterExtension(new MyAptExtClass());  // give Apt a pointer to our extension object
    ...
    @endverbatim

*/
class AptExtObject : public AptValueGC
{
  public:
    static void *operator new(size_t size);

    static void operator delete(void *p, size_t size)
    {
        APT_GC_DELETE(p, size);
    }
    static void *operator new[](size_t size)
    {
        APT_FAIL("Garbage collected Objects should never be created in Arrays!!!");
        return AptGetUserFuncs().pfnMemAlloc(APT_ALLOC_PARAMS(size));
    }
    static void operator delete[](void *p)
    {
        APT_FAIL("Garbage collected Objects should never be created in Arrays!!!");
        AptGetUserFuncs().pfnMemFree(p);
    }


    AptExtObject(const int32_t iNumMembers);
    virtual ~AptExtObject(void);

    AptValue *Lookup(const AptNativeString *const pKey) const;

    void Set(const AptNativeString *const pKey, AptValue *const pValue);

    // APT_ACCESS_INTERNAL:

    //! Gets the name the object will be referenced by in actionscript
    virtual const char *GetName(void) = 0;
    //! Called by Apt to actually create the C++ functions
    virtual void Initialize(void) = 0;

    //! looks up an AptValue within the extension object
    AptValue *objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const;
    //! sets an AptValue within the extension object, if one with the same name doesn't already exist
    bool objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue);

    virtual bool IsGarbageCollected() const
    {
        return (true);
    }

    virtual AptNativeHash *GetNativeHashVirtual();
    virtual bool ContainsNativeHashVirtual() const;
    virtual void RegisterReferences();
    virtual void DestroyGCPointers();

    uint32_t GetSize() const
    {
        return mnObjectSize;
    }

  protected:
    //! sets a function pointer by name in the extension object
    void SetFunction(const char *pKey, AptNativeFunction *pFunction);

    //! returns a pointer to the undefined value object
    static AptValue *GetUndefinedValue(void);
    //! returns a pointer to a function parameter
    static AptValue *GetParam(const int32_t iParam);
    //! creates a new AptObject object with a default size hash table and returns a pointer to it
    static AptValue *CreateNewAptObject(void);
    //! creates a new AptObject object with a hash table of specified size and returns a pointer to it
    static AptValue *CreateNewAptObject(const int32_t iHashSize);
    //! creates a new AptNativeFunction from the passed in C++ function, and returns a pointer
    static AptNativeFunction *CreateNewAptFunction(AptExtFunctionPtr pAptExtFnc);
    //! gets a variable by name from the specified context and returns a pointer to it
    static AptValue *GetVariable(AptValue *pContext, const AptNativeString *const pVariable);
    //! sets a variable by name in the specified context
    static bool SetVariable(AptValue *pContext, const AptNativeString *const pName, AptValue *const pValue);

  protected:
    //! Used by APT_INIT_FUNCTION_ARRAY_DECL
    struct AptExtFunctionRecord
    {
        AptNativeFunction *psMethod;
        AptExtFunctionPtr sMethod;
        const char *name;
    };

  private:
    AptNativeHash *mpNativeHash;
    uint32_t mnObjectSize;

    AptExtObject(const AptExtObject &);
    AptExtObject &operator=(const AptExtObject &);
};

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

//@}
