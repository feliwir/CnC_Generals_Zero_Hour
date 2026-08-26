#pragma once

#include "_AptValue.h"
#include "_AptValuePtrStack.h"
#include "_AptBasePtrStack.h"
// #include "_AptValuePtrArray.h"

#if defined(APT_DEBUG)
#include "_AptDebugStack.h"
#endif


#define INPUT_MOUSE 0x00
#define INPUT_KEYBOARD 0x01
#define INPUT_CHECKPOINT 0x02
#define INPUT_OTHER 0x03

// if the first two bits are "INPUT_OTHER" then compare the rest of the byte to these values
#define INPUT_SCREENGRAB ((0x00 << 2) | INPUT_OTHER)
#define INPUT_CUSTOM ((0x01 << 2) | INPUT_OTHER)
#define INPUT_TRIGGER ((0x02 << 2) | INPUT_OTHER)

#define SET_KEYBOARD_INPUT(_type_, _state_, _controller_) (INPUT_KEYBOARD | ((_type_ & 0x7fff) << 17) | ((_state_ & 0x7f) << 10) | ((_controller_ & 0xff) << 2))
#define SET_ANALOG_INPUT(_type_, _index_, _controller_) (INPUT_KEYBOARD | ((_type_ & 0x7fff) << 17) | ((_index_ & 0x7f) << 10) | ((_controller_ & 0xff) << 2))
#define SET_GESTURE_INPUT(_type_, _state_, _controller_) (INPUT_KEYBOARD | ((_type_ & 0x7fff) << 17) | ((_state_ & 0x7f) << 10) | ((_controller_ & 0xff) << 2))
#define SET_MOUSE_INPUT(_x_, _y_) (INPUT_MOUSE | ((_x_ & 0x7fff) << 17) | ((_y_ & 0x7fff) << 2))
#define SET_CHECKPOINT_INPUT(_type_) (INPUT_CHECKPOINT | ((_type_ & 0x1) << 2))
#define SET_SCREENGRAB_INPUT() (INPUT_SCREENGRAB)
#define SET_TRIGGER_INPUT() (INPUT_TRIGGER)

#define INPUT_IS_KEY(_stream_) (GET_TYPE_INPUT(_stream_) == INPUT_KEYBOARD)
// Analog stick support added
#define INPUT_IS_ANALOG(_stream_) (                                                               \
    (INPUT_IS_KEY(_stream_)) &&                                                                   \
    (((AptInputType)((((*_stream_) & (0x7fff << 17)) >> 17)) == AptInputType_RightAnalogStick) || \
     ((AptInputType)((((*_stream_) & (0x7fff << 17)) >> 17)) == AptInputType_LeftAnalogStick)))

#define INPUT_IS_GESTURE(_stream_) (                                                          \
    (INPUT_IS_KEY(_stream_)) &&                                                               \
    (((AptInputType)((((*_stream_) & (0x7fff << 17)) >> 17)) >= AptInputType_GestureStart) && \
     ((AptInputType)((((*_stream_) & (0x7fff << 17)) >> 17)) <= AptInputType_GestureEnd)))

#define INPUT_IS_MOUSE(_stream_) (GET_TYPE_INPUT(_stream_) == INPUT_MOUSE)
#define INPUT_IS_CHECKPOINT(_stream_) (GET_TYPE_INPUT(_stream_) == INPUT_CHECKPOINT)
#define INPUT_IS_SCREENGRAB(_stream_) (GET_TYPE_INPUT(_stream_) == INPUT_SCREENGRAB)

#define GET_ACTION_TYPE(_input_, _type_) \
    _type_ = (AptInputType)(((_input_ & (0x7fff << 17)) >> 17))

#define GET_ACTION_CONTROLLER(_input_, _controller_) \
    _controller_ = (AptInputController)(((_input_ & (0x00ff << 2)) >> 2))

#define GET_INPUT(_input_, _type_, _state_, _controller_)                      \
    {                                                                          \
        _type_       = (AptInputType)(((_input_ & (0x7fff << 17)) >> 17));     \
        _state_      = (AptInputState)(((_input_ & (0x007f << 10)) >> 10));    \
        _controller_ = (AptInputController)(((_input_ & (0x00ff << 2)) >> 2)); \
    }

#define GET_KEYBOARD_INPUT(_input_, _type_, _state_, _controller_) \
    {                                                              \
        APT_ASSERT(INPUT_IS_KEY(&_input_));                        \
        GET_INPUT(_input_, _type_, _state_, _controller_);         \
    }

// Analog stick support added
#define GET_ANALOG_INPUT(_input_, _type_, _controller_) \
    {                                                   \
        APT_ASSERT(INPUT_IS_ANALOG(&_input_));          \
        GET_ACTION_TYPE(_input_, _type_);               \
        GET_ACTION_CONTROLLER(_input_, _controller_);   \
    }

/** @brief Restores the sign of the unpacked input (when it's not supposed to be unsigned) */
inline int unpackSignedBitField(int32_t input, uint32_t uShift)
{
    const int32_t iMask = 0x7fff; // 15 bits fields
    int32_t iResult     = (input >> uShift) & iMask;
    if (iResult & 0x4000) // Bigger bit : sign
    {
        iResult |= ~iMask; // add sign bits
    }
    return iResult;
}

