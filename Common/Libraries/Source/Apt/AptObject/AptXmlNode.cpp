#include "AptXmlNode.h"
#include "AptObject/AptNativeFunction.h"
#include "AptXmlInterfaces.h"
#include "AptObject/AptXmlConst.h"
#include "AptValue/AptString.h"
#include "AptXmlAttributes.h"

#include "MainInline.h"

#if defined(APT_USE_XML_OBJECT)
AptXmlNode::AptXmlNode(AptVirtualFunctionTable_Indices eType, IAptXmlNode *pIXmlNodeParam)
    : AptObject(eType),
      pIXmlNode(pIXmlNodeParam),
      paChildNodes(NULL)
{
    //  Do nothing...
}

void AptXmlNode::PreDestroy()
{
    pIXmlNode = NULL;
}
#endif // #if defined (APT_USE_XML_OBJECT)
AptXmlNode::~AptXmlNode()
{
#if defined(APT_USE_XML_OBJECT)
    pIXmlNode    = NULL;
    paChildNodes = NULL;
#endif
}

void AptXmlNode::CleanNativeFunctions()
{
#if defined(APT_USE_XML_OBJECT)
    NATIVE_MEMBER_FUNCTION_DESTROY(appendChild);
    NATIVE_MEMBER_FUNCTION_DESTROY(cloneNode);
    NATIVE_MEMBER_FUNCTION_DESTROY(hasChildNodes);
    NATIVE_MEMBER_FUNCTION_DESTROY(insertBefore);
    NATIVE_MEMBER_FUNCTION_DESTROY(removeNode);
    NATIVE_MEMBER_FUNCTION_DESTROY(toString);
#endif
}

#if defined(APT_USE_XML_OBJECT)
bool AptXmlNode::objectMemberSet(AptValue *const pContext, const AptNativeString *const pName, AptValue *const pValue)
{
    if (pContext->isXmlNode())
    {
        AptXmlNode *pXmlNode = pContext->c_xmlnode();
        if (pName->EqualNoCase("NodeName"))
        {
            if (pValue->isString())
            {
                AptNativeString sBuf;
                pValue->toString(sBuf);
                if (pXmlNode->pIXmlNode)
                    pXmlNode->pIXmlNode->setNodeName(sBuf.ConstRawPtr());
            }
        }
        else if (pName->EqualNoCase("NodeValue"))
        {
            if (pValue->isString())
            {
                AptNativeString sBuf;
                pValue->toString(sBuf);
                if (pXmlNode->pIXmlNode)
                    pXmlNode->pIXmlNode->setNodeValue(sBuf.ConstRawPtr());
            }
        }
    }
    return (true);
}

