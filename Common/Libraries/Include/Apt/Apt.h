#pragma once

#include "AptVersion.h"

template <typename T>
class AptSharedPtr;
struct AptFile;
using AptFilePtr = AptSharedPtr<AptFile>;
class AptExtObject;
class AptCIH;
struct AptCharacter;

#include "AptStd/AptCXForm.h"
#include "AptStd/AptMatrix.h"
#include "AptStd/AptRect.h"
#include "AptDefine.h"
#include "AptString/EAString.h"
#include <stdarg.h>

// New file added to handle the Input key codes. Game teams can map the key codes as per their requirement
#include "AptInputKeyCode.h"

#include "AptStd/AptMath.h"

#ifndef APT_DEFAULT_MAX_QUEUED_FRAMES
#define APT_DEFAULT_MAX_QUEUED_FRAMES 16
#endif

enum AptInputState
{
    AptInputState_Pressed,
    AptInputState_Released,
    AptInputState_NumStates,
    AptInputState_WheelUp,   // F#563    1
    AptInputState_WheelDown, // F#563    2
    AptInputState_MouseMove
};
enum AptInputController
{
    AptInputController_Keyboard,
    AptInputController_Mouse,
    AptInputController_Gamepad0,
    AptInputController_Gamepad1,
    AptInputController_Gamepad2,
    AptInputController_Gamepad3,
    AptInputController_Gamepad4,
    AptInputController_Gamepad5,
    AptInputController_Gamepad6,
    AptInputController_Gamepad7,
    AptInputController_NumControllers,
};

enum AptMaskRenderOperation
{
    AptMaskRenderOperation_Subtract = -1,
    AptMaskRenderOperation_None     = 0,
    AptMaskRenderOperation_Add      = 1
};

/// These enumerated types will be used to pass in to AptUpdate() and AptRender() functions.
/// Apt will only tick the _levels that have been selected by combination of these
/// enumerated types. And Apt will only render the _levels that have been selected by combination
/// of these enumerated types. For eg. Call to AptRender(AptAnimLevel_0 |AptAnimLevel_3); will
/// only render movieclip inside _level0 and _level3.
/// Right now enumerated types are used through 32 bit integer as Apt only supports 24 levels so far.
enum AptAnimLevelE
{
    AptAnimLevel_NONE = 0x00000000,
    AptAnimLevel_0    = 0x00000001,
    AptAnimLevel_1    = 0x00000002,
    AptAnimLevel_2    = 0x00000004,
    AptAnimLevel_3    = 0x00000008,
    AptAnimLevel_4    = 0x00000010,
    AptAnimLevel_5    = 0x00000020,
    AptAnimLevel_6    = 0x00000040,
    AptAnimLevel_7    = 0x00000080,
    AptAnimLevel_8    = 0x00000100,
    AptAnimLevel_9    = 0x00000200,
    AptAnimLevel_10   = 0x00000400,
    AptAnimLevel_11   = 0x00000800,
    AptAnimLevel_12   = 0x00001000,
    AptAnimLevel_13   = 0x00002000,
    AptAnimLevel_14   = 0x00004000,
    AptAnimLevel_15   = 0x00008000,
    AptAnimLevel_16   = 0x00010000,
    AptAnimLevel_17   = 0x00020000,
    AptAnimLevel_18   = 0x00040000,
    AptAnimLevel_19   = 0x00080000,
    AptAnimLevel_20   = 0x00100000,
    AptAnimLevel_21   = 0x00200000,
    AptAnimLevel_22   = 0x00400000,
    AptAnimLevel_23   = 0x00800000,
    AptAnimLevel_24   = 0x01000000,
    AptAnimLevel_25   = 0x02000000,
    AptAnimLevel_26   = 0x04000000,
    AptAnimLevel_27   = 0x08000000,
    AptAnimLevel_28   = 0x10000000,
    AptAnimLevel_29   = 0x20000000,
    AptAnimLevel_30   = 0x40000000,
    AptAnimLevel_31   = 0x80000000,
    AptAnimLevel_ALL  = 0xffffffff
};

/// getBytesTotal and getBytesLoaded is called on 4 different types of objects.
/// MovieClip, Sound, XML, LoadVars Object.
/// Out of these XML case is handled by the XML interface exposed by Apt.
enum AptGetBytesEnum
{
    AptGetBytesEnum_MovieClip = 0,
    AptGetBytesEnum_Sound     = 1,
    AptGetBytesEnum_XML       = 2,
    AptGetBytesEnum_LoadVars  = 3,
};

// Forward Declarations
struct AptSavedInputRecord;
class AptValue;
class AptTarget;

using AptAssetString = void *;
#if defined(APT_USE_SOUND_OBJECT)
using AptAssetSound = void *;
#endif
using AptAssetRenderingUnit = void *;
using AptAssetTexture       = void *;
using AptAnimationUserData  = void *;
using AptAssetMoiveClip     = void *;
#if defined(APT_CUSTOM_CONTROL_USE_ZID)
using AptAssetCustomControlZId = void *;
#endif
using AptTargetInstance = void *;
using AptAssetEffect    = void *;

/// Opaque handle to an Apt library instance. Created by AptLibraryInitialize()
/// and passed to every lifecycle call; its definition is internal to Apt.
/// Only one library may be live at a time.
struct AptLibrary;
using AptLibraryHandle = AptLibrary *;

/// Apt TextField
/// AptAllocateStringParameters structure is passed to pfnAllocateString callback. Various flags in this structure are used to propagate text
/// field changes to the auxiliary string callbacks. With these flags, the auxiliary callbacks can determine
/// what needs to be done for their own implementation. Apt makes appropriate deallocations and allocations before a render call of a
/// text field object.  The only time Apt will call pfnAllocateString from AptUpdate before a render call is when
/// certain properties are queried before the render that would require up-to-date text field information. These particular text field properties
/// are �_textWidth� and �_textHeight.�
/// If no text field properties are queried/changed then in non-decoupled mode, if required, Apt will call pfnAllocateString just before rendering it.
/// In decoupled rendering mode, Apt will always call pfnAllocateString from simulation/AptUpodate side.
/// In decoupled rendering mode, pfnDeallocateString could be called from either side Render or Simulation.
/// In decoupled rendering mode, auxiliary library should take care of allocation/deallocation of font textures (if required) from correct
/// thread. These changes made were done to save the overall string allocation and deallocation callbacks made to the auxiliary libraries.

/// This enumerated type defines values for various types of text alignments that can be used when
/// pfnAllocateString callback is called.
enum AptStringAlignment
{
    AptStringAlignment_Left    = 0,
    AptStringAlignment_Right   = 1,
    AptStringAlignment_Center  = 2,
    AptStringAlignment_None    = 3,
    AptStringAlignment_Justify = 4,
};

/// Following are the flags that are passed to auxiliary library with pfnAllocateString callback in AptAllocateStringParameters. These flags indicate
/// what changes were made to text field. Depending on those flags, auxiliary library can decide how to implement the callback.
using TextFieldChangeE = enum {
    APT_TEXTFIELD_NONE        = 0x00001, // No Change
    APT_TEXTFIELD_FUPDATE     = 0x00002, // Force update
    APT_TEXTFIELD_DIRTY       = 0x00004, // Dirty flag
    APT_TEXTFIELD_AUTOSIZE    = 0x00008, // autoSize flag set
    APT_TEXTFIELD_ASCHANGE    = 0x00010, // autoSize flag changed
    APT_TEXTFIELD_BACKGROUND  = 0x00020, // Background color flag set
    APT_TEXTFIELD_BGCOLOR     = 0x00040, // Background color changed
    APT_TEXTFIELD_BORDER      = 0x00080, // Border flag set
    APT_TEXTFIELD_BORDERCOLOR = 0x00100, // Border color flag set
    APT_TEXTFIELD_TEXTCHANGE  = 0x00200, // Text in field changed
    APT_TEXTFIELD_TEXTCOLOR   = 0x00400, // Text color changed
    APT_TEXTFIELD_TEXTWIDTH   = 0x00800, // _textWidth property changed
    APT_TEXTFIELD_WORDWRAP    = 0x01000, // WordWrap flag set
    APT_TEXTFIELD_HEIGHT      = 0x02000, // _height changed
    APT_TEXTFIELD_WIDTH       = 0x04000, // _width changed
    APT_TEXTFIELD_FONTCHANGE  = 0x08000, // Font name changed
    APT_TEXTFIELD_FONTSIZE    = 0x10000, // Font size changed
    APT_TEXTFIELD_ALIGNCHANGE = 0x20000, // Font alignment changed
    APT_TEXTFIELD_ALL         = 0xfffff, // All is changed
};

// Adding enum for Font Style

/// AptFontStyle gets passed in AptAllocateStringParameters structure to pfnAllocateString callback.
/// This enumerated type indicates the font style for the text field to be allocated. Auxiliary library can decide
/// which of the font styles it can implement.
using AptFontStyle = enum {
    AptFontStyle_Normal         = 0x00000000,
    AptFontStyle_Bold           = 0x00000001,
    AptFontStyle_Italic         = 0x00000010,
    AptFontStyle_Underline      = 0x00000100,
    AptFontStyle_StrikeThrough  = 0x00001000,
    AptFontStyle_isBoldSet      = 0x00010000,
    AptFontStyle_isItalicSet    = 0x00100000,
    AptFontStyle_isUnderlineSet = 0x01000000,
    AptFontStyle_None           = 0x00000002,
};

enum
{
    AptTextFormatLeading_Undefined  = 0x7FFFFF, // Passed as AptAllocateStringParameters.nLeading when TextFormat.leading is undefined
    AptTextFormatTracking_Undefined = 0x7FFFFF  // Passed as AptAllocateStringParameters.nTracking when TextFormat.tracking is undefined
};

/// This is the structure that gets passed to pfnAllocateString callback.
/// This structure has all the information required for allocating and rendering text field.
/// In/Out words at the beginning of the comment for members below indicate if those are in or out parameters.
/// Out parameters are the ones that auxiliary library should fill before returning from pfnAllocateString callback.
struct AptAllocateStringParameters
{
    /// In - This is name of the font used to display the text.
    const char *szFontName;