#define GET_MOUSE_INPUT(_input_, _x_, _y_)       \
    {                                            \
        APT_ASSERT(INPUT_IS_MOUSE(&_input_));    \
        _x_ = unpackSignedBitField(_input_, 17); \
        _y_ = unpackSignedBitField(_input_, 2);  \
    }

#define GET_CHECKPOINT_INPUT(_input_, _type_)                    \
    {                                                            \
        APT_ASSERT(INPUT_IS_CHECKPOINT(&_input_));               \
        _type_ = (unsigned int)(((_input_ & (0x01 << 2)) >> 2)); \
    }

// Watch out, some architectures will give an address error exception for unaligned pointers, read out a byte at a time.
#if defined(APT_PLATFORM_PLAYSTATION2)
// This is a little endian implementation. Big endian machines will need a different implementation.
#define GET_UNALIGNED_U32(_ptr_)                            \
    (((unsigned int)*(((uint8_t *)_ptr_) + 0)) << 0) |      \
        (((unsigned int)*(((uint8_t *)_ptr_) + 1)) << 8) |  \
        (((unsigned int)*(((uint8_t *)_ptr_) + 2)) << 16) | \
        (((unsigned int)*(((uint8_t *)_ptr_) + 3)) << 24)

#define GET_UNALIGNED_U16(_ptr_)                       \
    (((unsigned int)*(((uint8_t *)_ptr_) + 0)) << 0) | \
        (((unsigned int)*(((uint8_t *)_ptr_) + 1)) << 8)

#else
#define GET_UNALIGNED_U32(_ptr_) (*(uint32_t *)_ptr_)
#define GET_UNALIGNED_U16(_ptr_) (*(uint16_t *)_ptr_)
#endif

#define GET_TYPE_INPUT(_stream_) (*_stream_ & 0x03)
#define GET_OTHER_INPUT_TYPE(_stream_) (*_stream_ & 0x0f)

extern AptInput gNullInput;

struct AptSavedInputRecord
{
    unsigned int nTick;
};
struct AptSavedInputRecordEOF : public AptSavedInputRecord
{
    unsigned char nInput;
};
struct AptSavedInputRecordInput : public AptSavedInputRecord
{
    unsigned int nInput;
};
struct AptSavedInputRecordCheckpoint : public AptSavedInputRecord
{
    char szBuf[256];
};

struct AptSavedInputRecordCustom : public AptSavedInputRecord
{
    uint16_t nInputType;
    uint16_t nInputBufferSize;
};

struct AptSavedInputRecordTriggers : public AptSavedInputRecordInput
{
    AptAnalogStickInfo mAnalogTriggerInfo;
    AptSavedInputRecordTriggers()
    {
        mAnalogTriggerInfo.fXAxisValue = 0.0f;
        mAnalogTriggerInfo.fYAxisValue = 0.0f;
    }
};

struct AptConstantPool
{
    int nItems;
    AptValue **apItems;
};

struct AptAction_GotoFrame
{
    int nFrame;
};

struct AptAction_GotoFrame2
{
    int bPlay;
};

struct AptAction_GetUrl
{
    char *szUrl;
    char *szWin;
};

struct AptAction_SetTarget
{
    char *szTarget;
};

struct AptAction_GotoLabel
{
    char *szLabel;
};

struct AptAction_PushString
{
    char *szStringToBePushed;
};

struct AptAction_Push
{
    mutable AptConstantPool items;
};

struct AptAction_BranchAddress
{
    int nTargetDelta;
};

struct AptAction_StoreRegister
{
    int nRegister;
};

// TODO: can remove 4 bytes by packing...
struct AptAction_DefineFunction
{
    const char *szName;
    int nParams;
    char **aszParams;
    int nCodeSize;
    mutable AptConstantPool constantPool;
};

// added in 0.16.00 for implementing definefunction2

// For each parameter to the function, a register may be specified.
// If the register specified is zero, the parameter is created as a variable named ParamName in the
// activation object, which can be referenced with ActionGetVariable and ActionSetVariable.
// If the register specified is non-zero, the parameter is copied into the register, and it can be referenced
// with ActionPush and ActionStoreRegister, and no variable is created in the activation object.
struct AptRegisterParam
{
    uint32_t nRegister;
    char *szParamName;
};

// nFlags are defined in this way.

// bit 0 - preloadThisFlag -         0 - Don�t preload this into register,       1 - Preload this into register
// bit 1 - suppressThisFlag -        0 - Create this variable,                   1 - Don�t create this variable
// bit 2 - PreloadArgumentsFlag      0 - Don�t preload arguments into register,  1 - Preload arguments into register
// bit 3 - SuppressArgumentsFlag     0 - Create arguments variable,              1 - Don�t create arguments variable
// bit 4 - PreloadSuperFlag          0 - Don�t preload super into register,      1 - Preload super into register
// bit 5 - SuppressSuperFlag         0 - Create super variable,                  1 - Don�t create super variable
// bit 6 - PreloadRootFlag           0 - Don�t preload _root into register,      1 - Preload _root into register
// bit 7 - PreloadParentFlag         0 - Don�t preload _parent into register,    1 - Preload _parent into register
// bit 8 - PreloadGlobalFlag         0 - Don�t preload _global into register,    1 - Preload _global into register