AptValue *AptXmlNode::objectMemberLookup(AptValue *const pContext, const AptNativeString *const pName) const
{
    XmlMembers *pProp    = pContext ? XmlMemberIndex::in_word_set(pName->c_str(), pName->Size()) : NULL;
    AptXmlNode *pXmlNode = c_xmlnode();
    // IAptXmlNode * pIXmlNode1 = (IAptXmlNode *) pXmlNode->pIXmlNode ;  // unused

    if (pProp)
    {
        switch (pProp->nIndex)
        {
        case AptXmlNode_appendChild:
        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(appendChild);
        }
        break;
        case AptXmlNode_attributes:

        {
            AptXmlAttributes *pXmlAttr = new AptXmlAttributes(AptVFT_XmlAttributes, pXmlNode->pIXmlNode);
            if (pXmlNode->pIXmlNode)
            {
                // call getFirstAttribute, and getNextAttribute to create fill in AptXmlAtrributes Object.
                AptXmlAttributePair XmlAttribPair = pXmlNode->pIXmlNode->getFirstAttribute();
                while (XmlAttribPair.pKey && XmlAttribPair.pValue)
                {
                    // valid XmlAttribPair so fill that in AptXmlAtrributes
                    AptString *pAttribValue = AptString::Create();
                    pAttribValue->cpy(XmlAttribPair.pValue);
                    AptNativeString strKey(XmlAttribPair.pKey);
                    pXmlAttr->Set(&strKey, pAttribValue);
                    XmlAttribPair = pXmlNode->pIXmlNode->getNextAttribute();
                }
            }
            if (pXmlAttr)
                return pXmlAttr;
            else
                return gpUndefinedValue;
        }
        break;
        case AptXmlNode_childNodes:

        {
            AptArray *paChildNodes = NULL;
            if (pXmlNode->pIXmlNode)
            {
                // everytime childNodes array is referenced, just create a new one
                // by allocating new AptArray, and filling it with newly created
                // AptXmlNode objects which wrap up return values from getFirstChildNode(),
                // and getNextChildNode()

                // create new childNode array every time it is accessed as it might have
                //  changed in between last and this access to childNodes
                paChildNodes = new AptArray();
                if (paChildNodes)
                {
                    // create AptXmlNode objects, store pointers to IAptXmlNodes in them
                    // and add them to array.

                    //                      IAptXml * pIXmlNode1 = (IAptXml *) pXmlNode->pIXmlNode ;
                    IAptXmlNode *pChildNode = pXmlNode->pIXmlNode->getFirstChildNode();
                    //                      IAptXmlNode * pChildNode = pIXmlNode1->getFirstChildNode() ;

                    int iArrIndex = 0;
                    while (pChildNode)
                    {
                        AptXmlNode *pTempAptXmlNode = new AptXmlNode(AptVFT_XmlNode, pChildNode);
                        paChildNodes->set(iArrIndex++, pTempAptXmlNode);
                        pChildNode = pXmlNode->pIXmlNode->getNextChildNode();
                    }
                    return paChildNodes;
                }
            }
            else
                return gpUndefinedValue;
        }
        break;
        case AptXmlNode_cloneNode:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(cloneNode);
        }
        break;
        case AptXmlNode_firstChild:

        {
            // this is a read-only property. call IAptXmlNode->firstChild
            if (pXmlNode->pIXmlNode)
            {
                IAptXmlNode *pFirstChildNode = pXmlNode->pIXmlNode->firstChild();
                // This property should be null if the node does not have children. This property is undefined if the node is a text node.
                // here we just check if xml implementation returns a valid child or not otherwise we just return undefined
                if (pFirstChildNode)
                {
                    AptXmlNode *pFirstChildAptXmlNode = new AptXmlNode(AptVFT_XmlNode, pFirstChildNode);
                    if (pFirstChildAptXmlNode->getIsDefined())
                    {
                        return pFirstChildAptXmlNode;
                    }
                }
                else
                    return gpUndefinedValue;
            }
            else
                return gpUndefinedValue;
        }
        break;
        case AptXmlNode_hasChildNodes:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(hasChildNodes);
        }
        break;
        case AptXmlNode_insertBefore:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(insertBefore);
        }
        break;
        case AptXmlNode_lastChild:

        {
            // this is a read-only property. call IAptXmlNode->lastChild
            if (pXmlNode->pIXmlNode)
            {
                IAptXmlNode *pLastChildNode = pXmlNode->pIXmlNode->lastChild();
                // This property should be null if the node does not have children. This property is undefined if the node is a text node.
                // here we just check if xml implementation returns a valid child or not otherwise we just return undefined
                if (pLastChildNode)
                {
                    AptXmlNode *pLastChildAptXmlNode = new AptXmlNode(AptVFT_XmlNode, pLastChildNode);
                    if (pLastChildAptXmlNode->getIsDefined())
                    {
                        return pLastChildAptXmlNode;
                    }
                }
                else
                    return gpUndefinedValue;
            }
            else
                return gpUndefinedValue;
        }
        break;
        case AptXmlNode_nextSibling:

        {
            // this is a read-only property. call IAptXmlNode->nextSibling
            if (pXmlNode->pIXmlNode)
            {
                IAptXmlNode *pNextSiblingNode = pXmlNode->pIXmlNode->nextSibling();
                // This property should be null if the node does not have children. This property is undefined if the node is a text node.
                // here we just check if xml implementation returns a valid child or not otherwise we just return undefined
                if (pNextSiblingNode)
                {
                    AptXmlNode *pNextSiblingAptXmlNode = new AptXmlNode(AptVFT_XmlNode, pNextSiblingNode);
                    if (pNextSiblingAptXmlNode->getIsDefined())
                    {
                        return pNextSiblingAptXmlNode;
                    }
                }
                else
                    return gpUndefinedValue;
            }
            else
                return gpUndefinedValue;
        }
        break;
        case AptXmlNode_nodeName:

        {
            AptString *pString = AptString::Create();
            pString->cpy(*StringPool::GetString(SC_null));
            if (pXmlNode->pIXmlNode)
            {
                char *szNodeName = NULL;
                szNodeName       = pXmlNode->pIXmlNode->nodeName();
                if (szNodeName)
                {
                    pString->cpy(szNodeName);
                }
            }
            return pString;
        }
        break;
        case AptXmlNode_nodeType:

        {
            if (pXmlNode->pIXmlNode)
            {
                return AptInteger::Create(pXmlNode->pIXmlNode->nodeType());
            }
            else
                return gpUndefinedValue;
        }
        break;
        case AptXmlNode_nodeValue:

        {
            AptString *pString = AptString::Create();
            pString->cpy(*StringPool::GetString(SC_null));
            if (pXmlNode->pIXmlNode)
            {
                char *szNodeValue = NULL;
                szNodeValue       = pXmlNode->pIXmlNode->nodeValue();
                if (szNodeValue)
                {
                    pString->cpy(szNodeValue);
                }
            }
            return pString;
        }
        break;
        case AptXmlNode_parentNode:

        {
            // this is a read-only property. call IAptXmlNode->parentNode()
            AptXmlNode *pParentAptXmlNode = (AptXmlNode *)gpUndefinedValue;
            if (pXmlNode->pIXmlNode)
            {
                IAptXmlNode *pParentNode = pXmlNode->pIXmlNode->parentNode();
                // This property should be null if the node does not have children. This property is undefined if the node is a text node.
                // here we just check if xml implementation returns a valid child or not otherwise we just return undefined
                if (pParentNode)
                {
                    pParentAptXmlNode = new AptXmlNode(AptVFT_XmlNode, pParentNode);
                    if (pParentAptXmlNode->getIsDefined())
                    {
                        return pParentAptXmlNode;
                    }
                }
            }
            return pParentAptXmlNode;
        }
        break;
        case AptXmlNode_previousSibling:

        {
            // this is a read-only property. call IAptXmlNode->previousSibling
            if (pXmlNode->pIXmlNode)
            {
                IAptXmlNode *pPreviousSiblingNode = pXmlNode->pIXmlNode->previousSibling();
                // This property should be null if the node does not have children. This property is undefined if the node is a text node.
                // here we just check if xml implementation returns a valid child or not otherwise we just return undefined
                if (pPreviousSiblingNode)
                {
                    AptXmlNode *pPreviousSiblingAptXmlNode = new AptXmlNode(AptVFT_XmlNode, pPreviousSiblingNode);
                    if (pPreviousSiblingAptXmlNode->getIsDefined())
                    {
                        return pPreviousSiblingAptXmlNode;
                    }
                }
                else
                    return gpUndefinedValue;
            }
            else
                return gpUndefinedValue;
        }
        break;
        case AptXmlNode_removeNode:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(removeNode);
        }
        break;
        case AptXmlNode_toString:

        {
            NATIVE_MEMBER_FUNCTION_DISPATCH(toString);
        }
        }
    }
    return 0;
}