    /// In - Out - When passed in x0 and y0 indicate the left-top positions of the text to be rendered.
    /// Auxiliary library should pass back the new actual text field size in these fields fields if change via the autoSize flag.
    float x0;
    /// In - Out - When passed in x0 and y0 indicate the left-top positions of the text to be rendered.
    /// Auxiliary library should pass back the new actual text field size in these fields fields if change via the autoSize flag.
    float y0;
    /// In - Out - When passed in x1 and y1 are right bottom positions of the text to be rendered.
    /// Auxiliary library should pass back the new actual text field size in these fields fields if change via the autoSize flag.
    float x1;
    /// In - Out - When passed in x1 and y1 are right bottom positions of the text to be rendered.
    /// Auxiliary library should pass back the new actual text field size in these fields fields if change via the autoSize flag.
    float y1;

    /// In - This indicates the alignment for the text to be displayed
    AptStringAlignment eAlignment;
    /// In - This flag reflects the auto-alignment flag in Flash text field.
    AptStringAlignment eBoxAlignment;

    /// In - This reflects the MultiLine line flag in Flash.
    int bMultiline;
    /// In - This reflects the Word wrap flag in Flash.
    int bWordWrap;
    /// In - This is the color of the text to be displayed.
    unsigned int nColour;
    /// In - This is background color for the text to be displayed.
    unsigned int nBackColor;
    /// In - This is the border color for the text to be displayed.
    unsigned int nBorderColor;
    /// In - Flag that indicates whether text has a background or not. If true then nBackColor should be used to set it.
    int bBackground;
    /// In - Flag that indicates whether text has a border around. If true then nBorderColor should be used to draw it.
    int bBorder;

    /// In - This float is the height of the font.
    float fFontHeight;

    /// In - This is the scroll property for text field in Flash. It defines the vertical position of text in a text field
    uint32_t nLineOffset;

    /// Out - not used anywhere right now. It will be used to later ti return back num lines used.
    int *pnNumLines;

    /// In - This is the actual string to be displayed. By convention, if szString starts with a dollar sign "\$",
    /// the remainder of the string is a text id to be localized. Otherwise, the text should just be treated as a string
    /// directly.
    const char *szString;

    /// In - This flag is a combination of TextFieldChangeE enumerated type.
    unsigned int eFlags;

    /// In - This flag is a combination of AptFontStyle enumerated type.
    unsigned int nFontStyle;
    /// In - This is a property from TextFormat object. It indicates the indentation from the left margin to the first character in the paragraph.
    int nIndent;
    /// In - This is a property from TextFormat object. It indicates how much left margin in pixels has to be used.
    int nLeftMargin;
    /// In - This is a property from TextFormat object. It indicates how much right margin in pixels has to be used.
    int nRightMargin;
    /// In - This is a property from TextFormat object. It indicates how much leading space between lines in pixels has to be used.
    int nLeading;
    /// In - Indicates how much space between characters there will be. Note, the property TextFormat.tracking does not exist in Flash, only Apt.
    int nTracking;

    /// Out - Auxiliary library should return back the actual text width in this member. (not texture width)
    float fTextWidth;
    /// Out - Auxiliary library should return back the actual text height in this member. (not texture height)
    float fTextHeight;
    /// Out - It should be filled with length of string in pixels.
    float fStrLen;
    /// Out - This is an out parameter and it should be filled with the number of lines text takes in given boundary box.
    uint32_t nMaxScroll;

    /// In-Out
    /// Out - This is the zid that pfnAllocateString returns back to Apt. Apt further uses it for pfnDrawString, pfnDeallocateString or pfnAllocateString.
    /// In - If zid was already allocated in previous pfnAllocateString callback then Apt might pass in the same zid to auxiliary
    /// library for further changes to the same text field.
    AptAssetString pCurrString;
};

struct AptSysClock
{
    int Second;
    int Minute;
    int Hour;
    int Day;
    int Date;
    int Month;
    int Year;
    int Hundredths;
};

struct AptAnalogStickInfo
{
    // one of the x or y values could be zero.
    /// Denotes the x-axis change for the analog stick
    float fXAxisValue;
    /// Denotes the y-axis change for the analog stick
    float fYAxisValue;
    /// Denotes the zero-based controller number
    unsigned char nController;
    /// Denotes for which analog stick this event is occurring 0 - Left and 1 - Right.
    AptInputType nSide;
};

struct AptGestureInfo
{
    // data generated from the gesture
    float fGestureData;

    /// Denotes which gesture is occurring.
    AptInputType nGesture;

    /// Denotes the zero-based controller number
    unsigned char nController;
};

struct AptMovieclipInformation
{
    int nAnimations{0};
    int nMovieClips{0};
    int nButtons{0};
    int nStaticText{0};
    int nDynamicText{0};
    int nMorph{0};
    int nShapes{0};
    int nCustomControls{0};

    AptMovieclipInformation()

    {
    }

    void Reset();
};

// $senzee-lockless, for lockless Apt and uifx
struct AptRenderInfo
{
    inline AptRenderInfo() : mTransform(),
                             mnLevel(0),
                             mnMaskDepth(0),
                             meMaskOper(AptMaskRenderOperation_None),
                             mpEffect(NULL)
    {
    }

    inline AptRenderInfo(const ::AptMath::ClipTransformT &transform, unsigned nLevel, unsigned nMaskDepth, AptMaskRenderOperation eMaskOp) : mTransform(transform), mnLevel(nLevel), mnMaskDepth(nMaskDepth), meMaskOper(eMaskOp), mpEffect(NULL)
    {
    }

    ::AptMath::ClipTransformT mTransform;
    unsigned mnLevel;
    unsigned mnMaskDepth;
    AptMaskRenderOperation meMaskOper;
    AptAssetEffect mpEffect; // $senzee-lockless/uifx, for (very near) future
};

// this data is only specific to this particular frame and AptAux should not assume that the data will stay for next frame onwards
struct AptEffectData
{
    uint32_t mnBlendMode{0};
    // Apt aux is supposed to only query the values inside filter array and store them off in its own data structures
    // and do not point to actual AptValues as those can be deleted/changed/re-used by Sim thread for next frame.
    AptValue *pFiltersArray;
    // we can add more members here if needed.

    inline AptEffectData() : pFiltersArray(NULL) {}
};

struct AptUserFunctions
{

    /// \name General Functions
    //@{

#if defined(APT_ALLOCATION_TRACKING)
    /// Allocate memory block.
    /// @param nSize is the size in bytes of the block to be allocated.
    void *(*pfnMemAlloc)(size_t nSize, const char *szName, int nLine);
    void *(*pfnTempAlloc)(size_t nSize, const char *szName, int nLine);
    void *(*pfnRenderListAlloc)(size_t nSize, const char *szName, int nLine);
#else
    /// Allocate memory block.
    /// @param nSize is the size in bytes of the block to be allocated.
    void *(*pfnMemAlloc)(size_t nSize);
    void *(*pfnTempAlloc)(size_t nSize);
    void *(*pfnRenderListAlloc)(size_t nSize);
#endif

    /// Free memory block.
    /// @param pData is a pointer that was previously returned from pfnMemAlloc().
    void (*pfnMemFree)(void *pData);
    void (*pfnTempFree)(void *pData);
    void (*pfnRenderListFree)(void *pData);

    /// Free memory block. @param pData is a pointer that was previously returned from pfnMemAlloc().
    /// @param nSize is a hint which can be used to optimize small object allocation/deallocation, and
    /// is the size of memory block which is pointed to by pData.
    void (*pfnMemFreeSize)(void *pData, size_t nSize);
    void (*pfnTempFreeSize)(void *pData, size_t nSize);
    void (*pfnRenderListFreeSize)(void *pData, size_t nSize);

    /// Called when an assertion has failed. This function should simply alert the programmer to this fact.
    /// This gets called from APT_ASSERT
    void (*pfnAssertFail)(
        const char *szExpression,
        const char *szFile,
        unsigned int nLine);

    // Added in 2.07.01
    /// Called when an assertion has failed. This function should simply alert the programmer to this fact.
    /// This gets called from APT_ASSERTM
    void (*pfnAssertFailMsg)(
        const char *szExpression,
        const char *szMsg,
        const char *szFile,
        unsigned int nLine);

    /// Sets the background/clear color for rendering.
    void (*pfnSetBackgroundColour)(unsigned int nColour);

    /// Printf-style function used for diagnostic output from the Apt library, as well
    /// as output of 'trace' calls from Flash ActionScript code. Trace output will be
    /// prefixed as such.
    void (*pfnDebugPrint)(const char *szFormat, ...);


    /// Only required if saved inputs are enabled. @param pRecord is a
    /// variable length structure of size nSize. When this function is called
    /// nSize bytes should be saved to the saved input stream.
    void (*pfnDebugAddSavedInput)(AptSavedInputRecord *pRecord, int nSize);

    /// Only required if screen grabs are being done with saved inputs. This function
    /// will be called with a string representation of the frame so that the client
    /// knows that the next frame that is rendered should be captured for saved inputs
    /// (i.e. the frame has advanced)
    void (*pfnDebugSetScreenGrabPending)(const char *szFrame);

    /// Loads and frees animations. @param szBaseName is the base file name for the
    /// animation (normally the filename of the .SWF file, without the .SWF part).
    /// @param AptAsyncLoadContext is a handle that you must store to give back to Apt
    /// when you have completed the (possibly asynchronous) load of the named animation.
    /// When the load is finished, call AptCompleteAnimationAsyncLoad.
    /// See the documentation for AptLoadAnimation() for more information on the load
    /// process.
    void (*pfnLoadAnimation)(
        const char *szBaseName,
        AptFilePtr pAsyncLoadContext);
    void (*pfnFreeAnimation)(void *pUserHandle);
    void (*pfnFreeConstantTable)(void *pConstTable);