// enum to get the flag out of nFlags.
enum AptDefinefunction2FlagsType
{
    DF2_PreloadThisFlag = 0,
    DF2_suppressThisFlag,
    DF2_PreloadArgumentsFlag,
    DF2_SuppressArgumentsFlag,
    DF2_PreloadSuperFlag,
    DF2_SuppressSuperFlag,
    DF2_PreloadRootFlag,
    DF2_PreloadParentFlag,
    DF2_PreloadGlobalFlag
};

// TODO: can remove 4 bytes by packing...
struct AptAction_DefineFunction2
{
    const char *szName;   // name of function, empty if anonymous (Valid when unresolved)
    int nParams;          // # of parameters
    short nRegisterCount; // number of registers to allocate
    short nFlags;         // flags that denote what to preload/suppress

    AptRegisterParam *aszParams;
    int nCodeSize; // # of bytes of code that follow
    mutable AptConstantPool constantPool;

    APT_FORCE_INLINE int getDF2Flag(AptDefinefunction2FlagsType eType) const
    {
        return (nFlags & (1 << eType));
    }
};

//------end of new code added for 0.16.00

// enum to get the flag out of uFlags.
enum AptTryCatchBlockFlagsType
{
    TCFB_HasCatchBlock             = 1,
    TCFB_HasFinallyBlock           = 2,
    TCFB_PutCaughtObjectInRegister = 4
};

/*************************************************************************************************\
ActionTry Block Definition defines handlers for exceptional conditions, implementing the
ActionScript try, catch, and finally keywords.

Note:   The CatchSize and FinallySize fields always exist, whether or not the CatchBlockFlag or
        FinallyBlockFlag settings are 1.

Note:   The try, catch and finally blocks do not use end tags to mark the end of their respective blocks.
        Instead, the length of a block is set by the TrySize, CatchSize and FinallySize values.

Note:   uFlags was given private access, so don't use it, foo! use the all purpose functions
        provided instead.
\*************************************************************************************************/
struct AptAction_TryCatchFinallyBlock
{
    uint32_t uTryCodeSize;     // # of bytes of code that follow (after the Struct)
    uint32_t uCatchCodeSize;   // # of bytes of code that follow (After Try Block)
    uint32_t uFinallyCodeSize; // # of bytes of code that follow (After catch Block)
  private:
    uint8_t uFlags; // Flags (See AptTryCatchBlockFlagsType) Use functions to access.
    uint8_t uAlignment1;
    uint8_t uAlignment2;

  public:
    uint8_t uCaughtRegister; // Register # to put caught object in.
    char *szCaughtVarName;   // Var Name to put caught object in.

    // AptConstantPool     constantPool;

    APT_FORCE_INLINE bool hasCatchBlock() const
    {
        return (uFlags & TCFB_HasCatchBlock) != 0;
    }

    APT_FORCE_INLINE bool hasFinallyBlock() const
    {
        return (uFlags & TCFB_HasFinallyBlock) != 0;
    }

    APT_FORCE_INLINE bool putCaughtObjectInRegister() const
    {
        return (uFlags & TCFB_PutCaughtObjectInRegister) != 0;
    }

    APT_FORCE_INLINE uint8_t *getTryBlockBase() const
    {
        return (uint8_t *)(this + 1); // this+1 moves up by sizoef(AptAction_DefineDefineTryCatchFinallyBlock)
    }

    APT_FORCE_INLINE uint8_t *getCatchBlockBase() const
    {
        return (uint8_t *)(this + 1) + uTryCodeSize;
    }

    APT_FORCE_INLINE uint8_t *getFinallyBlockBase() const
    {
        return (uint8_t *)(this + 1) + uTryCodeSize + uCatchCodeSize;
    }
};

struct AptAction_With
{
    unsigned char *pEnd;
};

struct AptActionBlock
{
    unsigned char *aActionStream;
};

struct AptEventActionBlock
{
    // this could probably be nTriggers: 24, nKeyCode: 8 to save 4 bytes
    int nTriggers;
    int nKeyCode;
    AptActionBlock actions;
};

struct AptEventActionSet
{
    int nEventActions;
    AptEventActionBlock *aEventActions;
};

enum AptActionConditionFlag
{
    AptActionConditionFlag_IdleToOverUp      = 0x01,
    AptActionConditionFlag_OverUpToIdle      = 0x02,
    AptActionConditionFlag_OverUpToOverDown  = 0x04,
    AptActionConditionFlag_OverDownToOverUp  = 0x08,
    AptActionConditionFlag_OverDownToOutDown = 0x10,
    AptActionConditionFlag_OutDownToOverDown = 0x20,
    AptActionConditionFlag_OutDownToIdle     = 0x40,
    AptActionConditionFlag_IdleToOverDown    = 0x80,
    AptActionConditionFlag_OverDownToIdle    = 0x100,
    AptActionConditionFlag_KeyPress          = 0xFE00,
};

// For buttons
struct AptActionConditionBlock
{
    int nConditions;
    AptActionBlock actions;
};

struct AptDisplayList;
class AptCharacterInst;
struct AptConstFile;

class AptCIH;
class AptFrameStack;

