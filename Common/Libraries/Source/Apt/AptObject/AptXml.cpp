#include "AptXml.h"
#include "AptObject/AptNativeFunction.h"
#include "AptXmlInterfaces.h"
#include "_Apt.h"
#include "AptXmlAttributes.h"

#if defined(APT_USE_XML_OBJECT)
#include "AptObject/AptXmlConst.h"
#endif
#include "MainInline.h"


void *IAptXmlImpl::operator new(size_t size)
{
    AptUpdateAutoLock lock;
    return APT_NONGC_NEW(size);
}

void IAptXmlImpl::operator delete(void *p, size_t size)
{
    AptUpdateAutoLock lock;
    APT_NONGC_DELETE(p, size);
}

void *IAptXmlImpl::operator new[](size_t size)
{
    AptUpdateAutoLock lock;
    return AptNonGCAllocSaveSize(APT_ALLOC_PARAMS(size));
}

void IAptXmlImpl::operator delete[](void *p)
{
    AptUpdateAutoLock lock;
    AptNonGCFreeSavedSize(p);
}

void *AptXmlAttributePair::operator new(size_t size)
{
    AptUpdateAutoLock lock;
    return APT_NONGC_NEW(size);
}

void AptXmlAttributePair::operator delete(void *p, size_t size)
{
    AptUpdateAutoLock lock;
    APT_NONGC_DELETE(p, size);
}

void *AptXmlAttributePair::operator new[](size_t size)
{
    AptUpdateAutoLock lock;
    return AptNonGCAllocSaveSize(APT_ALLOC_PARAMS(size));
}

void AptXmlAttributePair::operator delete[](void *p)
{
    AptUpdateAutoLock lock;
    AptNonGCFreeSavedSize(p);
}

AptXml::~AptXml()
{
    //  Do nothing...
}

void AptXml::CleanNativeFunctions()
{
#if defined(APT_USE_XML_OBJECT)
    //      NATIVE_MEMBER_FUNCTION_DESTROY(contentType) ;
    //      NATIVE_MEMBER_FUNCTION_DESTROY(createElement) ;
    //      NATIVE_MEMBER_FUNCTION_DESTROY(createTextNode) ;
    //      NATIVE_MEMBER_FUNCTION_DESTROY(docTypeDecl) ;
    NATIVE_MEMBER_FUNCTION_DESTROY(getBytesTotal);
    NATIVE_MEMBER_FUNCTION_DESTROY(getBytesLoaded);
    //      NATIVE_MEMBER_FUNCTION_DESTROY(ignoreWhite) ;
    NATIVE_MEMBER_FUNCTION_DESTROY(load);
    NATIVE_MEMBER_FUNCTION_DESTROY(parseXml);
    //      NATIVE_MEMBER_FUNCTION_DESTROY(send) ;
    //      NATIVE_MEMBER_FUNCTION_DESTROY(sendAndLoad) ;
    //      NATIVE_MEMBER_FUNCTION_DESTROY(status) ;
#endif
}

#if defined(APT_USE_XML_OBJECT)

AptXml::AptXml(IAptXmlNode *pIXmlParam)
    : AptXmlNode(AptVFT_Xml, pIXmlParam)
{
    //  Do nothing...
}

AptXml::AptXml(AptValue *pszSource)
    : AptXmlNode(AptVFT_Xml, NULL)
{
    IAptXml *pNewAptXml;
    if (AptGetLib()->mpAptXmlImpl == NULL)
    {
        APT_ASSERT(AptGetLib()->mpAptXmlImpl);
    }
    // call IAptXmlImpl->createNewAptXml to get the new IAptXml *
    pNewAptXml = NULL;
    if (pszSource->isString())
    {
        pNewAptXml = AptGetLib()->mpAptXmlImpl->createNewAptXml(pszSource->c_string()->GetInternalString()->c_str());
    }
    else
    {
        pNewAptXml = AptGetLib()->mpAptXmlImpl->createNewAptXml();
    }
    if (pNewAptXml)
    {
        this->pIXmlNode = pNewAptXml;
    }
}

void AptXml::PreDestroy()
{
    delete pIXmlNode;
    pIXmlNode = NULL;
}