    // Added this extra function in 0.15.00 for FR 380
    /// This function is called by Apt when the animation (including all its assets, imported libraries)
    /// is completely loaded by Apt and is ready to play the animation.
    /// @param szBaseName is a char * to name of the animation and @param szTargetName is the name of
    /// target in which animation was loaded. szTargetName can be pointing to a string with zero length.
    /// This function will be called only for animations/swfs loaded by AptLoadAnimation from code
    /// or loadMovie functions from actionscript.
    void (*pfnLoadAnimationCompleted)(const char *szBaseName, const char *szTargetName);

    /// Loads and frees Images
    /// Takes the fileName, which includes the image's extension.
    /// asyncLoadContext is a handle that you must store to give back to Apt when you have
    /// completed the (possibly asynchronous) load of the named animation.
    /// When the load is finished, call AptCompleteImageAsyncLoad.
    void (*pfnLoadImage)(const char *fileName, AptFilePtr asyncLoadContext);
    void (*pfnFreeImage)(void *userData);

    /// Renders a texture file loaded via pfnLoadImage
    void (*pfnDrawImage)(AptAssetTexture string, const AptRenderInfo *renderInfo);

    //@}

    /// \name Data Access Functions
    //@{

    /// This function is called when the fscommands() function from the Flash
    /// authoring environment is used. The parameters are identical to what is specified
    /// by the ActionScript.
    void (*pfnCommand)(const char *szCommand, const char *szParams);

    /// The function is called when the animation executes a loadVariables
    /// command. The format of the returned string is typical URL variable encoding, so
    /// that the same code and data work in the authoring environment, viewer, and game
    /// (see 'URL format' below). The string that is returned by this callback function
    /// will be deallocated by the Apt library, and so must be allocated using the
    /// memory allocation functions above. @param szCommand is the URL or the text file name
    /// from which variables are to be loaded.
    AptValue *(*pfnLoadVariables)(const char *szCommand);

    /// The function is called when the animation executes a LoadVars.sendAndLoad
    /// command. The format of the returned string is typical URL variable encoding, so
    /// that the same code and data work in the authoring environment, viewer, and game.
    /// The string that is returned by this callback function
    /// will be deallocated by the Apt library, and so must be allocated using the
    /// memory allocation functions above. There is no input parameter because the
    /// call to this function is preceded by call to pfnSendVariables. This happens in case of
    /// LoadVars.sendAndLoad function. Apt calls pfnSendVariables with variables in URL-encoded
    /// format and then immediately calls pfnLoadVariablesNULL to get the response back from auxiliary
    /// library.
    AptValue *(*pfnLoadVariablesNULL)();

    /// Apt allows for a simple extension of Flash dot syntax to get/set values from/to
    /// the host application. In script code, the extern object is a special
    /// reference that will cause these functions get called. For example, ActionScript
    /// such as:
    /// @code
    /// extern.options\_volume = 7;
    /// trace(extern.options\_volume);
    /// @endcode
    /// Would first call @code pfnSetExternVariable("options\_volume", "7") @endcode
    /// The host application should validate, convert the string value to the correct type, and
    /// store the value under the given name. When the variable is read in the second
    /// line, @code pfnGetExternVariable("options\_volume") @endcode will be called and should
    /// return an up-to-date value for the specified variable. The string returned by
    /// pfnGetExternVariable() is deallocated by the Apt library, and so must be
    /// allocated using the memory allocation functions above.
    void (*pfnSetExternVariable)(const char *szVar, const char *szValue);
    AptValue *(*pfnGetExternVariable)(const char *szVar);

    /// The function is called when the actionscript in animation executes a LoadVars.send
    /// command. The format of the passed string is typical URL variable encoding, so
    /// that the same code and data work in the authoring environment, viewer, and game
    /// (see 'URL format' below). @param szUrl is the a pointer to a string specifying the
    /// absolute or relative location of the external script or application that receives the variables.
    /// In most cases this szUrl can be a game specific module or game itself. @param szTarget is a pointer to
    /// string specifying the name of the browser window or frame into which to load results.
    /// Can be a custom name or one of the four presets: "_blank", "_parent", "_self", or "_top". Right now
    /// this parameter will be passed to Auxiliary library but the return result will be ignored.
    /// @param szMethod is a pointer to a optional string specifying the HTTP method by which to
    /// send the properties of loadVarsObject to an external module. @param szProp is a pointer to a
    /// string in which all the properties of the loadVarsObject are collected together in URL-encoded
    /// format. @param bIsSendAndLoad LoadVars.sendAndLoad uses this flag to denote that
    /// there will be call to pfnLoadVariablesNULL callback function immediately followed after this call.
    /// Auxiliary library can make a note of that to send response back to Apt in next call to
    /// pfnLoadVariablesNULL.
    void (*pfnSendVariables)(const char *szUrl, const char *szTarget, const char *szMethod, const char *szProp, int bIsSendAndLoad);

    //@}

    /// \name String Functions
    //@{

    /// Allocates a string to be later rendered. In non-decoupled mode, allocate will be called only when the string contents
    /// change, so it's reasonable to do semi-expensive operations (re-flow text, allocate meshes, etc.) in this
    /// call. Apt passes AptAllocateStringParameters structure filled with all the information required to allocate and draw text.
    AptAssetString (*pfnAllocateString)(AptAllocateStringParameters *pParameters);

    /// Deallocates a string that was previously allocated with AllocateString.
    /// @param flags parameter helps to determine when to actually delete
    /// the texture or resources associated with string.
    /// @param asset is the AptAssetString or the zID that was returned by pfnAllocateString
    ///
    ///  When Apt calls the pfnAllocateString callback, it passes in the old AptAssetString object in
    ///  the AptAllocateStringParameters struct.  The previous pfnAllocateString implementation always
    ///  returned a new AptAssetString object back to Apt since the original AptAssetString would have
    ///  been deallocated before this call.  The original AptAssetString is now passed into the
    ///  pfnAllocateString callback so updates carry over to the original AptAssetString.
    ///
    /// In decoupled mode, this callback can be called from either update/simulation side or render side.
    void (*pfnDeallocateString)(AptAssetString asset, unsigned int flags);

    /// Renders a string allocated by pfnAllocateString. Alignment, coloring, etc. are generally performed in
    /// the pfnAllocateString to allow for caching in the rendering of the text.
    /// @param eMaskOperation parameter specifies the masking operation to be done on this string. (Can be
    ///  ignored if auxiliary library don't want to participate in masking.
    /// @param nMaskDepth parameter specifies the mask level to draw this object (can be ignored if
    /// nested masks are not supported)
    void (*pfnDrawString)(AptAssetString string, const AptRenderInfo *pRenderInfo);

    //@}

    /// \name Sound Functions
    //@{

    /// Load/free sounds. @param nID is the sound name (the same index that was
    /// used to name the sound when it was exported out of swfc.
    /// @param userData is the user data pointer that identifies the animation
    /// in which the sound resides.
    /// @param szName is the linkage name for the sound, or NULL if the sound
    /// does not have a linkage name.
    // added extra parameter in 0.16.00
#if defined(APT_USE_SOUND_OBJECT)
    AptAssetSound (*pfnLoadSound)(AptAnimationUserData userData, int nID, const char *szName);
    // AptAssetSound (*pfnLoadSound)(AptAnimationUserData userData, int nID);
    void (*pfnFreeSound)(AptAssetSound sound);

    /// Starts the given sound asset.
    /// szName is the exported linkage name of Sound. This is valid only when
    /// pfnStartSound() is called because of call from actionscript (Sound.start()) only after
    /// doing Sound.attachSound(). In all other cases szName will be NULL.
    void (*pfnStartSound)(AptAssetSound sound, const char *szName);
    // void (*pfnStartSound)(AptAssetSound sound);

    /// Starts the given sound stream. Sound streams are not loaded/freed
    /// because they are assumed to be large and are also not shareable across
    /// animations in Flash.
    void (*pfnStartSoundStream)(
        AptAnimationUserData userData,
        int nID);
#endif

    //@}

    /// \name Texture Functions
    //@{

    /// Load/free textures. @param nID is the texture name (the same index that
    /// was used to name the texture when it was exported out of swfc. Note that
    /// this function may be called on any thread, notably the Simulation thread,
    /// so handle those cases accordingly.
    /// @param userData is the user data pointer that identifies the animation
    /// which the texture resides.
    AptAssetTexture (*pfnLoadTexture)(
        AptAnimationUserData userData,
        int nID);
    void (*pfnFreeTexture)(AptAssetTexture texture);

    /// Attach the texture asset to the animation identified by userData,
    /// and texture id nID. This means that any rendering units in
    /// the given animation that use the texture nID should now use this texture.
    /// This function is used when a texture is imported from another (asset library)
    /// animation, but for consistency, it is also called for all textures even if
    /// they're being bound to their own animation.
    void (*pfnBindTexture)(
        AptAnimationUserData userData,
        int nID,
        AptAssetTexture texture);

    //@}

    /// \name Rendering Unit Functions
    //@{

    /// Load/free rendering units. @param nID is the texture name (the same index that
    /// was used to name the rendering unit when it was exported out of swfc.
    /// @param userData is the user data pointer that identifies the animation
    /// which the texture resides.
    AptAssetRenderingUnit (*pfnLoadRenderingUnit)(
        AptAnimationUserData userData,
        int nID);

    void (*pfnFreeRenderingUnit)(
        AptAssetRenderingUnit renderingUnit);

    /// Sets the current vertex matrix. All subsequent rendering unit, font
    /// draw calls, and custom control renders should have their vertex positions
    /// affected by this matrix.
    /// $NOTE this callback is now deprecated in favor of AptRenderInfo
#if !defined(APT_3D)
    void (*pfnSetVertexMatrix)(AptMatrix *pMatrix);
#else
    void (*pfnSetVertexMatrix)(AptMath::Mat44T *pMatrix);
#endif

    /// Sets the current color transform. All subsequent rendering unit,
    /// font draw calls, and custom control renders should have their colours
    /// scaled and translated as specified by this transform.  You must now use
    /// CopyToUint32 or CopyToFloatArray4 to copy the color info from AptCXForm.
    /// $NOTE this callback is now deprecated in favor of AptRenderInfo
    void (*pfnSetColourTransform)(AptCXForm *pCXForm);