#if defined(APT_DEBUG)
// make sure that first eventactions should be matched with the onc defined in AptEventActionFlag
enum AptActionType
{
    AptActionType_Invalid        = 0x00000000,
    AptActionType_OnLoad         = 0x00000001,
    AptActionType_EnterFrame     = 0x00000002,
    AptActionType_Unload         = 0x00000004,
    AptActionType_MouseMove      = 0x00000008,
    AptActionType_MouseDown      = 0x00000010,
    AptActionType_MouseUp        = 0x00000020,
    AptActionType_KeyDown        = 0x00000040,
    AptActionType_KeyUp          = 0x00000080,
    AptActionType_Data           = 0x00000100,
    AptActionType_Initialize     = 0x00000200,
    AptActionType_Press          = 0x00000400,
    AptActionType_Release        = 0x00000800,
    AptActionType_ReleaseOutside = 0x00001000,
    AptActionType_RollOver       = 0x00002000,
    AptActionType_RollOut        = 0x00004000,
    AptActionType_DragOver       = 0x00008000,
    AptActionType_DragOut        = 0x00010000,
    AptActionType_KeyPress       = 0x00020000,
    AptActionType_Wheel          = 0x00040000,

    AptActionType_Timer             = 0x00080000,
    AptActionType_Initclip          = 0x00100000,
    AptActionType_FrameActions      = 0x00200000,
    AptActionType_ButtonActions     = 0x00400000,
    AptActionType_InternalFunctions = 0x00800000,
    AptActionType_ArraySortFunction = 0x01000000,
    AptActionType_AssoInstToClass   = 0x02000000,
    AptActionType_CallFunction      = 0x04000000,
    AptActionType_CallMethod        = 0x08000000,
    AptActionType_ConstructorFunc   = 0x10000000
};

#endif

// This new structure isdefined in 0.18.00. This gets used in debug builds to add extra debugging information.
// Before calling runstream, we need to call PrepareForExecution and it needs pointer to this object of this structure.
struct AptActionSetup
{
#if defined(APT_DEBUG)
    const char *m_strFunction;
    AptValue *m_pContext;
    AptValue *m_pFuncValue;
    AptActionType m_eType;

    AptActionSetup(AptValue *pContext, AptValue *pFuncValue, const char *strFunction, AptActionType eType)
        : m_strFunction(strFunction), m_pContext(pContext), m_pFuncValue(pFuncValue), m_eType(eType)
    {
    }
#else
    // AptActionSetup():
#endif
};

// this macro creates the required object for PrepareForExecution.
#if defined(APT_DEBUG)
#define APT_DEFINE_ACTION_SETUP(_pContext, _pFuncValue, _strFunction, _eType) \
    ;                                                                         \
    AptActionSetup oActionSetup(_pContext, _pFuncValue, _strFunction, _eType);
#else
#define APT_DEFINE_ACTION_SETUP(_pContext, _pFuncValue, _strFunction, _eType) \
    AptActionSetup oActionSetup;
#endif

enum Actions
{
    AptActionInvalid = -1,