AptValue *AptXml::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    AptValue *pValueFromXmlNode = NULL;
    APT_ASSERT(pContext);
    if (pContext)
    {
        pContext->setVtblIndex(AptVFT_XmlNode);
        pValueFromXmlNode = AptXmlNode::objectMemberLookup(pContext, pName);
        pContext->setVtblIndex(AptVFT_Xml);
    }
    if ((pValueFromXmlNode) && pValueFromXmlNode->getIsDefined())
    {
        return pValueFromXmlNode;
    }
    XmlMembers *pProp = pContext ? XmlMemberIndex::in_word_set(pName->c_str(), pName->Size()) : NULL;
    if (pProp)
    {
        AptXmlNode *pXml = c_xml();
        IAptXml *pIXml   = (IAptXml *)pXml->pIXmlNode;
        switch (pProp->nIndex)
        {
        case AptXml_contentType:
        {
            AptString *pString = AptString::Create();
            pString->cpy(*StringPool::GetString(SC_null));
            if (pXml->pIXmlNode)
            {
                char *szNodeValue = NULL;
                szNodeValue       = pIXml->contentType();
                if (szNodeValue)
                {
                    pString->cpy(szNodeValue);
                }
            }
            return pString;
        }
        break;
        case AptXml_createElement:

        {
            //              NATIVE_MEMBER_FUNCTION_DISPATCH(createElement);
        }
        break;
        case AptXml_createTextNode:

        {
            //              NATIVE_MEMBER_FUNCTION_DISPATCH(createTextNode);
        }
        break;
        case AptXml_docTypeDecl:

        {
            AptString *pString = AptString::Create();
            pString->cpy(*StringPool::GetString(SC_null));
            if (pXml->pIXmlNode)
            {
                char *szDocTypeDecl = NULL;
                szDocTypeDecl       = pIXml->docTypeDecl();
                if (szDocTypeDecl)
                {
                    pString->cpy(szDocTypeDecl);
                }
            }
            return pString;
        }
        break;
        case AptXml_getBytesTotal:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(getBytesTotal);
        }
        break;
        case AptXml_getBytesLoaded:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(getBytesLoaded);
        }
        break;
        case AptXml_ignoreWhite:

        {
            return AptBoolean::Create(pIXml->isIgnoreWhite() != 0);
        }
        break;
        case AptXml_load:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(load);
        }
        break;
        case AptXml_loaded:

        {
            // read-only property
            return AptBoolean::Create(pIXml->isLoaded() != 0);
        }
        break;
        case AptXml_parseXml:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(parseXml);
        }
        break;
        case AptXml_send:

        {
            //              NATIVE_MEMBER_FUNCTION_DISPATCH(send);
        }
        break;
        case AptXml_sendAndLoad:

        {
            //              NATIVE_MEMBER_FUNCTION_DISPATCH(sendAndLoad);
        }
        break;
        case AptXml_status:

        {
            return AptInteger::Create(pIXml->status());
        }
        }
    }
    return 0;
}

//----------------------------------------------------------------------------------------------------------
// AptXml functions
// NATIVE_MEMBER_FUNCTION(AptXml, contentType)
//{
//  return gpUndefinedValue ;
//}
// NATIVE_MEMBER_FUNCTION(AptXml, createElement)
//{
//  return gpUndefinedValue ;
//}
// NATIVE_MEMBER_FUNCTION(AptXml, createTextNode)
//{
//  return gpUndefinedValue ;
//}

// NATIVE_MEMBER_FUNCTION(AptXml, docTypeDecl)
//{
//   return gpUndefinedValue ;
// }
NATIVE_MEMBER_FUNCTION(AptXml, getBytesTotal)
{
    int iBytesTotal = 0;
    if (pThis->isXml())
    {
        AptXmlNode *pXml = pThis->c_xml();
        IAptXml *pIXml   = (IAptXml *)pXml->pIXmlNode;
        if (pIXml)
            iBytesTotal = pIXml->getBytesTotal();
    }
    return AptInteger::Create(iBytesTotal);
}
NATIVE_MEMBER_FUNCTION(AptXml, getBytesLoaded)
{
    int iBytesLoaded = 0;
    if (pThis->isXml())
    {
        AptXmlNode *pXml = pThis->c_xml();
        IAptXml *pIXml   = (IAptXml *)pXml->pIXmlNode;
        if (pIXml)
            iBytesLoaded = pIXml->getBytesLoaded();
    }
    return AptInteger::Create(iBytesLoaded);
}
// NATIVE_MEMBER_FUNCTION(AptXml, ignoreWhite)
//{
//   return gpUndefinedValue ;
// }
NATIVE_MEMBER_FUNCTION(AptXml, load)
{
    if (nParams < 1)
        return gpUndefinedValue;

    if (pThis->isXml())
    {
        AptValue *pUrl = gAptActionInterpreter.stackAt(0);
        if (pUrl->isString())
        {
            AptNativeString sBuf;
            pUrl->toString(sBuf);
            //          AptXmlNode * pXml = pThis->c_xml() ;
            IAptXml *pIXml = (IAptXml *)pThis->c_xml()->pIXmlNode;
            if (pIXml)
                pIXml->load(sBuf.ConstRawPtr());
        }
    }
    return gpUndefinedValue;
}
// NATIVE_MEMBER_FUNCTION(AptXml, isLoaded)
//{
//   return gpUndefinedValue ;
// }
NATIVE_MEMBER_FUNCTION(AptXml, parseXml)
{
    if (nParams < 1)
        return gpUndefinedValue;

    if (pThis->isXml())
    {
        AptValue *pSource = gAptActionInterpreter.stackAt(0);
        if (pSource->isString())
        {
            AptNativeString sBuf;
            pSource->toString(sBuf);
            AptXmlNode *pXml = pThis->c_xml();
            IAptXml *pIXml   = (IAptXml *)pXml->pIXmlNode;
            if (pIXml)
                pIXml->parseXml(sBuf.ConstRawPtr());
        }
    }
    return gpUndefinedValue;
}
////NATIVE_MEMBER_FUNCTION(AptXml, send)
////{
////    return gpUndefinedValue ;
////}
////NATIVE_MEMBER_FUNCTION(AptXml, sendAndLoad)
////{
////    return gpUndefinedValue ;
////}
// NATIVE_MEMBER_FUNCTION(AptXml, status)
//{
//   return gpUndefinedValue ;
// }

#endif // #if defined (APT_USE_XML_OBJECT)
/*H*************************************************************************************************
    EOF
*************************************************************************************************H*/