// AptXmlNode function
NATIVE_MEMBER_FUNCTION(AptXmlNode, appendChild)
{
    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptXmlNode, cloneNode)
{
    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptXmlNode, hasChildNodes)
{
    if (!pThis->isXmlNode())
        return AptBoolean::Create(0);
    AptXmlNode *pXmlNode = pThis->c_xmlnode();

    if (pXmlNode->pIXmlNode)
    {
        int bHasChildNodes = 0;
        bHasChildNodes     = pXmlNode->pIXmlNode->hasChildNodes();
        return AptBoolean::Create(bHasChildNodes != 0);
    }
    else
        return AptBoolean::Create(0);
}
NATIVE_MEMBER_FUNCTION(AptXmlNode, insertBefore)
{
    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptXmlNode, removeNode)
{
    return gpUndefinedValue;
}

NATIVE_MEMBER_FUNCTION(AptXmlNode, toString)
{
    if (!pThis->isXmlNode())
        return gpUndefinedValue;

    AptXmlNode *pXmlNode = pThis->c_xmlnode();
    AptString *pString   = AptString::Create();
    pString->cpy(*StringPool::GetString(SC_null));
    if (pXmlNode->pIXmlNode)
    {
        char *szToString = NULL;
        szToString       = pXmlNode->pIXmlNode->toString();
        if (szToString)
            pString->cpy(szToString);
    }
    return pString;
}
#endif // #if defined (APT_USE_XML_OBJECT)
/*H*************************************************************************************************
    EOF
*************************************************************************************************H*/