    AptActionEnd           = 0x00,
    AptActionNextFrame     = 0x04,
    AptActionPrevFrame     = 0x05,
    AptActionPlay          = 0x06,
    AptActionStop          = 0x07,
    AptActionToggleQuality = 0x08,
#if defined(APT_USE_SOUND_OBJECT)
    AptActionStopSounds = 0x09,
#endif
    AptActionAdd                     = 0x0A,
    AptActionSubtract                = 0x0B,
    AptActionMultiply                = 0x0C,
    AptActionDivide                  = 0x0D,
    AptActionEquals                  = 0x0E,
    AptActionLessThan                = 0x0F,
    AptActionAnd                     = 0x10,
    AptActionOr                      = 0x11,
    AptActionNot                     = 0x12,
    AptActionStringEquals            = 0x13,
    AptActionStringLength            = 0x14,
    AptActionSubString               = 0x15,
    AptActionPop                     = 0x17,
    AptActionToInteger               = 0x18,
    AptActionGetVariable             = 0x1C,
    AptActionSetVariable             = 0x1D,
    AptActionSetTarget2              = 0x20,
    AptActionStringAdd               = 0x21,
    AptActionGetProperty             = 0x22,
    AptActionSetProperty             = 0x23,
    AptActionCloneSprite             = 0x24,
    AptActionRemoveSprite            = 0x25,
    AptActionTrace                   = 0x26,
    AptActionStartDragMovie          = 0x27,
    AptActionStopDragMovie           = 0x28,
    AptActionStringLessThan          = 0x29,
    AptActionCastOp                  = 0x2B, // Added for Flash Player 7 and AS 2.0 support (Release 17.0).
    AptActionImplementsOp            = 0x2C, // Added for Flash Player 7 and AS 2.0 support (Release 17.0).
    AptActionThrow                   = 0x2A, // Added for Flash Player 7 and AS 2.0 support (Release 17.0).
    AptActionRandom                  = 0x30,
    AptActionMBLength                = 0x31,
    AptActionCharToAscii             = 0x32, // Renamed for Release 17.0 to match Flash documentation.
    AptActionAsciiToChar             = 0x33, // Renamed for Release 17.0 to match Flash documentation.
    AptActionGetTimer                = 0x34,
    AptActionMBSubString             = 0x35,
    AptActionMBCharToAscii           = 0x36, // Multi-byte version, renamed for Release 17.0 to match Flash documentation.
    AptActionMBAsciiToChar           = 0x37, // Multi-byte version, renamed for Release 17.0 to match Flash documentation.
    AptActionDelete                  = 0x3a,
    AptActionDelete2                 = 0x3b,
    AptActionDefineLocal             = 0x3c,
    AptActionCallFunction            = 0x3d,
    AptActionReturn                  = 0x3e,
    AptActionModulo                  = 0x3f,
    AptActionNewObject               = 0x40,
    AptActionDefineLocal2            = 0x41,
    AptActionInitArray               = 0x42,
    AptActionInitObject              = 0x43,
    AptActionTypeOf                  = 0x44,
    AptActionTargetPath              = 0x45,
    AptActionEnumerate               = 0x46,
    AptActionAdd2                    = 0x47,
    AptActionLessThan2               = 0x48,
    AptActionEquals2                 = 0x49,
    AptActionToNumber                = 0x4a,
    AptActionToString                = 0x4b,
    AptActionPushDuplicate           = 0x4C,
    AptActionStackSwap               = 0x4d,
    AptActionGetMember               = 0x4e,
    AptActionSetMember               = 0x4f,
    AptActionIncrement               = 0x50,
    AptActionDecrement               = 0x51,
    AptActionCallMethod              = 0x52,
    AptActionNewMethod               = 0x53,
    AptActionInstanceOf              = 0x54, // Added for Release 17.0 (Actually in Flash Player 6, but unsupported)
    AptActionEnumerate2              = 0x55,
    AptActionPushThis                = 0x56, // push string 'this'
    AptActionPushGlobal              = 0x58, // push string '_global'
    AptActionPush0                   = 0x59, // push zero
    AptActionPush1                   = 0x5a, // push one
    AptActionCallFuncAndPop          = 0x5b, // call func and pop
    AptActionCallFuncSetVar          = 0x5c, // call func and set var
    AptActionCallMethodPop           = 0x5d, // call method and pop
    AptActionCallMethodSetVar        = 0x5e, // call method and set var
    AptActionBitAnd                  = 0x60,
    AptActionBitOr                   = 0x61,
    AptActionBitXor                  = 0x62,
    AptActionBitLShift               = 0x63,
    AptActionBitRShift               = 0x64,
    AptActionBitURShift              = 0x65,
    AptActionStrictEquals            = 0x66,
    AptActionGreater                 = 0x67,
    AptActionExtends                 = 0x69, // Added for Flash Player 7 and AS 2.0 support. (Release 17.0)
    AptActionPushThisVariable        = 0x70, // push string 'this', get var
    AptActionPushGlobalVariable      = 0x71, // push string '_global', get var
    AptActionPushZeroSetVar          = 0x72, // push 0, set var
    AptActionPushTrue                = 0x73, // push true
    AptActionPushFalse               = 0x74, // push false
    AptActionPushNULL                = 0x75, // push true
    AptActionPushUndefined           = 0x76, // push false
    AptActionTraceStart              = 0x77, // start of trace
    AptActionGotoFrame               = 0x81,
    AptActionGetUrl                  = 0x83,
    AptActionStoreRegister           = 0x87,
    AptActionDefineDictionary        = 0x88,
    AptActionWaitForFrame            = 0x8a,
    AptActionSetTarget               = 0x8b,
    AptActionGotoLabel               = 0x8c,
    AptActionDefineFunction2         = 0x8e,
    AptActionTry                     = 0x8F, // Added for Flash Player 7 and AS 2.0 support. (Release 17.0)
    AptActionWith                    = 0x94,
    AptActionPush                    = 0x96,
    AptActionBranchAlways            = 0x99,
    AptActionGetUrl2                 = 0x9a,
    AptActionDefineFunction          = 0x9b,
    AptActionBranchIfTrue            = 0x9d,
    AptActionCallFrame               = 0x9e,
    AptActionGotoFrame2              = 0x9f,
    AptActionPushString              = 0xa1, // push string, get var
    AptActionPushStringDictByte      = 0xa2, // push string in dict, byte
    AptActionPushStringDictWord      = 0xa3, // push string in dict, word
    AptActionPushStringGetVar        = 0xa4, // push string, get var
    AptActionPushStringGetMember     = 0xa5, // push string, get member
    AptActionPushStringSetVar        = 0xa6, // push string, set var
    AptActionPushStringSetMember     = 0xa7, // push string, set var
    AptActionStringDictByteGetVar    = 0xae, // push string in dict (byte), and get var
    AptActionStringDictByteGetMember = 0xaf, // push string in dict (byte), and get member
    AptActionDictCallFuncPop         = 0xb0, // dict call func and pop
    AptActionDictCallFuncSetVar      = 0xb1, // dict call func and set var
    AptActionDictCallMethodPop       = 0xb2, // dict call method and pop
    AptActionDictCallMethodSetVar    = 0xb3, // dict call method and set var
    AptActionPushFloat               = 0xb4, // push float
    AptActionPushByte                = 0xb5, // push byte
    AptActionPushWord                = 0xb6, // push word
    AptActionPushDWord               = 0xb7, // push dword
    AptActionBranchIfFalse           = 0xb8, // Branch if false
    AptActionPushRegister            = 0xb9, // Push register added in 2.03.xx

    // If more actions are needed, the Breakpoint action and LastAptAction should move down
    AptActionBreakpoint = 0xba,
    LastAptAction       = 0xbb,
};

struct AptActionInterpreter
{
    friend struct AptAnimation;

    // changed from AptValueStack to AptValuePtrStack
    AptBasePtrStack<AptValue> stack;
    AptValuePtrStack<AptValue> withStack;
    AptValuePtrStack<AptValue> setTargetStack;
    AptValuePtrStack<AptValue> thisStack;
    AptValuePtrStack<AptValue> createdObjectsStack;