    /// Draws the given rendering unit. If eMaskOperation is AptMaskRenderOperation/_None,
    /// the rendering unit is drawn normally. If eMaskOperation is AptMaskRenderOperation/_Add,
    /// the rendering unit should only be rendered so as to allow subsequent
    /// things to show through on that part of the screen. If
    /// eMaskOperation is AptMaskRenderOperation/_Subtract, the rendering unit should be
    /// subtracted from the mask (so that things don't show up there). However, if the there's no mask
    /// being used (i.e. DrawRenderingUnit has been called with Subtract the same number of times
    /// as it's been called with Add, then no masking should be performed (sorry, blame Flash).
    /// @param eMaskOperation parameter specifies the masking operation to be done on this string. (Can be
    ///  ignored if you don't want to participate in masking.
    /// @param nMaskDepth parameter specifies the mask level to draw this object (can be ignored if
    ///  nested masks are not supported)
    void (*pfnDrawRenderingUnit)(AptAssetRenderingUnit renderingUnit, const AptRenderInfo *pRenderInfo);

    /// Render a custom control. @param szType is the value of /_type as specified in
    /// component definition dialog in Flash. @param szTarget is the full slash-style path to the object
    /// (if it was given an instance name), and @param renderingUnit is the shape associated with the
    /// custom control (normally just a placeholder rectangle shape so that the bounding rectangle is available).
    /// The custom control should be rendered with the current vertex matrix and colour transforms applied to it.
    /// @param szCustomProperties is a pointer to a string which has all the properties that start with prefix '_'
    /// (except '_type' property which is always there for a custom control). This string is in
    /// URL-encoded format. Any other component properties that do not have '_ prefix will not be passed back
    /// along with the callback function. This is done so that implementer will not have to keep on parsing each and every
    /// component property that comes as part of szCustomProperties. User can just prefix only those properties with '_'
    /// which are required to be handled in callback.
    /// @param eMaskOperation parameter specifies the masking operation to be done on this string. (Can be
    ///  ignored if you don't want to participate in masking.
    /// @param nMaskDepth parameter specifies the mask level to draw this object (can be ignored if
    //// nested masks are not supported)
    void (*pfnCustomControlRender)(
        char *szType,
        char *szTarget,
        AptAssetRenderingUnit renderingUnit,
        const char *szCustomProperties, const AptRenderInfo *pRenderInfo);

    /// Before rendering a custom control object, Apt will issue this callback to determine whether or not the use
    /// wants to parse the components properties.  Apt will parse the components properties if true is passed back,
    /// or if the functions pointer is NULL.
    bool (*pfnCustomControlUpdate)(AptAssetRenderingUnit renderingUnit);

#if defined(APT_CUSTOM_CONTROL_USE_ZID)
    /// These new callbacks are added for custom control by which Apt can avoid converting movieclip properties into url-encoded string
    /// for every update. These callbacks are optional and Apt will work by default in older mode of generating url-encoded
    /// string of all properties starting with '_' and passing them every time to customcontrolrender callback. But if new callbacks are
    /// used then Apt will not url-encode properties and aux library can access only required properties to improve speed.

    /// This callback will be called for every AptUpdate. Aux library can create an object associated with that
    /// custom control and then return the object pointer as AptAssetCustomControlZId which is a void pointer. If there are no changes and
    /// parameter force == false, then aux library can return NULL to signify it does not need a new zid.
    /// Aux library can use various AptValueHelper functions on the customControlMovie to access properties of the movieclip.
    AptAssetCustomControlZId (*pfnCreateCustomControlZid)(AptAssetRenderingUnit renderingUnit,
                                                          const char *szTarget,
                                                          AptValue *customControlMovie,
#if APT_VERSION_AT_LEAST(3, 1, 2, 1)
                                                          AptAssetCustomControlZId *ppPreviousZid,
#else
                                                          AptAssetCustomControlZId pPreviousZid,
#endif
                                                          bool bForce);

    /// This will be called when the custom control gets destroyed and no longer displayed on screen.
    void (*pfnDestroyCustomControlZid)(AptAssetCustomControlZId zID);

    /// This will be called at the render time similar to older pfnCustomControlRender callback. But this callback will only have
    /// zID passed as aux library already has processed the zID and should have information to render this custom control.
    void (*pfnCustomControlRenderWithZid)(AptAssetCustomControlZId zID,
                                          AptAssetRenderingUnit renderingUnit, const AptRenderInfo *pRenderInfo);

#endif

    // DT - [AT] Investigate problems in custom control render object and masking
    //
    // Problem: optimizing render calls by batching causes custom controls (rendered immediately) to be rendered out of order
    //          according to their hierarchical placement in the apt render tree
    //
    // Solution: before rendering to a custom control cause the render manager to flush the accumulated batch
    //
    // This will be called every time just before the pfnCustomControlRender or pfnCustomControlRenderWithZid are called to dispatch
    // the render batch
    void (*pfnRenderDispatch)(const AptRenderInfo *pRenderInfo, bool bCustomRender);

    /// This will be called to inform of the imminent unload of an AptCIH.  This is useful for aux components which hold on
    /// to references to AptValue(s)/AptObject(s) so that they can unbind when an animation is forcibly unloaded.  This is being
    /// added now to support stable movie player custom controls.
    void (*pfnOnUnload)(AptValue *pValue);

    int (*pfnPointHitTest)(
        float fXPos,
        float fYPos,
        AptAssetMoiveClip pCurrent);

    /// Returns Real Time Clock. @param pClock is of type AptSysClock defined in Apt.h
    ///  This callback function should fill all the members of this structure. @param bLocal is
    ///  boolean that determines which time to be sent back, local or Universal time.
    void (*pfnGetRealTimeClock)(
        AptSysClock *pClock,
        bool bLocal);

    /// Returns total number of bytes in File or object. This function gets called when
    /// getBytesTotal command gets executed in actionscript for following objects - MovieClip,
    /// Sound, XML, LoadVars. Out of these XML.getBytesTotal should be handled by the XML interface
    /// implementations, so this function will not be executed with XML option. @param eGetBytes is the enumerated type
    /// that tells which type of object has called this function. @param szFileName is the name of file
    /// on which the last operation of Load was performed. This might be sometimes NULL if the actual string is
    /// not available to Apt.
    int (*pfnGetBytesTotal)(
        const char *szFileName,
        AptGetBytesEnum eGetBytes);

    /// Returns total number of bytes loaded out of total bytes from a file or object. This function gets called when
    /// getBytesLoaded command gets executed in actionscript for following objects - MovieClip,
    /// Sound, XML, LoadVars. Out of these XML.getBytesLoaded should be handled by the XML interface
    /// implementations, so this function will not be executed with XML option. @param eGetBytes is the enumerated type
    /// that tells which type of object has called this function. @param szFileName is the name of file
    /// on which the last operation of Load was performed. This might be sometimes NULL if the actual string is
    /// not available to Apt.
    int (*pfnGetBytesLoaded)(
        const char *szFileName,
        AptGetBytesEnum eGetBytes);

    // Added to support analog stick support feature request

    /// Returns the information about last analog stick event that was inserted in the AptInputQueue.
    /// Basically, game/auxiliary library will add AptInputType_LeftAnalogStick or AptInputType_RightAnalogStick
    /// to AptInputQueue with help of AptAddToInputQueue and then actionscript should check one of these codes through key.getCode().
    /// Once actionscript finds out that it is a analog stick event then it should call
    /// Key.getAnalogStickInfo() to get more information about the analog stick data.
    /// It is game/auxiliary library responsibility to maintain the information about the last analog stick
    /// event that was added to AptInputQueue. Key.getAnalogStickInfo() is implemented in AptKey class in Apt core library
    /// and gives call to pfnGetAnalogStickInfo callback function.
    /// Key.getAnalogStickInfo() creates a new AptObject, and fills it with fields of AptAnalogStickInfo structure
    /// returned by this callback function.
    /// Following is example of actionscript that can be used to get analog stick information.
    /// @verbatim
    /// myMovieclip.onKeyDown = function()
    /// {
    ///    nkey = Key.getCode() ;
    ///    if (nKey == 501)     // AptInputType_LeftAnalogStick
    ///    {
    ///         stickInfo = Key.getAnalogStickInfo() ;
    ///         trace("controller - " + stickInfo.controller) ;
    ///         trace("fXAxisValue - " + stickInfo.fXAxisValue) ;
    ///         trace("fYAxisValue - " + stickInfo.fYAxisValue) ;
    ///    }
    /// }
    /// // controller, fXAxisValue, fYAxisValue are pre-defined properties of Object returned by Key.getAnalogStickInfo()
    /// @endverbatim
    /// @param nController is the zero based controller number for which analog stick information should be
    /// returned.
    // AptAnalogStickInfo  (*pfnGetAnalogStickInfo)(int nController) ; This is no longer needed, all analog data is maintained in Apt.

    /// This callback gets called from AptActionInterpreter::getVariable() when a variable is searched in all scopes but
    /// it is not found and so undefinedValue is returned. So this could be a possible uninitialized variable.
    /// This callback may get called many times as in actionscript there can be many occasions of accessing an uninitialized
    /// variable.Implementation of this callback is optional.
    /// @param pszVarName is const pointer to the variable name that is being accessed.
    void (*pfnUninitializedVarAccess)(const char *pszVarName);

    /// This callback is for the Flash Stage object.  When Stage.height is called, we need to ask the aux libs what the width of the viewport was set to.
    float (*pfnGetStageHeight)();
    /// This callback is for the Flash Stage object.  When Stage.width is called, we need to ask the aux libs what the width of the viewport was set to.
    float (*pfnGetStageWidth)();

    /// Used to handle saved inputs generated by a call to AptAddCustomSavedInput. These can be anything the game team
    /// wants to add that is not controlled by apt, but must occur in order for apt to accurately reproduce a given run.
    void (*pfnCustomSavedInputHandler)(void *pBuffer, unsigned int nBufferSize);

