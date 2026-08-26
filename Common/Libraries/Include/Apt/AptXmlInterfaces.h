#pragma once

/**
 * Interfaces that the user can implement in order to parse XML data.
 */

#include "Apt.h"
#include <stddef.h>

class IAptXml;

/// This interface works as a factory kind of interface. User needs to derive a class from this interface, implement
///  the function createNewAptXml and then create a object of that class in game/viewer.
///  Then use AptSetXMLImplementor() function to register newly created object with Apt.
///  This object pointer will be stored by Apt and will be used ahead when there is call to create a new XML object.

class IAptXmlImpl
{
  public:
    virtual ~IAptXmlImpl() {}

    /// Apt will call this function to generate new IAptXml type objects.
    virtual IAptXml *createNewAptXml(const char *url = NULL) = 0;

    /// custom new/delete operators that can be used on non-sim thread
    static void *operator new(size_t);
    static void operator delete(void *, size_t);
    static void *operator new[](size_t);
    static void operator delete[](void *);

};

/// This structure basically stores a pointer to attribute key and attribute value.
/// getFirstAttribute() and getNextAttribute() functions from IAptXmlNode, return
/// these type of objects.
class AptXmlAttributePair
{
  public:
    char *pKey;
    char *pValue;
    AptXmlAttributePair() : pKey(NULL), pValue(NULL) {};

    /// custom new/delete operators that can be used on non-sim thread
    static void *operator new(size_t);
    static void operator delete(void *, size_t);
    static void *operator new[](size_t);
    static void operator delete[](void *);

};

/// IAptXmlNode is the basic node definition in XML and all the nodes except the root of document will of this type.
/// IAptXml is derived from IAptXmlNode and it adds some more functions to IAptXmlNode which are specific to a XML
/// object and not in XmlNode object.
class IAptXmlNode
{
  public:
    /// Method - This should delete the IAptXmlNode object and any resources attached to it.
    virtual ~IAptXmlNode() {};

    /// Method - appends the specified child node to the XML object's child list.
    /// The appended child node is placed in the tree structure once removed from its existing parent node, if any.
    ///  Not in first release
    virtual void appendChild(IAptXmlNode *) = 0;

    /// This method will return the first attribute from the IAptXmlNode.
    virtual AptXmlAttributePair getFirstAttribute() = 0;

    /// This function will be called to enumerate thru all the attributes of the IAptXmlNode.
    /// It will return consecutive attributes in IAptXmlNode.
    /// Both pKey, and pValue will be NULL in case of end of Attributes.
    virtual AptXmlAttributePair getNextAttribute() = 0;

    /// This function will set the attribute pair.
    virtual void setAttribute(const char *pszKey, const char *pszValue) = 0;

    /// Collection (read-only); returns an array of the specified XML object's children.
    /// Each element in the array is a reference to an XML object that represents a child node.
    /// This is a read-only property and cannot be used to manipulate child nodes.
    /// Use the methods appendChild, insertBefore, and removeNode to manipulate child nodes.
    /// This collection is undefined for text nodes (nodeType == 3).
    /// There are 2 functions to enumerate thru this collection

    /// Returns pointer to First child node, otherwise NULL
    virtual IAptXmlNode *getFirstChildNode() = 0;

    /// Returns pointer to next child node , otherwise NULL
    /// This function will be called to enumerate thru all the child nodes of IAptXmlNode.
    virtual IAptXmlNode *getNextChildNode() = 0;

    /// Method - constructs and returns a new XML node of the same type, name, value, and attributes
    /// as the specified XML object. If deep is set to true, all child nodes are recursively cloned,
    /// resulting in an exact copy of the original object's document tree.
    ///  boolean relaced with int.
    virtual void cloneNode(int deep) = 0;

    /// Property (read-only) - evaluates the specified XML object and references the first child
    /// in the parent node's children list. This property is null if the node does not have children.
    /// This property is undefined if the node is a text node. This is a read-only property and cannot
    /// be used to manipulate child nodes; use the methods appendChild, insertBefore, and removeNode
    /// to manipulate child nodes.
    virtual IAptXmlNode *firstChild() = 0;