    // Removed DumpStack Stack, FrameStack Stack, and regArray
    AptScriptFunctionBase *mpCurrentFunction;

#if defined(APT_DEBUG) // Callfunction stack
    using DebugCallStackInfoT = struct DebugCallStackInfo_t
    {
        // we can remove this version once nowhere it is used
        DebugCallStackInfo_t(const AptNativeString &sFuncName, AptValue *pContext)
        {
            sFunctionName    = sFuncName;
            pFunctionContext = pContext;
            eActionType      = AptActionType_Invalid;
        }
        DebugCallStackInfo_t(const AptNativeString &sFuncName, AptValue *pContext, AptActionType eType)
        {
            sFunctionName    = sFuncName;
            pFunctionContext = pContext;
            eActionType      = eType;
        }
        ~DebugCallStackInfo_t()
        {
            pFunctionContext = NULL;
        }
        AptNativeString sFunctionName;
        AptValue *pFunctionContext;
        AptActionType eActionType;
        APT_NEW_DELETE_OPERATORS
        // Added Metric defines
    };
    AptDebugStack<DebugCallStackInfoT> debugCallStack;
#endif

    //--------------


    mutable AptConstantPool constantPool;

    AptInput input;

    // these will no longer be used now.
    AptValue *apRegisters[4];

    // The following two items were added to keep track of class scope
    int nThisCount;

    // Added for Flash Player 7 and AS 2.0 support (Release 17.0).

  private:
    AptValue *mpThrownValue;
    int mnStackFrameBase;  // index of lowest stack element that is in our frame.
    int mnActiveIntervals; // the number of currently active intervals
  public:
    // used to decide if we want to skip 'trace' bytecode and all bytecodes before 'trace' that create trace string.
    bool mbSkipTraceBytecodes;
    bool bShutDown;
    // used to assign component classes to global space during initialization
    bool bRunningInitActions;

    static bool ENABLE_AGGRESIVE_ZOMBIE_CLEANUP;

    APT_FORCE_INLINE bool hasThrownValue() const
    {
        return (mpThrownValue != NULL);
    }

    /**
        @brief Added to clear out exceptions (should not be any). This prints out a message if there is one along
        with info on how we got there (passed in). These exceptions should be fixed in the AS code!
    */
    void *PrepareForExecution(const char *pszLocationinfo);
    void *PrepareForExecution(AptActionSetup *pActionSetup);

    void CleanupAfterExecution(const char *pszLocationinfo, void *pPassedValue);
    void CleanupAfterExecution(void *pPassedValue, AptActionSetup *pActionSetup);

    APT_FORCE_INLINE bool doUnwindStack() const
    {
        return hasThrownValue();
    }

    APT_FORCE_INLINE AptValue *getThrownValue() const
    {
        return mpThrownValue;
    }

    APT_INLINE void throwValue(AptValue *pThrown)
    {
        APT_INC(pThrown);
        mpThrownValue = pThrown;
    }

    APT_INLINE void clearThrownValue()
    {
        APT_DEC(mpThrownValue);
        mpThrownValue = NULL;
    }

  public:
    // added, so that stack sizes can be changed by user as per their requirement.
    void initialize(const AptInitParams &aptInitParms);
    void shutdown(void);

    /**
        @brief ASK - 0.19.00 for Debugger - added extra AptCharacterInst * pParentCharacter parameter that will hold the parent CharacterInst which actually has the
        bytecode stream that is getting executed in this runstream.
        We can remove this parameter if later we encapsulate all the context stuff into prepareforexecution and if we have a unique way
        to find out from which swf file we are executing the bytecode at the beginning of runstream.
        So while executing bytecode if we hit a breakpoint by some way then we need to find out from which swf file we are executing the bytecode
        and then compare in the breakpoint list.
    */
    const uint8_t *runStream(const uint8_t *aActionStream, AptCIH *pCurrentContext, int nMaxStreamBytes = -1, AptCharacterInst *pParentCharacter = NULL);
    static void resolveStream(unsigned char *aActionStream, unsigned char *pBase, AptConstFile *aConstantFile, intptr_t *pnCurrentConstantIndex);
    static void unresolveStream(unsigned char *aActionStream, unsigned char *pBase, intptr_t *pnCurrentConstantIndex);
    static void valueToObject(AptValue *pCurrentContext, AptValue *pWith, AptValue *pVal, AptValue **ppInst);

    bool setVariable(AptValue *pCurrentContext, AptValue *pWith, const AptNativeString *pVarName, AptValue *pVal, int bGlobal = 1, int bLookInFunctionScope = 1, int bIsMember = 0);
    AptValue *getVariable(AptValue *pCurrentContext, AptValue *pWith, const AptNativeString *pVarName, int bGlobal = 1, int bLookInFunctionScope = 1, int bIsMember = 0);
    static AptValue *getObject(AptValue *pCurrentContext, AptValue *pWith, const AptNativeString *pPathName);
    static bool getContext(AptValue *pCurrentContext, AptValue *pWith, const AptNativeString *pVarName, AptValue **ppContext, AptNativeString &sName);
    static bool getContext(AptValue *pCurrentContext, AptValue *pWith, const AptNativeString *pVarName, AptValue **ppContext, char *szName);
    static void getName(AptCIH *pCIH, AptNativeString &sBuf);
    static void getName2(AptCIH *pCIH, AptNativeString &sBuf);
    static bool isObjectOfType(AptValue *pObject, AptValue *pInterface);
    void callFunction(AptValue *pContext, AptValue *pFuncDef, int nStackParams, AptValue *pOverrideThis = NULL, AptValue *pOverrideSuper = NULL, bool bEnableScopeInfo = false);

