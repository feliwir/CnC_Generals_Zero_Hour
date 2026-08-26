#pragma once

#include "AptObject/AptObject.h"
#include "_AptInternalDefines.h"

class IAptXmlNode;

class AptXmlAttributes : public AptObject
{
  public:
#if defined(APT_USE_XML_OBJECT)
    explicit AptXmlAttributes(AptVirtualFunctionTable_Indices eType, IAptXmlNode *pIXmlNodeParam = NULL);

    virtual void PreDestroy();

    virtual bool objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue);

  public:
    IAptXmlNode *pIXmlNode; // this object could be pointing to IAptXml also
#else
    explicit AptXmlAttributes(AptVirtualFunctionTable_Indices eType, IAptXmlNode *pIXmlNodeParam = NULL) : AptObject(eType)
    {
        APT_ASSERT(0 && "APT-Warning-XML object cannot be used as it is compiled out, make sure to compile with APT_USE_XML_OBJECT defined");
    }
#endif
  protected:
    virtual ~AptXmlAttributes();

  public: // because AptXmlNode calls our operator new
    APT_VALUE_GC_NEW_DELETE_OPERATORS
  private:
};