    /// Method - returns true if the specified XML object has child nodes; otherwise, returns false
    ///  boolean replaced with int.
    virtual int hasChildNodes() = 0;

    /// Method - inserts a new child node into the XML object's child list, before the beforeNode node.
    /// If the beforeNode parameter is undefined or null, the node is added using appendChild.
    /// If beforeNode is not a child of myXML, the insert fails.
    ///  Not implemeted in first version of XML implementation.
    virtual void insertBefore(IAptXmlNode *childNode, IAptXmlNode *beforeNode) = 0;

    /// Property (read-only)- evaluates the XML object and references the last child in the parent
    /// node's child list. This method returns null if the node does not have children. This is a
    /// read-only property and cannot be used to manipulate child nodes; use the methods appendChild,
    /// insertBefore, and removeNode to manipulate child nodes.
    virtual IAptXmlNode *lastChild() = 0;

    /// Property (read-only) - evaluates the XML object and references the next sibling in the parent
    /// node's child list. This method returns null if the node does not have a next sibling node.
    /// This is a read-only property and cannot be used to manipulate child nodes. Use the methods
    /// appendChild, insertBefore, and removeNode to manipulate child nodes.
    virtual IAptXmlNode *nextSibling() = 0;

    /// Property - returns the node name of the XML object. If the XML object is an XML element (nodeType == 1),
    /// nodeName is the name of the tag representing the node in the XML file. For example, TITLE is
    /// the nodeName of an HTML TITLE tag. If the XML object is a text node (nodeType == 3), the nodeName is null.
    virtual char *nodeName() = 0;

    /// Method - This function will set the nodeName property of IAptXmlNode.
    virtual void setNodeName(const char *name) = 0;

    /// Property (read-only) - returns a nodeType value, where 1 is an XML element and 3 is a text node
    virtual int nodeType() = 0;

    /// Property - returns the node value of the XML object. If the XML object is a text node, the nodeType
    /// is 3, and the nodeValue is the text of the node. If the XML object is an XML element (node type is 1),
    /// it has a null nodeValue and is read-only
    virtual char *nodeValue() = 0;

    /// Method — This function will set the value of IAptXmlNode.
    virtual void setNodeValue(const char *strValue) = 0;

    /// Property (read-only) - references the parent node of the specified XML object, or returns null if the
    /// node has no parent. This is a read-only property and cannot be used to manipulate child nodes; use the
    /// methods appendChild, insertBefore, and removeNode to manipulate children.
    virtual IAptXmlNode *parentNode() = 0;

    /// Property (read-only) - returns a reference to the previous sibling in the parent node's child list.
    /// Returns null if the node does not have a previous sibling node. This is a read-only property and cannot
    /// be used to manipulate child nodes; use the methods appendChild, insertBefore, and removeNode to manipulate
    /// child nodes.
    virtual IAptXmlNode *previousSibling() = 0;

    /// Method - removes the specified XML object from its parent. All descendents of the node are also deleted
    ///  Not implemeted in first version of XML implementation.
    virtual void removeNode() = 0;

    /// Method - evaluates the specified XML object, constructs a textual representation of the XML structure
    /// including the node, children, and attributes, and returns the result as a string.
    virtual char *toString() = 0;

}; // end of IAptXmlNode

class IAptXml : public IAptXmlNode
{
  public:
    /// Method — will remove the XML document, all its child nodes if any of them are present and any resources related to them.
    virtual ~IAptXml() {};

    /// Property - the MIME type that is sent to the server when you call the XML.send or XML.sendAndLoad method. The default is
    /// application/x-www-form-urlencoded
    virtual char *contentType() = 0;

    /// Method — This function will set the new contentType of IAptXml i.e root Document.
    ///  Not implemeted in first version of XML implementation in Apt
    virtual void setContentType(const char *strType) = 0;