    APT_INLINE
    AptValue *stackAt(const int32_t nPos) const;
    APT_INLINE
    AptValue *stackGetPop();

    void stackPushIndirect(AptValue *const pValue);
    APT_INLINE
    void stackPush(AptValue *const pValue);
    APT_INLINE
    void stackPop(void);
    APT_INLINE
    void stackPushNoInc(AptValue *const pValue);
    APT_INLINE
    void stackPopNoDec(void);
    APT_INLINE
    void stackPopAndPush(const int32_t nCountToPop, AptValue *const pValue);
    APT_INLINE
    void stackPop(const int32_t nItems);
    APT_INLINE
    void stackSafePop(const int32_t nItems);

    void loadVariables(AptValue *pContext, AptValue *pWith, const AptNativeString *pURL);

    static AptValue *cbCallMethod_hitTest(AptCIH *pCIH, int nParams);
    static AptValue *cbCallMethod_setInterval(AptValue *pContext, int nParams);
    static AptValue *cbCallMethod_clearInterval(AptValue *pContext, int nParams);
    static AptValue *cbCallMethod_isNaN(AptValue *pContext, int nParams);
    static AptValue *cbCallMethod_unescape(AptValue *pContext, int nParams);
    static AptValue *cbCallMethod_escape(AptValue *pContext, int nParams);
    static AptValue *cbCallMethod_boolean(AptValue *pContext, int nParams);
    static AptValue *cbCallMethod_ASSetPropFlags(AptValue *pContext, int nParams); // Added for Flash Player 7 and AS 2.0 support (Release 17.0).
    static AptValue *cbCallMethod_parseInt(AptValue *pContext, int nParams);
    static AptValue *cbCallMethod_parseFloat(AptValue *pContext, int nParams);

    static AptValue *GetNextProto(AptValue *pObject);
    static bool HasMethodImplementation(AptValue *pObject, const AptNativeString *psFunction);
    static AptValue *FindSuperImplementor(AptValue *pObject, const AptNativeString *psFunction);

    /**
        @brief made public as it was called from duplicatemovieclip.
        also added extra initObject parameter defaulted to NULL to take care of object initializations
    */
    AptValue *_doCloneSprite(AptCIH *pCurrentCIH, AptValue *pWith, AptValue *pSource, AptValue *pTarget, int nDepthInt, AptValue *pInitObject = NULL);

  private:
    const char *urlDecode(const char *szURL, AptNativeString &sKey, AptNativeString &sValue);
    bool isFSCommand(const char *szCommand);
    int doFSCommand(const char *szCommand, const char *szParams);
    //  void _doCloneSprite(AptCIH *pCurrentCIH, AptValue *pWith, AptValue *pSource, AptValue *pTarget, int nDepthInt);
    static void _parseStream(unsigned char *aActionStream, unsigned char *pBase, AptConstFile *aConstantFile, intptr_t *pnCurrentConstantIndex);
#if defined(APT_USE_SOUND_OBJECT)
    AptValue *_constructSoundObject(AptCIH *pParent);
#endif
    AptValue *_createObject(const uint8_t *instruction, AptValue *pCurrentContext, AptValue *pCurWith, const AptNativeString *szObject, int nParams = 0, bool bRunConstructor = true);
    void _doEnumerate(AptValue *pCurrentContext, AptValue *pCurWith);

    struct LocalContextT
    {
        const uint8_t *pInstruction;
        AptCIH *pCurrentContext;
        AptValue *pCurWith;
        unsigned char *pRemoveWithAt;
        AptValue *pSuper;
        bool bEncounteredReturn;
        // ASK added in 0.19.00 for debugger.
        // adding new member so all the bytecode functions can know from which swf file we are executing.
        AptCharacterInst *pParentCharacter;
    };

