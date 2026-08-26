#pragma once
#include "AptPlatform.h"

enum AptInputType
{
    AptInputType_MouseButton0  = 0,
    AptInputType_Left          = 1,
    AptInputType_Right         = 2,
    AptInputType_Home          = 3,
    AptInputType_End           = 4,
    AptInputType_Insert        = 5,
    AptInputType_Delete        = 6,
    AptInputType_Backspace     = 8,
    AptInputType_Enter         = 13,
    AptInputType_Up            = 14,
    AptInputType_Down          = 15,
    AptInputType_PgUp          = 16,
    AptInputType_PgDn          = 17,
    AptInputType_Tab           = 18,
    AptInputType_Escape        = 19,
    AptInputType_ASCII32       = 32,
    AptInputType_ASCII126      = 126,
    AptInputType_MouseMovement = 200,

    // These are used for Gesture Recognition
    AptInputType_GestureStart     = 204,
    AptInputType_VerticalAngleL   = AptInputType_GestureStart, // 201-203 reserved for input codes added in another version of the package (i.e. voice, handle, pointer)
    AptInputType_VerticalAngleR   = 205,
    AptInputType_HorizontalAngleL = 206,
    AptInputType_HorizontalAngleR = 207,
    AptInputType_VerticalSwipeL   = 208,
    AptInputType_VerticalSwipeR   = 209,
    AptInputType_HorizontalSwipeL = 210,
    AptInputType_HorizontalSwipeR = 211,
    AptInputType_GrabL            = 212,
    AptInputType_GrabR            = 213,
    AptInputType_WaveL            = 214,
    AptInputType_WaveR            = 215,
    AptInputType_GestureEnd       = AptInputType_WaveR,