    /// Used to give signal that playback of saved inputs is done.
    /// @param bSuccess = true means successfully done
    void (*pfnPlaySavedInputsDone)(bool bSuccess, const char *pszFailCause);

    /// This callback allows Apt users to handle zombie messages as they occur in Apt
    /// @param bZombieVectorFull = flag that shows if the zombie vector is full
    /// @param bRemoving = flag that tells if the zombie is being created (false) or being removed (true)
    /// @param pszMovieclipName = name of the zombie movieclip
    /// @param pszFileName = name of the zombie swf file
    void (*pfnHandleZombieState)(bool bZombieVectorFull, bool bRemoving, const char *pszMovieclipName, const char *pszFileName);

#if defined(APT_RENDER_FLAGS)
    /// Added new callbacks for pushing and popping render flags data.
    /// _renderflags is the new property defined for movieclips that can be interpreted by
    /// Apt and passed it to apt-aux before actually rendering the movieclip.
    /// For example in actionscript user can write like this.
    ///          myMovie._renderflags = "shadername" ;
    /// The string value set to _renderflags in actionscript will be passed to pfnPushRenderFlags
    /// callback function which can then interpret in information stored in that string as it wants
    /// and apply the effect. Push and pop callbacks gives flexibility of applying rendering effects
    /// to nested movieclips.
    void (*pfnPushRenderFlags)(const char *pszRenderFlags);
    void (*pfnPopRenderFlags)(const char *pszRenderFlags);
#endif

    // Callbacks for file IO, newly added for Actionscript profiling. It can be used for another file handling situations.

    /// Callback for the getTimer Opcode, should return the number of elapsed microseconds.
    uint64_t (*pfnGetCurrentTime)();

    // Added new callbacks in 2.06.00 for UI Effects.

    /// This callback is called from simulation thread when Apt is generating RenderBuffer list.
    /// AptAux library should lookup if there are any modules for effects that are passed through AptEffectData structure.
    /// @param uiEffectData contains blendMode and pointer to AptValue (AptArray) which has filter objects in it.
    /// @param movieclip is pointer to AptValue that represents movieclip for which this function is called.
    /// Apt will call this function every frame for every movieclip that has has blendModes and/or filters attached.
    /// AptAux library should return AptAssetEffect (void *) object that Apt stores inside RenderElement and
    /// passes back to AptAux through pfnDrawRenderingUnit, pfnDrawString, AptCallbackCustomControlRenderWithZid
    /// function in AptRenderInfo parameter at time of rendering.
    AptAssetEffect (*pfnCreateEffect)(const AptEffectData &uiEffectData, AptValue *movieClip);

    /// This callback is called from simulation thread when Apt is cleaning up the RenderBuffer list.
    /// Apt passes back the same AptAssetEffect that was returned to Apt at time of pfnCreateEffect callback function.
    /// This is called every frame for every movieclip that has blendMode and/or filters attached to it and if AptAux has returned
    /// a valid AptAssetEffect for that moveclip.
    /// AptAux is supposed to clean up the resources attached to AptAssetEffect.
    void (*pfnDestroyEffect)(AptAssetEffect uiEffect);

    /// This callback is called from render thread when Apt is actually going through RenderBuffer list for rendering them.
    /// AptRenderInfo structure has AptAssetEffect which was returned by AptAux for pfnCreateEffect callback function
    /// AptAux is suppose to store any state changes, setup new resources that are needed for effect to be applied for
    /// primitives that are rendered after this callback.
    void (*pfnPushEffect)(const AptRenderInfo &renderInfo);

    /// This callback is called from render thread when Apt is actually going through RenderBuffer list for rendering them.
    /// AptRenderInfo structure has AptAssetEffect which was returned by AptAux for pfnCreateEffect callback function
    /// AptAux is suppose to re-store any state changes, cleanup resources that were created for applying effects (pfnPushEffect)
    /// for primitives that were rendered after previous pfnPushEffect callback.
    void (*pfnPopEffect)(const AptRenderInfo &renderInfo);

    /// This callback happens when an AptReset takes place and the new target instance has been created
    void (*pfnResetTargetInstance)(AptTargetInstance targetInstance);

    /// Locale-aware convert string to upper case.
    void (*pfnUpperCaseString)(const char *utf8In, char *utf8Out, unsigned int utf8OutBytes);

    /// Locale-aware convert string to upper case.
    void (*pfnLowerCaseString)(const char *utf8In, char *utf8Out, unsigned int utf8OutBytes);

    AptUserFunctions();

    //@}
};

AptUserFunctions &AptGetUserFuncs(void);

class AptGlobalExtensionObject;
AptGlobalExtensionObject *AptGetGlobalExtensionObject(void);

/// This structure is used to define the different pool sizes and stack sizes that Apt will use
/// through out its life time.
struct AptInitParams
{
    /// size of the button set (default = 512)
    int iButtonSetSize{512};
    /// size of the input set (default = 512)
    int iInputSetSize{512};
    /// size of the listener set (default = 64). This is basically key and mouse listener set size.
    int iListenerSetSize{64};
    /// size of the action pool (default = 256)
    int iActionPoolSize{256};
    /// maximum number of interval functions per frame(default = 64)
    int iMaxIntervalFunctions{64};
    /// maximum number of button instances per frame(default = 256)
    int iMaxButtonInstances{256};
    /// maximum number of new movie clips which can be created per frame(default = 384).
    int iMaxNewMovieClipsPerFrame{2048};
    /// the maximum number of queued inputs per frame (default = 64)
    int iMaxQueuedInputs{64};
    /// This is size of stack that AptActionInterpreter uses to temporary push/pop items while interpreting actionscript
    /// byte code (default = 384)
    int iStackSize{600};

    // Removed iLocalFrameStackSize, iWithStackSize, iSetTargetStackSize, iThisStackSize in Apt 0.18.00

    /// This number now replaces older iLocalFrameStackSize, iWithStackSize, iSetTargetStackSize,
    /// iThisStackSize numbers. iCallStackDepth will serve as the size of all above stacks.
    /// All these stacks are related to how deep the call stack of actionscript functions can be.
    /// (default = 32)
    int iCallStackDepth{32};

    /// Deferred Vector Size: Deferred vector is in place to try to reduce iCache misses due to
    /// trashing the cache with destructors all the time. It batches up to this number of
    /// objects so that we only run destructors when we hit this. This can be put as low as 1
    /// if you want the memory back.
    /// (default = 256)
    int iDeferedVectorSize{256};

    /// This is the size of the string pool coming from the script
    /// You can increase the size if you have a lot of strings in your script
    /// And _parseStream becomes too slow
    /// (default = 1024)
    int iStringPoolSize{1024};

    /// This is the maximum number of registers that can be in use at any given time. The Number needed is Dependant on
    /// whether how the files are exported (Flash Publish Settings) and on how deep the call stack can get. It is usually
    /// safe to set this to 4 times the max call stack depth achieved in the Actionscript code.
    /// This number will also be used for allocating the global AptRegisters for memory optimization.
    /// (default = 128)
    int iRegArraySize{128};

    /// This is the maximum number of AptLookups which will be allocated at Apt initialization time
    /// If user gets an assert in AptLookup::Create function then increase this value.
    /// (default = 128)
    int iLookupArraySize{128};

    /// This vector is used to keep track of files that are required to stay in memory after they are removed.  Right now, data files will only stay around
    /// if external function references are found.  Doing this should be avoided since it can take up more memory then is desired.  If this is done, you should
    /// reconsider and evaluate your swf files to resolve this.
    int iZombieVectorSize{8};
    /// All TextFields in Flash have the mouseWheelEnabled flag set to true by default.  Doing this by default for all TextFields could be costly so we will set this flag
    /// to false by default.  If you'd like Apt to set this property to true by default, set the bDefaultMouseWheel flag to true.
    bool bDefaultMouseWheelFlag{false};
    /// When turned on, Apt will print out a quick view of all the script functions that are maintaining external references.
    /// To print out the external references, you must define both APT_XML_MEMORY_DUMP_SUPPORTED and APT_USE_REGISTER_CALLBACKS.
    /// These are automatically defined in the Debug builds and off in all other builds.  To get even more information on the
    /// memory allocations in Apt, you can use PrintObjectMapXML().
    bool bPrintZombieDump{false};
    /// When turned on, Apt will call 3 callback functions, namely to create, update, destroy custom control ZIds. Set to false by default.
    bool bCustomControlUseZid{false};
    /// This turns off the apt debugger even if the Apt library was built with it
    /// AptDebugger blocks the game waiting for a connection. Disabling it is possible to run in "normal" mode
    /// an apt debugger built, without running the PC part to establish the connection
    bool bAptDebuggerEnabled{false};
    // This flag is to replace compile time #define APT_USE_NEW_CLASS_INIT_ORDER, and make it runtime option.
    /// When turned on this will use the new class initialization order fix where parent class constructors for movieclip get
    /// called before child class constructors for movieclips get called. (Default = false)
    bool bUseNewClassInitOrder{false};

    /// When turned on this will skip the 'trace' bytecode and all the bytecodes before 'trace' that generate trace string.
    /// Note : This will work if and only if the actionscript is modified to have "AptTrace();" or "_global.AptTrace()" call immediately
    /// before actual trace call. And also '--optimize' flag is passed to SWFC parser to invoke aTackCompiler.exe.
    /// Something like this
    /// @verbatim
    ///  AptTrace();  trace("This is a test trace " + mc1.myfunction1()) ;
    ///  AptTrace();
    ///  trace("This is a another test trace " + mc2.myfunction2()) ;
    /// @endverbatim
    /// If trace is inside a class function then you have to use "_global.AptTrace()".
    /// When "AptTrace();" call is added before trace(...) call, aTackCompiler.exe detects this new function call and replaces it
    /// with a new bytecode that only SWFC can parse. At runtime Apt will just directly jump to bytecode after 'trace' if bSkipTraceBytecodes
    /// is set to true. Another API function AptSetSkipTracesFlag is also added that can allow user to turn this flag on/off from game
    /// side after Apt is initialized.
    bool bSkipTraceBytecodes{false};

    AptInitParams()