    /// Method - creates a new XML element with the name specified in the parameter. The new element initially has no parent,
    /// no children, and no siblings. The method returns a reference to the newly created XML object representing the element.
    /// This method and createTextNode are the constructor methods for creating nodes for an XML object.
    ///  Not implemeted in first version of XML implementation of Apt
    virtual IAptXmlNode *createElement(const char *name) = 0;

    /// Method - creates a new XML text node with the specified text. The new node initially has no parent, and text nodes
    /// cannot have children or siblings. This method returns a reference to the XML object representing the new text node.
    /// This method and createElement are the constructor methods for creating nodes for an XML object
    ///  Not implemeted in first version of XML implementation.
    virtual IAptXmlNode *createTextNode(const char *text) = 0;

    /// Property - sets and returns information about the XML document DOCTYPE declaration.
    /// After the XML text has been parsed into an XML object, the XML.docTypeDecl property of the XML object
    /// is set to the text of the XML document's DOCTYPE declaration. For example, <!DOCTYPE greeting SYSTEM "hello.dtd">.
    /// This property is set using a string representation of the DOCTYPE declaration, not an XML node object.
    ///  Not implemeted in first version of XML implementation of Apt
    virtual char *docTypeDecl() = 0;

    // ActionScript's XML parser is not a validating parser. The DOCTYPE declaration is read by the parser and stored in the docTypeDecl property, but no DTD validation is performed.
    // If no DOCTYPE declaration was encountered during a parse operation, XML.docTypeDecl is set to undefined. XML.toString outputs the contents of XML.docTypeDecl immediately after the XML declaration stored in XML.xmlDecl, and before any other text in the XML object. If XML.docTypeDecl is undefined, no DOCTYPE declaration is output

    /// Method — This will set the docTypeDecl property of IAptXml object.
    ///  Not implemeted in first version of XML implementation of Apt
    virtual void setDocTypeDecl(const char *docTypeDecl) = 0;

    /// Method - returns the size, in bytes, of the XML document.
    virtual int getBytesTotal() = 0;

    /// Method - returns the number of bytes loaded (streamed) for the XML document. You can compare the value of
    /// getBytesLoaded with the value of getBytesTotal to determine what percentage of an XML document has loaded.
    virtual int getBytesLoaded() = 0;

    /// Property - Default setting is false. When set to true, text nodes that only contain white space are discarded
    /// during the parsing process. Text nodes with leading or trailing white space are unaffected.
    ///  boolean replaced with int
    virtual void setIgnoreWhite(int bIgnorWhite) = 0;

    /// Tells whether ignoreWhite is turned on or off.
    virtual int isIgnoreWhite() = 0;

    /// Method - loads an XML document from the specified URL, and replaces the contents of the specified XML object
    /// with the downloaded XML data. The load process is asynchronous; it does not finish immediately after the load
    /// method is executed. When load is executed, the XML object property loaded is set to false. When the XML data
    /// finishes downloading, the loaded property is set to true, and the onLoad method is invoked. The XML data is
    /// not parsed until it is completely downloaded. If the XML object previously contained any XML trees, they are discarded.
    virtual void load(const char *url) = 0;

    /// Property — returns if the load operation is completed and XML object is loaded or not.
    ///  boolean replaced with int.
    virtual int isLoaded() = 0;

    /// Method - parses the XML text specified in the source parameter, and populates the specified XML object with the
    /// resulting XML tree. Any existing trees in the XML object are discarded.
    virtual void parseXml(const char *sourcestring) = 0;

    /// Method; encodes the specified XML object into an XML document and sends it to the specified URL using the POST method.
    ///  Not implemeted in first version of XML implementation of Apt
    virtual void send(const char *url) = 0;

    /// Method; encodes the specified XML object into a XML document, sends it to the specified URL using the POST method,
    /// downloads the server's response and then loads it into the targetXMLobject specified in the parameters.
    /// The server response is loaded in the same manner used by the load method.
    ///  Not implemeted in first version of XML implementation of Apt
    virtual void sendAndLoad(const char *url, IAptXml *target) = 0;

    /// Property; automatically sets and returns a numeric value indicating whether an XML document was
    /// successfully parsed into an XML object
    virtual int status() = 0;

}; // end of IAptXml