    // Conversions of the previous codes made by BUTTONACTIONCODE_TO_KEYCODE
    // (: Apt input key codes are inconsistent. User didn't have names for the converted codes)
    AptKeycode_Left      = 37,
    AptKeycode_Right     = 39,
    AptKeycode_Home      = 36,
    AptKeycode_End       = 35,
    AptKeycode_Insert    = 45,
    AptKeycode_Delete    = 46,
    AptKeycode_Backspace = 8,
    AptKeycode_Enter     = 13,
    AptKeycode_Up        = 38,
    AptKeycode_Down      = 40,
    AptKeycode_PgUp      = 33,
    AptKeycode_PgDn      = 34,
    AptKeycode_Tab       = 9,
    AptKeycode_Escape    = 27,

// Key codes from AptInputType_PadSelect to AptInputType_PadR3 can be mapped as per game team requirements. These are not used in APT core.
#if defined(APT_PLATFORM_XBOX)
    AptInputType_PadSelect   = 300, /**< PS2 = Select, NGC = NONE! */
    AptInputType_PadBack     = 300, /**< Xbox = Back, NGC = NONE! */
    AptInputType_PadStart    = 301, /**< All = Start */
    AptInputType_PadCross    = 302, /**< PS2 = Cross */
    AptInputType_PadA        = 302, /**< Xbox/NGC = A */
    AptInputType_PadCircle   = 303, /**< PS2 = Circle */
    AptInputType_PadB        = 303, /**< Xbox/NGC = B */
    AptInputType_PadTriangle = 304, /**< PS2 = Triangle (don't confuse with Cross for PS2!) */
    AptInputType_PadX        = 304, /**< Xbox/NGC = X (don't confuse with Cross for PS2!) */
    AptInputType_PadSquare   = 305, /**< PS2 = Square */
    AptInputType_PadY        = 305, /**< Xbox/NGC = Y */
    AptInputType_PadL1       = 306, /**< PS2 = L1, NGC = NONE! */
    AptInputType_PadBlack    = 306, /**< Xbox = Black, NGC = NONE! */
    AptInputType_PadR1       = 307, /**< PS2 = R1 */
    AptInputType_PadWhite    = 307, /**< Xbox = White */
    AptInputType_PadZTrigger = 307, /**< NGC = Z Trigger */
    AptInputType_PadL2       = 308, /**< PS2 = L2 */
    AptInputType_PadL        = 308, /**< Xbox/NGC = L */
    AptInputType_PadR2       = 309, /**< PS2 = R2 */
    AptInputType_PadR        = 309, /**< Xbox/NGC = R */
    AptInputType_PadL3       = 310,
    AptInputType_PadR3       = 311,
#elif defined(APT_PLATFORM_XBOXONE)
    AptInputType_PadSelect   = 300, /**< PS2 = Select, NGC = NONE! */
    AptInputType_PadBack     = 300, /**< Xbox = Back, NGC = NONE! */
    AptInputType_PadStart    = 301, /**< All = Start */
    AptInputType_PadCross    = 302, /**< PS2 = Cross */
    AptInputType_PadA        = 302, /**< Xbox/NGC = A */
    AptInputType_PadCircle   = 303, /**< PS2 = Circle */
    AptInputType_PadB        = 303, /**< Xbox/NGC = B */
    AptInputType_PadTriangle = 304, /**< PS2 = Triangle (don't confuse with Cross for PS2!) */
    AptInputType_PadX        = 304, /**< Xbox/NGC = X (don't confuse with Cross for PS2!) */
    AptInputType_PadSquare   = 305, /**< PS2 = Square */
    AptInputType_PadY        = 305, /**< Xbox/NGC = Y */
    AptInputType_PadL1       = 306, /**< PS2 = L1, NGC = NONE! */
    AptInputType_PadBlack    = 306, /**< Xbox = Black, NGC = NONE! */
    AptInputType_PadR1       = 307, /**< PS2 = R1 */
    AptInputType_PadWhite    = 307, /**< Xbox = White */
    AptInputType_PadZTrigger = 307, /**< NGC = Z Trigger */
    AptInputType_PadL2       = 308, /**< PS2 = L2 */
    AptInputType_PadL        = 308, /**< Xbox/NGC = L */
    AptInputType_PadR2       = 309, /**< PS2 = R2 */
    AptInputType_PadR        = 309, /**< Xbox/NGC = R */
    AptInputType_PadL3       = 310,
    AptInputType_PadR3       = 311,
#elif defined(APT_PLATFORM_PLAYSTATION2)
    AptInputType_PadSelect   = 300, /**< PS2 = Select, NGC = NONE! */
    AptInputType_PadBack     = 300, /**< Xbox = Back, NGC = NONE! */
    AptInputType_PadStart    = 301, /**< All = Start */
    AptInputType_PadCross    = 302, /**< PS2 = Cross */
    AptInputType_PadA        = 302, /**< Xbox/NGC = A */
    AptInputType_PadCircle   = 303, /**< PS2 = Circle */
    AptInputType_PadB        = 303, /**< Xbox/NGC = B */
    AptInputType_PadTriangle = 304, /**< PS2 = Triangle (don't confuse with Cross for PS2!) */
    AptInputType_PadX        = 304, /**< Xbox/NGC = X (don't confuse with Cross for PS2!) */
    AptInputType_PadSquare   = 305, /**< PS2 = Square */
    AptInputType_PadY        = 305, /**< Xbox/NGC = Y */
    AptInputType_PadL1       = 306, /**< PS2 = L1, NGC = NONE! */
    AptInputType_PadBlack    = 306, /**< Xbox = Black, NGC = NONE! */
    AptInputType_PadR1       = 307, /**< PS2 = R1 */
    AptInputType_PadWhite    = 307, /**< Xbox = White */
    AptInputType_PadZTrigger = 307, /**< NGC = Z Trigger */
    AptInputType_PadL2       = 308, /**< PS2 = L2 */
    AptInputType_PadL        = 308, /**< Xbox/NGC = L */
    AptInputType_PadR2       = 309, /**< PS2 = R2 */
    AptInputType_PadR        = 309, /**< Xbox/NGC = R */
    AptInputType_PadL3       = 310,
    AptInputType_PadR3       = 311,
#elif defined(APT_PLATFORM_WINDOWS) || defined(APT_PLATFORM_LINUX)
    AptInputType_PadSelect   = 300, /**< PS2 = Select, NGC = NONE! */
    AptInputType_PadBack     = 300, /**< Xbox = Back, NGC = NONE! */
    AptInputType_PadStart    = 301, /**< All = Start */
    AptInputType_PadCross    = 302, /**< PS2 = Cross */
    AptInputType_PadA        = 302, /**< Xbox/NGC = A */
    AptInputType_PadCircle   = 303, /**< PS2 = Circle */
    AptInputType_PadB        = 303, /**< Xbox/NGC = B */
    AptInputType_PadTriangle = 304, /**< PS2 = Triangle (don't confuse with Cross for PS2!) */
    AptInputType_PadX        = 304, /**< Xbox/NGC = X (don't confuse with Cross for PS2!) */
    AptInputType_PadSquare   = 305, /**< PS2 = Square */
    AptInputType_PadY        = 305, /**< Xbox/NGC = Y */
    AptInputType_PadL1       = 306, /**< PS2 = L1, NGC = NONE! */
    AptInputType_PadBlack    = 306, /**< Xbox = Black, NGC = NONE! */
    AptInputType_PadR1       = 307, /**< PS2 = R1 */
    AptInputType_PadWhite    = 307, /**< Xbox = White */
    AptInputType_PadZTrigger = 307, /**< NGC = Z Trigger */
    AptInputType_PadL2       = 308, /**< PS2 = L2 */
    AptInputType_PadL        = 308, /**< Xbox/NGC = L */
    AptInputType_PadR2       = 309, /**< PS2 = R2 */
    AptInputType_PadR        = 309, /**< Xbox/NGC = R */
    AptInputType_PadL3       = 310,
    AptInputType_PadR3       = 311,
#elif (defined(APT_PLATFORM_MICROSOFT) && defined(APT_PLATFORM_CONSOLE))
    AptInputType_PadSelect   = 300, /**< PS2 = Select, NGC = NONE! */
    AptInputType_PadBack     = 300, /**< Xbox = Back, NGC = NONE! */
    AptInputType_PadStart    = 301, /**< All = Start */
    AptInputType_PadCross    = 302, /**< PS2 = Cross */
    AptInputType_PadA        = 302, /**< Xbox/NGC = A */
    AptInputType_PadCircle   = 303, /**< PS2 = Circle */
    AptInputType_PadB        = 303, /**< Xbox/NGC = B */
    AptInputType_PadTriangle = 304, /**< PS2 = Triangle (don't confuse with Cross for PS2!) */
    AptInputType_PadX        = 304, /**< Xbox/NGC = X (don't confuse with Cross for PS2!) */
    AptInputType_PadSquare   = 305, /**< PS2 = Square */
    AptInputType_PadY        = 305, /**< Xbox/NGC = Y */
    AptInputType_PadL1       = 306, /**< PS2 = L1, NGC = NONE! */
    AptInputType_PadBlack    = 306, /**< Xbox = Black, NGC = NONE! */
    AptInputType_PadR1       = 307, /**< PS2 = R1 */
    AptInputType_PadWhite    = 307, /**< Xbox = White */
    AptInputType_PadZTrigger = 307, /**< NGC = Z Trigger */
    AptInputType_PadL2       = 308, /**< PS2 = L2 */
    AptInputType_PadL        = 308, /**< Xbox/NGC = L */
    AptInputType_PadR2       = 309, /**< PS2 = R2 */
    AptInputType_PadR        = 309, /**< Xbox/NGC = R */
    AptInputType_PadL3       = 310,
    AptInputType_PadR3       = 311,
#elif defined(APT_PLATFORM_PSP)
    AptInputType_PadSelect   = 300, /**< PS2 = Select, NGC = NONE! */
    AptInputType_PadBack     = 300, /**< Xbox = Back, NGC = NONE! */
    AptInputType_PadStart    = 301, /**< All = Start */
    AptInputType_PadCross    = 302, /**< PS2 = Cross */
    AptInputType_PadA        = 302, /**< Xbox/NGC = A */
    AptInputType_PadCircle   = 303, /**< PS2 = Circle */
    AptInputType_PadB        = 303, /**< Xbox/NGC = B */
    AptInputType_PadTriangle = 304, /**< PS2 = Triangle (don't confuse with Cross for PS2!) */
    AptInputType_PadX        = 304, /**< Xbox/NGC = X (don't confuse with Cross for PS2!) */
    AptInputType_PadSquare   = 305, /**< PS2 = Square */
    AptInputType_PadY        = 305, /**< Xbox/NGC = Y */
    AptInputType_PadL1       = 306, /**< PS2 = L1, NGC = NONE! */
    AptInputType_PadBlack    = 306, /**< Xbox = Black, NGC = NONE! */
    AptInputType_PadR1       = 307, /**< PS2 = R1 */
    AptInputType_PadWhite    = 307, /**< Xbox = White */
    AptInputType_PadZTrigger = 307, /**< NGC = Z Trigger */
    AptInputType_PadL2       = 308, /**< PS2 = L2 */
    AptInputType_PadL        = 308, /**< Xbox/NGC = L */
    AptInputType_PadR2       = 309, /**< PS2 = R2 */
    AptInputType_PadR        = 309, /**< Xbox/NGC = R */
    AptInputType_PadL3       = 310,
    AptInputType_PadR3       = 311,
#elif (defined(APT_PLATFORM_SONY) && defined(APT_PLATFORM_CONSOLE)) || defined(APT_PLATFORM_PS3)
    AptInputType_PadSelect   = 300, /**< PS2 = Select, NGC = NONE! */
    AptInputType_PadBack     = 300, /**< Xbox = Back, NGC = NONE! */
    AptInputType_PadStart    = 301, /**< All = Start */
    AptInputType_PadCross    = 302, /**< PS2 = Cross */
    AptInputType_PadA        = 302, /**< Xbox/NGC = A */
    AptInputType_PadCircle   = 303, /**< PS2 = Circle */
    AptInputType_PadB        = 303, /**< Xbox/NGC = B */
    AptInputType_PadTriangle = 304, /**< PS2 = Triangle (don't confuse with Cross for PS2!) */
    AptInputType_PadX        = 304, /**< Xbox/NGC = X (don't confuse with Cross for PS2!) */
    AptInputType_PadSquare   = 305, /**< PS2 = Square */
    AptInputType_PadY        = 305, /**< Xbox/NGC = Y */
    AptInputType_PadL1       = 306, /**< PS2 = L1, NGC = NONE! */
    AptInputType_PadBlack    = 306, /**< Xbox = Black, NGC = NONE! */
    AptInputType_PadR1       = 307, /**< PS2 = R1 */
    AptInputType_PadWhite    = 307, /**< Xbox = White */
    AptInputType_PadZTrigger = 307, /**< NGC = Z Trigger */
    AptInputType_PadL2       = 308, /**< PS2 = L2 */
    AptInputType_PadL        = 308, /**< Xbox/NGC = L */
    AptInputType_PadR2       = 309, /**< PS2 = R2 */
    AptInputType_PadR        = 309, /**< Xbox/NGC = R */
    AptInputType_PadL3       = 310,
    AptInputType_PadR3       = 311,
    AptInputType_PadMove     = 312, /**< PS3 Move button on motion controller */
    AptInputType_PadT        = 313, /**< PS3 T trigger on motion controller */
#else
#error undefined platform

#endif
    // End of key code change list
    AptInputType_Connected        = 400,
    AptInputType_Disconnected     = 401,
    AptInputType_LeftAnalogStick  = 501, /**< Actionscript should call Key.getAnalogStickInfo() to get details of the analog input. */
    AptInputType_RightAnalogStick = 502, /**< Actionscript should call Key.getAnalogStickInfo() to get details of the analog input. */
    AptInputType_NumInputs

};