    {
    }
};

// These are the base init params that are used by the AptTarget object
struct AptTargetInitParams
{
    /// maximum number of interval functions per frame(default = 64)
    int iMaxIntervalFunctions{64};
    /// the maximum number of queued inputs per frame (default = 64)
    int iMaxQueuedInputs{64};
    /// size of the listener set (default = 64). This is basically key and mouse listener set size.
    int iListenerSetSize{64};
    /// size of the listener set (default = 64). This is basically key and mouse listener set size.
    int iInputSetSize{512};
    /// size of the button set (default = 512)
    int iButtonSetSize{512};
    /// size of the action pool (default = 256)
    int iActionPoolSize{256};

    AptTargetInitParams()

    {
    }

    void CopyAptInitParms(const AptInitParams *aptInitParms)
    {
        iMaxIntervalFunctions = aptInitParms->iMaxIntervalFunctions;
        iMaxQueuedInputs      = aptInitParms->iMaxQueuedInputs;
        iListenerSetSize      = aptInitParms->iListenerSetSize;
        iInputSetSize         = aptInitParms->iInputSetSize;
        iButtonSetSize        = aptInitParms->iButtonSetSize;
        iActionPoolSize       = aptInitParms->iActionPoolSize;
    }
};

/// This allows to turn traces on/off at runtime.
/// @param bSkipTraces When turned on, this will skip the 'trace' bytecode and all the bytecodes before 'trace'
/// that generate trace string.
/// Note : This will work if and only if the actionscript is modified to have "AptTrace();" call immediately before actual trace call.
/// And also '--optimize' flag is passed to SWFC parser to invoke aTackCompiler.exe.
void AptSetSkipTracesFlag(bool bSkipTraces);

/// Initializes the Apt library. Must be called before other Apt API functions,
/// but \i after the callbacks have been filled out. Normally, this means
/// you'll call the auxiliary library initialize function before AptInitialize.
/// It now takes a pointer AptInitParams structure as parameter which will be used to
/// get different stack sizes and pool sizes. NULL is set as default parameter value and
/// in that case it will use the default values as written in description of AptInitParams.
/// User can not change the pool sizes/stack sizes once Apt is Initialized with this call.
/// User will have to call AptShutDown() and then call AptInitialize() with new sizes.
/// If game just calls AptInitialize in multithreaded environment then Apt will assume that
/// both Render and Update are running in same thread, it will initialize Apt in that way.
// void AptInitialize(void);
void AptInitialize(AptLibraryHandle hLib, const AptInitParams *pAptInitParms = NULL, bool bCreateDefaultTarget = true);

/// Now instead of AptInitialize game should call initialize on update and render
/// sides separately. This will initialize Update side and should be called from
/// update/simulation thread. Now there are  asserts added in debug mode for checking
/// threadid so when this function gets called Apt uses the current thread as update thread
/// and then for all other update related functions Apt will assert if functions are not called
/// from same thread.
void AptUpdateInitialize(AptLibraryHandle hLib, const AptInitParams *pAptInitParms = NULL, bool bCreateDefaultTarget = true);

/// Now instead of AptInitialize game should call initialize on update and render
/// sides separately. This will initialize Render side and should be called from
/// render thread. Now there are  asserts added in debug mode for checking
/// threadid so when this function gets called Apt uses the current thread as render thread
/// and then for all other render related functions Apt will assert if functions are not called
/// from same thread.
void AptRenderInitialize(AptLibraryHandle hLib);

/// Shuts down Apt and frees all acquired resources.
void AptShutdown(AptLibraryHandle hLib, int bQuiet = false);

/// This should be called to shutdown the Update side of Apt when running in multithreaded
/// environment.
void AptUpdateShutdown(AptLibraryHandle hLib, int bQuiet = false);

/// This should be called to shutdown the Render side of Apt when running in multithreaded
/// environment.
void AptRenderShutdown(AptLibraryHandle hLib);

// Added a new function that will reset Apt to its original state when it was initialized.
/// This function will reset Apt to its original state when it was started.
/// It will clear the displaylist, unload all the animations, call garbage collection,
/// remove any pending loader/linker requests for animations, reset all the allocated Pools.
/// This function will not allocate any new pools, but it will clean all the contents of
/// _global, all the actionscript objects, all extension objects. In other words it will make
/// Apt in a state as if it just initialized.
void AptReset(AptLibraryHandle hLib);

/// Parameters for AptLibraryInitialize(). Supersedes AptAllocatorInitialize()'s
/// pool sizes, and carries the user callbacks that used to be poked into the
/// gAptFuncs global before initialising.
struct AptLibraryInitParams
{
    /// User callbacks. Required: Apt cannot allocate without pfnMemAlloc/pfnMemFree.
    /// Supplying these here rather than through a global is why AptLibraryInitialize
    /// can validate them at the point they are given.
    AptUserFunctions Funcs;

    size_t nGCMainPoolSize        = 0x10000;
    size_t nGCOverflowPoolSize    = 0x4000;
    size_t nNonGCMainPoolSize     = 0x10000;
    size_t nNonGCOverflowPoolSize = 0x4000;

    /// APT_OPT_* flags, previously set with AptOptEnable() before AptInitialize().
    unsigned long nOptFlags = 0;
};

/// Creates an Apt library instance: validates the callbacks, then builds the
/// allocation pools. Call before AptInitialize(); the returned handle brackets
/// the whole of Apt's lifetime and is passed to every lifecycle call.
///
/// Replaces AptAllocatorInitialize(). The old function existed outside
/// AptInitialize()/AptShutdown() and hand-enforced that nesting with asserts;
/// the handle makes the nesting structural.
///
/// @return the new library, or NULL if the required callbacks are missing.
AptLibraryHandle AptLibraryInitialize(const AptLibraryInitParams *pAptLibInitParms);

/// Destroys a library created by AptLibraryInitialize(), reporting any leaks.
/// AptShutdown() must have been called first. Replaces AptAllocatorShutdown().
void AptLibraryShutdown(AptLibraryHandle hLib);

/// Initiates the load of an animation into the target specified by
/// szTarget. szBaseName should not include any extension; it
/// should just be the base file name of the animation to be loaded.
/// szTarget is in slash-syntax format: for example, /_level0 would
/// load the animation into the lowest level in the player. Additionally, a special
/// target of /#import can be used to preload an asset animation that the
/// game knows will be referenced by other screens.
///
/// The flow of loading is:
/// -# Game calls AptLoadAnimation.
/// -# Apt locates the animation, and calls AptGetUserFuncs().pfnLoadAnimation.
/// -# Game begins (possibly asynchronous) loading procedure.
/// -# Game completes asynchronous load, and calls
/// AptCompleteAnimationAsyncLoad (below) with the loaded data.
/// -# Apt resolves and links the animation data and begins playing the animation.
void AptLoadAnimation(
    const char *szBaseName,
    const char *szTarget);

/// Same as AptLoadAnimation above only this takes a target to set up before calling the function
void AptTargetLoadAnimation(
    AptTargetInstance Target,
    const char *szBaseName,
    const char *szTarget);

/// Loads a file, but doesn't specifically link it into the playing levels of
/// animations. This can be helpful for load time optimizations on the game side.
AptFilePtr AptPreloadAnimation(const char *szBaseName);

/// Same as AptPreloadAnimation above only this takes a target to set up before calling the function
AptFilePtr AptTargetPreloadAnimation(
    AptTargetInstance Target,
    const char *szBaseName);

/// This will remove the preloaded animations with call to AptPreloadAnimation from Apt data structures.
/// It will also remove any other imported libraries that have been loaded because of preloaded animations.
/// Apt will try to walk through all the currently running animations and check if any of the imported library
/// is not getting accidentally removed because call to this function. In this operation Apt might remove some
/// imported libraries which are actually needed by running animations. For example an already loaded animation A imports library
/// B and B in turn imports D, C and D in-turn imports library E. Now another animation F is preloaded and it also imports E.
/// While going through all the list, right now Apt has no way to find out if A is also indirectly importing E and so while doing
/// operation, Apt might remove E library. But this will happen only for nested library imports. And current code
/// will work with one level deep of library imports and will not accidentally remove libraries that are imported by
/// currently running animations.
void AptCancelPreloadedAnimation(const char *szBaseName);

/// Same as AptCancelPreloadedAnimation above only this takes a target to set up before calling the function
void AptTargetCancelPreloadedAnimation(
    AptTargetInstance Target,
    const char *szBaseName);

/// Tests to see if a file is currently completely loaded.
/// @todo
bool AptIsFileLoaded(AptFilePtr file);

/// Same as AptIsFileLoaded above only this takes a target to set up before calling the function
bool AptTargetIsFileLoaded(
    AptTargetInstance Target,
    AptFilePtr file);

/// Indicates the completion of an Animation load operation. @param pData is the main .apt
/// file that contains the animation data. @param pConstTable is the .const file,
/// which contains constants that are used by Apt only during resolving the
/// animation (they can be discarded when AptGetUserFuncs().pfnFreeConstantTable is
/// called, which will be immediately after asynchronous load is completed and the
/// animation is resolved). @param pUserHandle is a user data field that must
/// uniquely identify this animation (and is used in the callbacks for loading
/// animation assets).
void AptCompleteAnimationAsyncLoad(
    AptFilePtr pAsyncLoadContext,
    void *pData,
    void *pConstTable,
    void *pUserHandle);

/// Wraps AptCompleteAnimationAsyncLoad passing in a target instance
void AptTargetCompleteAnimationAsyncLoad(AptTargetInstance Target, AptFilePtr file, void *pData, void *pConstTable, void *pUserData);

/// Indicates the completion of an Image load operation.
void AptCompleteImageAsyncLoad(AptFilePtr asyncLoadContext, AptAssetTexture texture, int width, int height, void *userData);
/// Wraps AptCompleteImageAsyncLoad passing in a target instance
void AptTargetCompleteImageAsyncLoad(AptTargetInstance Target, AptFilePtr file, AptAssetTexture texture, int width, int height, void *userData);