    //  Functions
    static void _FunctionAptActionEnd(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionNextFrame(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPrevFrame(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPlay(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionToggleQuality(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStopSounds(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionAdd(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionSubtract(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionMultiply(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDivide(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionEquals(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionLessThan(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionAnd(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionOr(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionNot(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStringEquals(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStringLength(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionSubString(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionToInteger(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionGetVariable(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionSetVariable(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionSetTarget2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStringAdd(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionGetProperty(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionSetProperty(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionCloneSprite(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionRemoveSprite(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionTrace(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStartDragMovie(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStopDragMovie(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStringLessThan(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionRandom(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionMBLength(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionCharToAscii(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionAsciiToChar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionGetTimer(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionMBSubString(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionMBCharToAscii(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionMBAsciiToChar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDelete(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDelete2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDefineLocal(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionCallFunction(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionReturn(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionModulo(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionNewObject(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDefineLocal2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionInitArray(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionInitObject(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionTypeOf(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionTargetPath(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionEnumerate(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionAdd2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionLessThan2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionEquals2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionToNumber(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionToString(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushDuplicate(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStackSwap(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionGetMember(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionSetMember(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionIncrement(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDecrement(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionCallMethod(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionNewMethod(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionEnumerate2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionBitAnd(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionBitOr(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionBitXor(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionBitLShift(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionBitRShift(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionBitURShift(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStrictEquals(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionGreater(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionGotoFrame(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionGetUrl(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStoreRegister(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDefineDictionary(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionWaitForFrame(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionSetTarget(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionGotoLabel(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDefineFunction2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionWith(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPush(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionGetUrl2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDefineFunction(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionBranchIfTrue(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionCallFrame(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionGotoFrame2(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionBranchAlways(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);

    //  Optimized opcodes
    static void _FunctionAptActionPushThis(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushGlobal(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPush0(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPush1(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionCallFuncAndPop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionCallFuncSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionCallMethodPop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionCallMethodSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);

    static void _FunctionAptActionPushThisVariable(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushGlobalVariable(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushZeroSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushTrue(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushFalse(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushNULL(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushUndefined(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionTraceStart(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);

    static void _FunctionAptActionPushString(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushStringDictByte(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushStringDictWord(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushStringGetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushStringGetMember(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushStringSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushStringSetMember(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);

    static void _FunctionAptActionStringDictByteGetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionStringDictByteGetMember(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDictCallFuncPop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDictCallFuncSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDictCallMethodPop(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionDictCallMethodSetVar(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);

    static void _FunctionAptActionPushFloat(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushByte(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushWord(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushDWord(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionPushRegister(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);

    static void _FunctionAptActionBranchIfFalse(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);

    // Begin ByteCodes added for Flash Player 7 and AS 2.0 support (Release 17.0).
    static void _FunctionAptActionExtends(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionCastOp(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionImplementsOp(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionTry(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionThrow(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    static void _FunctionAptActionInstanceOf(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);
    // End ByteCodes Added for Flash Player 7 and AS 2.0 support.

    /** @brief Breakpoint handler */
    static void _FunctionAptActionBreakpoint(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);

    using _FunctionAptAction = void (*)(AptActionInterpreter *const pInterpreter, LocalContextT *const pLocalContext);

    struct FunctionTable
    {
#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
        Actions mCheckAlignment;
#endif
        _FunctionAptAction mFunctionPointer;
    };

    static FunctionTable sGlobalTable[LastAptAction];
};

extern AptActionInterpreter gAptActionInterpreter;

// These flags correspond to how Element.pm writes the flags for sprites. The Wheel flag was added for our own purposes,
enum AptEventActionFlag
{
    AptEventActionFlag_Invalid        = 0x0,
    AptEventActionFlag_OnLoad         = 0x01,
    AptEventActionFlag_EnterFrame     = 0x02,
    AptEventActionFlag_Unload         = 0x04,
    AptEventActionFlag_MouseMove      = 0x08,
    AptEventActionFlag_MouseDown      = 0x10,
    AptEventActionFlag_MouseUp        = 0x20,
    AptEventActionFlag_KeyDown        = 0x40,
    AptEventActionFlag_KeyUp          = 0x80,
    AptEventActionFlag_Data           = 0x100,
    AptEventActionFlag_Initialize     = 0x200,
    AptEventActionFlag_Press          = 0x400,
    AptEventActionFlag_Release        = 0x800,
    AptEventActionFlag_ReleaseOutside = 0x1000,
    AptEventActionFlag_RollOver       = 0x2000,
    AptEventActionFlag_RollOut        = 0x4000,
    AptEventActionFlag_DragOver       = 0x8000,
    AptEventActionFlag_DragOut        = 0x10000,
    AptEventActionFlag_KeyPress       = 0x20000,
    AptEventActionFlag_Construct      = 0x40000,
    AptEventActionFlag_Wheel          = 0x80000,
    AptEventActionFlag_KeyEvents      = (AptEventActionFlag_KeyDown |
                                    AptEventActionFlag_KeyUp |
                                    AptEventActionFlag_KeyPress),
    AptEventActionFlag_MouseEvents    = (AptEventActionFlag_MouseMove |
                                      AptEventActionFlag_MouseDown |
                                      AptEventActionFlag_MouseUp |
                                      AptEventActionFlag_Press | // Extra mouse events that AptEventActionFlag_InputUpDown ignores
                                      AptEventActionFlag_Release |
                                      AptEventActionFlag_ReleaseOutside |
                                      AptEventActionFlag_RollOver |
                                      AptEventActionFlag_RollOut |
                                      AptEventActionFlag_DragOver |
                                      AptEventActionFlag_DragOut |
                                      AptEventActionFlag_Wheel),
    AptEventActionFlag_InputEvents    = (AptEventActionFlag_KeyEvents
#if defined(APT_USE_MOUSE)
                                      | AptEventActionFlag_MouseEvents),
#else
                                      ),
#endif
    AptEventActionFlag_AllEvents = (AptEventActionFlag_Data |
                                    AptEventActionFlag_EnterFrame |
                                    AptEventActionFlag_KeyEvents |
                                    AptEventActionFlag_OnLoad |
#if defined(APT_USE_MOUSE)
                                    AptEventActionFlag_MouseEvents |
#endif
                                    AptEventActionFlag_Unload)
};
