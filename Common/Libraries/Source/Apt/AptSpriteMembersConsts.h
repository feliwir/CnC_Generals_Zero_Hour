#pragma once

#include "AptSpriteMembers.h"

/**
 * Constants related to the sprites. NOTE: the numerical values for these constants must match the
 * corresponding numerical values in sprite.gperf.
 */

static const int AptPropertyNumber_x                 = 1;
static const int AptPropertyNumber_y                 = 2;
static const int AptPropertyNumber_xscale            = 3;
static const int AptPropertyNumber_yscale            = 4;
static const int AptPropertyNumber_currentframe      = 5;
static const int AptPropertyNumber_totalframes       = 6;
static const int AptPropertyNumber_alpha             = 7;
static const int AptPropertyNumber_visible           = 8;
static const int AptPropertyNumber_width             = 9;
static const int AptPropertyNumber_height            = 10;
static const int AptPropertyNumber_rotation          = 11;
static const int AptPropertyNumber_target            = 12;
static const int AptPropertyNumber_framesloaded      = 13;
static const int AptPropertyNumber_name              = 14;
static const int AptPropertyNumber_droptarget        = 15;
static const int AptPropertyNumber_url               = 16;
static const int AptPropertyNumber_highquality       = 17;
static const int AptPropertyNumber_focusrect         = 18;
static const int AptPropertyNumber_soundbuftime      = 19;
static const int AptPropertyNumber_quality           = 20;
static const int AptPropertyNumber_xmouse            = 21;
static const int AptPropertyNumber_ymouse            = 22;
static const int AptPropertyNumberextern             = 23;
static const int AptPropertyNumberisNaN              = 26;
static const int AptPropertyNumberunescape           = 27;
static const int AptPropertyNumberescape             = 28;
static const int AptPropertyNumber_boolean           = 29;
static const int AptPropertyNumber_blendMode         = 30;
static const int AptPropertyNumber_filters           = 31;
static const int AptPropertyNumber_parseInt          = 32;
static const int AptPropertyNumber_parseFloat        = 33;
static const int AptPropertyNumber_parent            = 34;
static const int AptPropertyNumber_skipeval          = 35;
static const int AptPropertyNumber_CustomControlType = 36;

static const int AptTextPropertyautoSize          = 1;
static const int AptTextPropertybackground        = 2;
static const int AptTextPropertybackgroundColor   = 3;
static const int AptTextPropertyborder            = 4;
static const int AptTextPropertyborderColor       = 5;
static const int AptTextPropertyhscroll           = 6;
static const int AptTextPropertylength            = 7;
static const int AptTextPropertymaxChars          = 8;
static const int AptTextPropertymaxscroll         = 9;
static const int AptTextPropertymultiline         = 10;
static const int AptTextPropertyscroll            = 11;
static const int AptTextPropertytext              = 12;
static const int AptTextPropertytextColor         = 13;
static const int AptTextPropertytextHeight        = 14;
static const int AptTextPropertytextWidth         = 15;
static const int AptTextPropertytype              = 16;
static const int AptTextPropertyvariable          = 17;
static const int AptTextPropertywordWrap          = 18;
static const int AptTextProperty_height           = 19;
static const int AptTextProperty_width            = 20;
static const int AptTextPropertymouseWheelEnabled = 21;

static const int AptSpriteMethod_attachMovie          = 100;
static const int AptSpriteMethod_duplicateMovieClip   = 101;
static const int AptSpriteMethod_getURL               = 102;
static const int AptSpriteMethod_gotoAndPlay          = 103;
static const int AptSpriteMethod_gotoAndStop          = 104;
static const int AptSpriteMethod_loadMovie            = 105;
static const int AptSpriteMethod_loadVariables        = 106;
static const int AptSpriteMethod_play                 = 107;
static const int AptSpriteMethod_prevFrame            = 108;
static const int AptSpriteMethod_removeMovieClip      = 109;
static const int AptSpriteMethod_stop                 = 110;
static const int AptSpriteMethod_getBytesLoaded       = 111;
static const int AptSpriteMethod_getBytesTotal        = 112;
static const int AptSpriteMethod_nextFrame            = 113;
static const int AptSpriteMethod_createTextField      = 114;
static const int AptSpriteMethod_getDepth             = 115;
static const int AptSpriteMethod_createEmptyMovieClip = 116;
static const int AptSpriteMethod_getBounds            = 117;
static const int AptSpriteMethod_hitTest              = 118;
static const int AptSpriteMethod_removeTextField      = 119;
static const int AptSpriteMethod_swapDepths           = 120;
static const int AptSpriteMethod_unloadMovie          = 121;
static const int AptSpriteMethod_setMask              = 122;
static const int AptSpriteMethod_getNewTextFormat     = 123;
static const int AptSpriteMethod_getTextFormat        = 124;
static const int AptSpriteMethod_setTextFormat        = 125;
static const int AptSpriteMethod_startDrag            = 126;
static const int AptSpriteMethod_localToGlobal        = 127;
#if defined(APT_3D)
static const int AptPropertyNumber_z         = 128;
static const int AptPropertyNumber_xrotation = 129;
static const int AptPropertyNumber_yrotation = 130;
#endif
#if defined(APT_RENDER_FLAGS)
static const int AptSpriteMethod__renderflags = 131;
#endif
static const int AptSpriteMethod_globalToLocal  = 132;
static const int AptSpriteMethod_loadMovieNum   = 133;
static const int AptSpriteMethod_unloadMovieNum = 134;
#if defined(APT_3D)
static const int AptPropertyNumber_zscale = 135;
#endif
static const int AptSpriteMethod_getInstanceAtDepth = 136;