/// The update function. Should be called in the main update loop of the game.
/// @param nDeltaTime is the number of elapsed milliseconds since the last time
/// AptUpdate() was called.
/// Now added extra parameter to AptUpdate() that can specify which animation levels
/// should be updated/ticked. This feature allows game teams to selectively update/tick the
/// levels. Game code can change the levels to be updated in between 2 calls to updated and
/// achieve result of selectively updating different animation levels.
/// The default value is kept as AptAnimLevel_All
/// Important thing to be remembered here is that, the selective update/tick will only affect the
/// animation levels and not input processing and timers. As movieclips can be added to various
/// input listener sets, Apt will not be able to find out which movieclips to handle or not.
/// The selective AptUpdate will not be effective for input processing. So game teams should make sure that
/// if game does not want to handle inputs in specific level that is not often updated then
/// do not add any movieclips from those animations to input listener sets.

void AptUpdate(unsigned int nDeltaTimeMS,
               AptAnimLevelE eAnimLevels = AptAnimLevel_ALL);

void AptUpdateRender(AptAnimLevelE eAnimLevels = AptAnimLevel_ALL);

/// Same as AptUpdate above only this takes a target to set up before calling the function
void AptUpdateTarget(
    AptTargetInstance Target,
    unsigned int nDeltaTimeMS,
    AptAnimLevelE eAnimLevels = AptAnimLevel_ALL);

void AptUpdateRenderTarget(
    AptTargetInstance Target,
    AptAnimLevelE eAnimLevels);

/// The render function. Should be called in the main render loop of the game, while
/// drawing is allowed. The rendering-type callback functions of gAptFuncs
/// will only be called from within this function.
/// Now added extra parameter to AptRender that can specify which animation levels
/// should be displayed. This feature allows game teams to selectively display the
/// levels. Game code can change the Render target in between 2 calls to AptRender and
/// achieve result of displaying different animation levels on different render targets.
/// The default value is kept as AptAnimLevel_All
void AptRender(uint32_t nDeltaTimeMS, AptAnimLevelE eAnimLevels = AptAnimLevel_ALL);
/// Same as AptRender above only this takes a target to set up before calling the function
void AptRenderTarget(
    AptTargetInstance Target,
    uint32_t nDeltaTimeMS,
    AptAnimLevelE eAnimLevels = AptAnimLevel_ALL);

/// Call the garbage collection at the end of the next update.
/// This function can be called by the game teams if they want a partial garbage collection
/// before calling AptShutdown(). This function call takes an important amount of CPU cycles
/// (depending of your script complexity) so avoid to call it too often.
void AptPartialGarbageCollection(void);

/// Adds a mouse-position-set to Apt's input queue. If there's no mouse on the
/// target platform this need not be called.
void AptSetMousePosition(int nX, int nY);

/// Same as AptSetMousePosition above only this takes a target to set up before calling the function
void AptTargetSetMousePosition(
    AptTargetInstance Target,
    int nX,
    int nY);

/// Adds an input to Apt's input queue of the given type (eInput), in the
/// given state (eState, pressed or released), from the given controller
/// (eController). The possible values for the input type, state, and
/// controller can be found in enum AptInputType.
void AptAddToInputQueue(
    AptInputType eInput,
    AptInputState eState,
    AptInputController eController);

/// Same as AptAddToInputQueue above only this takes a target to set up before calling the function
void AptTargetAddToInputQueue(
    AptTargetInstance Target,
    AptInputType eInput,
    AptInputState eState,
    AptInputController eController);

/// Adds an input to Apt's analog input queue, the AptAnalogStickInfo object
/// should contain the new position data along with the controller number and either
/// the left or right analog stick AptInputType.
void AptAddToInputAnalogQueue(
    AptAnalogStickInfo pAnalogInput);

/// Same as AptAddToInputAnalogQueue above only this takes a target to set up before calling the function
void AptTargetAddToInputAnalogQueue(
    AptTargetInstance Target,
    AptAnalogStickInfo pAnalogInput);

/// Adds an input to Apt's gesture input queue, the AptGestureInfo object
/// should contain the new gesture data
void AptAddToInputGestureQueue(
    AptGestureInfo pGestureInput);

/// Same as AptTargetAddToInputGestureQueue above only this takes a target to set up before calling the function
void AptTargetAddToInputGestureQueue(
    AptTargetInstance Target,
    AptGestureInfo pGestureInput);

#if defined(APT_ALTERNATE_INPUT)
/// Adds an alternate input to alternate input queue.
/// To handle this input inside action script there has to be a movie clip object added to AlternateInput set
/// with AlternateInput.addListener() function. Apt makes a copy of the string member inside AptAltInput.
void AptAddAlternateInput(
    const char *szEvent,
    AptValue *pValue);

/// Same as AptAddAlternateInput above only this takes a target to set up before calling the function
void AptTargetAddAlternateInput(
    AptTargetInstance Target,
    const char *szEvent,
    AptValue *pValue);
#endif

/// Sets the input root for Apt. Any inputs add after this will only be
/// processed by this object or objects that are children of it. To reset
/// to default behavior, szRoot can be NULL.
/// The boolean return value indicates the success of the operation;
/// setting the root to 0 will always succeed.
bool AptSetInputRoot(const char *szRoot);

/// Returns the dimensions of the animation. This will be the dimensions of the
/// animation that's loaded into the root of the player (normally, /_level0).
void AptGetAnimationSize(int *pnWidth, int *pnHeight);

/// Same as AptGetAnimationSize above only this takes a target to set up before calling the function
void AptTargetGetAnimationSize(
    AptTargetInstance Target,
    int *pnWidth,
    int *pnHeight);

/// Should be called when the main animation is first loaded to set focus to
/// an arbitrary button. This cannot be done inside of Apt, as it has no knowledge
/// of how animations are being set up to construct screens, or when it would
/// be appropriate to set focus to something when it detects that no button has
/// focus.
void AptSetValidFocusButton(void);

/// Same as AptSetValidFocusButton above only this takes a target to set up before calling the function
void AptTargetSetValidFocusButton(void);

/// Sets the value of a variable. @param szName is a slash-style path to a
/// variable, and @param szValue is the value of the variable that is to be
/// set.
void AptSetInternalVariable(const char *szName, const char *szValue);

/// Same as AptTargetSetValidFocusButton above only this takes a target to set up before calling the function
void AptTargetSetInternalVariable(
    AptTargetInstance Target,
    const char *szName,
    const char *szValue);

/// Gets the value of a variable. @param szName is a slash-style path to a
/// variable, and @param szValue is the value of the variable that
/// has been retrieved.
void AptGetInternalVariable(const char *szName, char *szValue);
void AptGetInternalVariable(const AptNativeString *szName, char *szValue);

/// Same as AptTargetGetInternalVariable above only this takes a target to set up before calling the function
void AptTargetGetInternalVariable(
    AptTargetInstance Target,
    const char *szName,
    char *szValue);

/// Calls an ActionScript function. @param szName is the slash-style path to the
/// function. @param szReturnValue is the return value from the function. This can
/// be 0 (NULL) if you don't care for the return value.
/// @param szThisObject is the slash-style path to the object that is used as
/// the context for the function to run in. If this parameter is 0 (NULL)
/// the function will be called as if the context of /_level0.
/// @param nNumParams is the number of char * parameters to the function
/// that appear in the varargs.
void AptCallFunction(
    const char *szName,
    char *szReturnValue      = 0,
    const char *szThisObject = 0,
    int nNumParams           = 0,
    ...);

void AptCallFunctionV(
    const char *szName,
    char *szReturnValue,
    const char *szThisObject,
    int nNumParams,
    va_list &varargs);

/// @param nNumParams is the number of AptValue * parameters to the function
/// that appear in the varargs.
void AptCallFunctionOpti(
    const char *szName,
    char *szReturnValue      = 0,
    const char *szThisObject = 0,
    int nNumParams           = 0,
    ...);

void AptCallFunctionVOpti(
    const char *szName,
    char *szReturnValue,
    const char *szThisObject,
    int nNumParams,
    va_list &varargs);

void AptCallFunctionAOpti(
    const char *szName,
    char *szReturnValue,
    const char *szThisObject,
    int nNumParams,
    AptValue **apValues);

// $senzee Calls an ActionScript function. @param szName is the slash-style path to the
/// function. @param szReturnValue is the return value from the function. This can
/// be 0 (NULL) if you don't care for the return value.
/// @param pThis is the AptValue * that points to the appropriate actionscript *this*
/// object reference. If this parameter is 0 (NULL)
/// the function will be called as if the context of /_level0.
/// @param nNumParams is the number of AptValue * parameters to the function
/// that appear in the varargs.
void AptCallMemberFunction(
    const char *szName,
    char *szReturnValue = 0,
    AptValue *pThis     = 0,
    int nNumParams      = 0,
    ...);

void AptCallMemberFunctionV(
    const char *szName,
    char *szReturnValue,
    AptValue *pThis,
    int nNumParams,
    va_list &varargs);

/// Call an actionscript function that you already have ahold of.
AptValue *AptCallFunctionObject(AptValue *pFuncValue, int nNumParams, ...);
AptValue *AptCallFunctionObjectA(AptValue *funcValue, int numParams, AptValue **params);

/// Same as AptCallFunction's above only these take a target to set up before calling the function
void AptTargetCallFunction(AptTargetInstance Target, const char *szName, char *szReturnValue = 0, const char *szThisObject = 0, int nNumParams = 0, ...);
void AptTargetCallFunctionV(AptTargetInstance Target, const char *szName, char *szReturnValue, const char *szThisObject, int nNumParams, va_list &varargs);
void AptTargetCallFunctionOpti(AptTargetInstance Target, const char *szName, char *szReturnValue = 0, const char *szThisObject = 0, int nNumParams = 0, ...);
void AptTargetCallFunctionVOpti(AptTargetInstance Target, const char *szName, char *szReturnValue, const char *szThisObject, int nNumParams, va_list &varargs);

/// Seeds Apt's internal random number generator. This is not required, but can
/// be useful to control the repeatability or network characteristics of Apt's
/// random number generation.
void AptSeedRand(unsigned int nSeed);

