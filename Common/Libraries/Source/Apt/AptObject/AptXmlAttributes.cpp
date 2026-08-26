#include "AptXmlAttributes.h"
#include "AptXmlInterfaces.h"
#include "AptValue/AptString.h"

#include "MainInline.h"

AptXmlAttributes::~AptXmlAttributes()
{
#if defined(APT_USE_XML_OBJECT)
    pIXmlNode = NULL;
#endif
}

#if defined(APT_USE_XML_OBJECT)
AptXmlAttributes::AptXmlAttributes(AptVirtualFunctionTable_Indices eType, IAptXmlNode *pIXmlNodeParam)
    : AptObject(eType),
      pIXmlNode(pIXmlNodeParam)
{
    //  Do nothing...
}

void AptXmlAttributes::PreDestroy()
{
    pIXmlNode = NULL;
}

bool AptXmlAttributes::objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue)
{
    if (pContext->isXmlAttributes())
    {
        AptXmlAttributes *pXmlAttrib = pContext->c_xmlattributes();
        if (pValue->isString())
        {
            pXmlAttrib->pIXmlNode->setAttribute(pName->c_str(), pValue->c_string()->GetInternalString()->c_str());
        }
    }
    return (true);
}
#endif // #if defined (APT_USE_XML_OBJECT)
/*H*************************************************************************************************
    EOF
*************************************************************************************************H*/