// WARNING: 200 is hardcoded in some places as the start of indices into
// the below aSpriteGperfToActionFlag. Please do not add properties above
// 200 or some code will break!
static const int AptSpriteMethod_onData           = 200;
static const int AptSpriteMethod_onDragOut        = 201;
static const int AptSpriteMethod_onDragOver       = 202;
static const int AptSpriteMethod_onEnterFrame     = 203;
static const int AptSpriteMethod_onKeyDown        = 204;
static const int AptSpriteMethod_onKeyUp          = 205;
static const int AptSpriteMethod_onKillFocus      = 206;
static const int AptSpriteMethod_onLoad           = 207;
static const int AptSpriteMethod_onMouseDown      = 208;
static const int AptSpriteMethod_onMouseMove      = 209;
static const int AptSpriteMethod_onMouseUp        = 210;
static const int AptSpriteMethod_onPress          = 211;
static const int AptSpriteMethod_onRelease        = 212;
static const int AptSpriteMethod_onReleaseOutside = 213;
static const int AptSpriteMethod_onRollOut        = 214;
static const int AptSpriteMethod_onRollOver       = 215;
static const int AptSpriteMethod_onSetFocus       = 216;
static const int AptSpriteMethod_onUnload         = 217;
static const int AptSpriteMethod_onMouseWheel     = 218;
// WARNING: I do not recommend adding properties after 218 here, see comments above!

static const int aSpriteGperfToActionFlag[] =
    {
        AptEventActionFlag_Data,           // static const int AptSpriteMethod_onData = 200;
        AptEventActionFlag_DragOut,        // static const int AptSpriteMethod_onDragOut = 201;
        AptEventActionFlag_DragOver,       // static const int AptSpriteMethod_onDragOver = 202;
        AptEventActionFlag_EnterFrame,     // static const int AptSpriteMethod_onEnterFrame = 203;
        AptEventActionFlag_KeyDown,        // static const int AptSpriteMethod_onKeyDown = 204;
        AptEventActionFlag_KeyUp,          // static const int AptSpriteMethod_onKeyUp = 205;
        -1,                                // static const int AptSpriteMethod_onKillFocus = 206;
        AptEventActionFlag_OnLoad,         // static const int AptSpriteMethod_onLoad = 207;
        AptEventActionFlag_MouseDown,      // static const int AptSpriteMethod_onMouseDown = 208;
        AptEventActionFlag_MouseMove,      // static const int AptSpriteMethod_onMouseMove = 209;
        AptEventActionFlag_MouseUp,        // static const int AptSpriteMethod_onMouseUp = 210;
        AptEventActionFlag_Press,          // static const int AptSpriteMethod_onPress = 211;
        AptEventActionFlag_Release,        // static const int AptSpriteMethod_onRelease = 212;
        AptEventActionFlag_ReleaseOutside, // static const int AptSpriteMethod_onReleaseOutside = 213;
        AptEventActionFlag_RollOut,        // static const int AptSpriteMethod_onRollOut = 214;
        AptEventActionFlag_RollOver,       // static const int AptSpriteMethod_onRollOver = 215;
        -1,                                // static const int AptSpriteMethod_onSetFocus = 216;
        AptEventActionFlag_Unload,         // static const int AptSpriteMethod_onUnload = 217;
        AptEventActionFlag_Wheel,          // static const int AptSpriteMethod_onMouseWheel = 218;
};