/// Frees all allocated AptAssetStrings. On the next render frame they will be
/// reallocated. This is useful if you have changed languages, and so want to
/// reallocate strings that have now changed their mapping. It is also
/// useful for font rasterization methods with a texture cache, as this will
/// defragment the cache (albeit in a pretty brute-force manner).
void AptDeallocateAllStrings();

/// Same as AptDeallocateAllStrings's above only this takes a target to set up before calling the function
void AptTargetDeallocateAllStrings(AptTargetInstance Target);

/// Returns a string that includes the Apt version number and date and time
/// at which the library was built.
const char *AptDebugVersionString();

/// Enable/disable saving of inputs. If this function is called with bEnabled
/// set to true, then pfnDebugAddSavedInput() and
/// pfnDebugSetScreenGrabPending() will be called and used to save the saved
/// input data.
void AptDebugEnableSavedInputs(int bEnabled);

/// Plays back saved inputs. @param aSavedInputs is a pointer to a stream
/// that was saved by using AptDebugEnableSavedInputs() and
/// recorded by using pfnDebugAddSavedInput().
void AptDebugPlaySavedInputs(
    AptSavedInputRecord *aSavedInputs,
    int nSavedInputsFileSize);

/// Stops playback of Saved Inputs immediately. User input is not accepted.
void AptDebugStopSavedInputs(void);

class IAptXmlImpl;

/// Sets the XML implementor. @param pIAptXmlImpl is a pointer IAptXmlImpl object
/// that user has created for XML implementation. Apt will save this pointer and use
/// it for generating new XML objects in future when there is call to
/// myDoc = new XML;
void AptSetXMLImplementor(IAptXmlImpl *pIAptXmlImpl);

/// Flushes the AptInputQueue.
void AptFlushInputQueue(void);

// old unfortunate name for AptDeallocateAllStrings
#define AptDeallocAllAssetString AptDeallocateAllStrings

int AptDebugGetCurrentFrame(int nLevel);
int AptDebugGetNumFrames(int nLevel);
int AptDebugIsPlaying(int nLevel);

// Mouse Over Button detection
bool AptIsMouseOverButton(void);

void AptRegisterExtension(AptExtObject *pExtObject);
void AptUnRegisterExtension(AptExtObject *pExtObject);
void AptUnRegisterExtension(const char *pExtObjectName);

// These are here as a (hopefully?) temporary measure to allow Fifa to compile.
// I'm not entirely certain why Fifa needs these publicly exposed, as I am sure
// calling them can only break everything.  So please never call them.
// Today's date is 11/11/11.  If you are reading this in the year 2015, please delete
// this crap and see if Fifa still compiles.
void AptStackPush(AptValue *const pValue);
void AptStackPop(const int32_t nItems);

// Apt_DO_BREAK
/// This function can be used to turn of the break statement after assert in Apt code.
/// This can be helpful in debugging the application and continue to execute the program even after
/// assert statement inside Apt is executed.
/// User should be aware that after Assert statement is executed in Apt, further execution results may not be as expected.
/// Default is set to true and Apt will break after Assert statement.
/// Set it to 0 to turn off 'break' after Assert in Apt.
void AptBreakAfterAssert(int bBreakAfterAssert);

/// Apt now keeps files in memory that have external function references.  Files being held onto are placed in a Zombie vector and are
/// only release when all external references are released.  This function provides a way to see everything being held onto by Apt.  The output
/// will show the file name, file size, number of external references, and the sprite name if it has one.  If the zombie vector is being use
///  evaluation of your swf's is highly recommended.
void AptDebugPrintZombieVector();

/// Removes a given CIH from the zombie vector
/// @param pTarget A pointer to the CIH to potentially remove
/// @return true if the CIH was found and removed, false otherwise
bool AptRemoveCIHFromZombieVector(AptCIH *pTarget);

// Need to determine the name of the button that the mouse is over
/// This API function allows the game to see what Mouse listening movieclip is currently under the mouse cursor.  This function only
/// handles movieclips that have one or more mouse events, movieclips that do not handle mouse events will not have their names returned.
void AptGetMouseOverSpriteName(char *szName);

// Added next set of debugging information functions in 0.18.00 - ASK
/// This function returns the current number of animations, sprites, shapes, textfield etc.
/// For more information on what numbers are returned please look in AptMovieclipInformation.
/// @param pMCInfo is pointer to a AptMovieclipInformation information object. Apt will fill
/// various counters in this objects. So this is a in-out parameter.
void AptGetMovieclipInfo(AptMovieclipInformation *pMCInfo, bool bGenerateNew = false);

/// This function prints all the instances of DynamicText in the movieclip tree. It should be only used from
/// Simulation/Update thread and should be used for debugging only.
/// @param eAnimLevels can be used to selectively print dynamic text instances from selected Flash levels
void AptPrintAllDynamicText(AptAnimLevelE eAnimLevels = AptAnimLevel_ALL);

/// This function prints a xml dump of whole movieclip tree. It should be only used from
/// Simulation/Update thread and should be used for debugging only. It prints the tree for the default AptTargetInstance
/// if you are using AptTargets.
/// @param eAnimLevels can be used to selectively print movieclip tree from selected Flash levels
void AptPrintMovieclipTree(AptAnimLevelE eAnimLevels = AptAnimLevel_ALL);

/// @param szFilename specifies the name of the file to create and dump the display list data to.
void AptDumpDisplayList(const char *szFileName);

/// This function prints a xml dump of all the SWF files loaded. It prints the SWF files in the default AptTargetInstance
/// if you are using AptTargets. It should be only used from Simulation/Update thread and should be used for debugging only.
/// @param bPrintImports can be used to print imported files section for the loaded SWF files.
/// @param bPrintExports can be used to print exported movieclips section for the loaded SWF files.
/// Turning on both flags can print a big xml dump.
void AptPrintLoadedSWFFiles(bool bPrintImports = false, bool bPrintExports = false);

/// Adds a custom Saved input to the saved inputs buffer (if we are logging).
/// This message is logged along with the time it was received and is replayed when playing the
/// saved input buffer later. These messages must be handled by the Game / Aux in the pfnCustomSavedInputHandler
/// callback.
void AptAddCustomSavedInput(void *pBuffer, unsigned int nSize);

/// Simply asserts if the current thread running is not the Simulation thread
void AptSetSimulationThread();
void AptAssertIsSimulationThread();
bool AptIsSimulationThread();

/// Simply asserts if the current thread running is not the Render thread
void AptAssertIsRenderThread();

/// \name AptTarget Functions
//@{

/// This function creates a new target instance and passes it back.  You can use this to start a new separate instance of Apt to be used
/// as a render target.  After creating a target, simply use AptChangeTargetInstance to use the target in the AptUpdate/AptRender calls.
/// You can not have more then one target using AptUpdate/AptRender at a time, and you must use AptDestroyTargetInstance to delete the
/// target when finished.
AptTargetInstance AptCreateTargetInstance(const AptTargetInitParams *aptInitParms);

/// This function destroys the passed in target that was created via the AptCreateTargetInstance function
void AptDestroyTargetInstance(AptTargetInstance Target);

/// This function changes the Apt target (for both sim and render) to be used in the next AptUpdate/AptRender calls.  Changing the target affects how most other Apt API
/// functions work since Apt will be using the new target as the main playing movie.  For instance, this function will set the target that inputs
/// will be passed to, all other targets will not receive the input.
void AptChangeTargetInstance(AptTargetInstance Target);

/// This function changes the Apt target (for simulation) to be used in the next AptUpdate/AptRender calls.  Changing the target affects how most other Apt API
/// functions work since Apt will be using the new target as the main playing movie.  For instance, this function will set the target that inputs
/// will be passed to, all other targets will not receive the input.
void AptChangeSimTargetInstance(AptTargetInstance Target);

/// This function changes the Apt target (for render) to be used in the next AptUpdate/AptRender calls.  Changing the target affects how most other Apt API
/// functions work since Apt will be using the new target as the main playing movie.  For instance, this function will set the target that inputs
/// will be passed to, all other targets will not receive the input.
void AptChangeRenderTargetInstance(AptTargetInstance Target);

/// This function will shutdown and restart the Apt target with the same AptInitParams it was created with.  No swf file will be playing after
/// this is called, you must use AptChangeTargetInstance and AptLoadAnimation again on this target.
void AptResetTargetInstance(AptTargetInstance Target);

/// This function sets the Apt target to the main default target for both simulation and render
void AptSetDefaultTargetInstance();

/// This function sets the Apt target to the main default target for simulation
void AptSetSimDefaultTargetInstance();

/// This function sets the Apt target to the main default target for render
void AptSetRenderDefaultTargetInstance();
//@}

/// Function to get a nice name with which to label a function
const char *AptGetFunctionDebugName(AptValue *pFunctionValue, const char *defaultName);

//! Apt optimization flags
#define APT_OPT_SPRSTACK (0x00000001)
#define APT_OPT_NO_ONENTER (0x00000002)
#define APT_OPT_SPRTREE (0x00000004)

// The optimisation flags, the mouse-wheel default and the zombie-dump level
// all live on the library now. Set them through AptLibraryInitParams::nOptFlags
// and AptInitParams, or toggle them with AptOptEnable()/AptOptDisable() below.

// this is done for performance improvements.

/// Following functions/macros can be used to turn on some newly introduced optimization in 0.15.00
///
/// Use APT_OPT_NO_ONENTER with these functions to turn on an optimization in which Apt will not call
/// queueClipEvents(onEnterFrame) for each and every movieclip. You can use it if you know that onEnterFrame
/// from actionscript is not used in any of the animations being played.
///
/// To have flags set before AptInitialize() (the usual case), prefer
/// AptLibraryInitParams::nOptFlags over calling these.
void AptOptEnable(AptLibraryHandle hLib, unsigned long nFlags);
void AptOptDisable(AptLibraryHandle hLib, unsigned long nFlags);
/// @return the APT_OPT_* flags currently set on hLib.
unsigned long AptOptGetFlags(AptLibraryHandle hLib);

#include "AptSharedPtr.h"
