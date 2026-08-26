/** Enums for a pool of pre-defined, reserved strings, avoiding the need to hash them. */

#pragma once
#include "_AptInternalDefines.h"

enum StringCode
{
    SC_FIRST,

    SC___proto__ = SC_FIRST,

    SC__alpha,
    SC__currentframe,
    SC__down,
    SC__droptarget,
    SC__focusrect,
    SC__framesloaded,
    SC__global,
    SC__height,
    SC__highquality,
    SC__left,
    SC__name,
    SC__quality,
    SC__right,
    SC__rotation,
    SC__soundbuftime,
    SC__target,
    SC__totalframes,
    SC__type,
    SC__up,
    SC__url,
    SC__visible,
    SC__width,
    SC__x,
    SC__xmouse,
    SC__xscale,
    SC__y,
    SC__ymouse,
    SC__yscale,
    SC_aa,
    SC_ab,
    SC_Array,
    SC_ba,
    SC_bb,
    SC_Boolean,
    SC_center,
    SC_Color,
    SC_controller, // Only used for analog stick!
    SC_Date,
    SC_Error,
    SC_false,
    SC_Function,
    SC_ga,
    SC_gb,
    SC_getRGB,
    SC_getTransform,
    SC_left,
    SC_LoadVars,
    SC_MovieClip,
    SC_MovieClipLoader,
    SC_none,
    SC_null,
    SC_Number,
    SC_Object,
    SC_onData,
    SC_onDragOut,
    SC_onDragOver,
    SC_onEnterFrame,
    SC_onKeyDown,
    SC_onKeyUp,
    SC_onLoad,
    SC_onMouseDown,
    SC_onMouseMove,
    SC_onMouseUp,
    SC_onMouseWheel, // F#563    10
    SC_onPress,
    SC_onRelease,
    SC_onReleaseOutside,
    SC_onRollOut,
    SC_onRollOver,
    SC_onUnload,
    SC_prototype,
    SC_ra,
    SC_rb,
    SC_right,
    SC_setRGB,
    SC_setTransform,
    SC_Sound,
    SC_String,
    SC_super,
    SC_TextFormat,
    SC_this,
    SC_true,
    SC_Undefined,
    SC_xMax,
    SC_xMin,
    SC_XML,
    SC_yMax,
    SC_yMin,
    SC___New__,

#if defined(APT_3D)
    SC__xrotation,
    SC__yrotation,
    SC__z,
#endif
    SC_filters,
    SC_BlendNormal,
    SC_BlendLayer,
    SC_BlendMultiply,
    SC_BlendScreen,
    SC_BlendLighten,
    SC_BlendDarken,
    SC_BlendDifference,
    SC_BlendAdd,
    SC_BlendSubtract,
    SC_BlendInvert,
    SC_BlendAlpha,
    SC_BlendErase,
    SC_BlendOverlay,
    SC_BlendHardlight,

    SC_DropShadowFilterClass,
    SC_BlurFilterClass,
    SC_GlowFilterClass,
    SC_BevelFilterClass,
    SC_GradientGlowFilterClass,
    SC_ConvolutionFilterClass,
    SC_ColorMatrixFilterClass,
    SC_GradientBevelFilterClass,

    SC_alphas,
    SC_angle,
    SC_bias,
    SC_blurX,
    SC_blurY,
    SC_clamp,
    SC_colors,
    SC_distance,
    SC_divisor,
    SC_filterType,
    SC_full,
    SC_hideObject,
    SC_highlightAlpha,
    SC_highlightColor,
    SC_inner,
    SC_knockout,
    SC_matrix,
    SC_matrixX,
    SC_matrixY,
    SC_outer,
    SC_preserveAlpha,
    SC_quality,
    SC_shadowAlpha,
    SC_shadowColor,
    SC_strength,
    SC_ratios,
    SC_type,
#if defined(APT_USE_DEBUG_NAMES)
    SC__debugName,
#endif
    SC__CustomControlType,

    SC_LAST
};

#include "_Apt.h"
#include "AptString/EAString.h"

class StringPool
{
  public:
    static void Initialize(const int32_t iSize);
    static void Teardown();

    APT_INLINE static const AptNativeString *GetString(const StringCode eSC)
    {
        APT_ASSERT(saConstant[eSC].IsEmpty() == false);
        return (&saConstant[eSC]);
    }

    static AptString *GetFromPool(const char *const pText);
    static void RemoveFromPool(AptString *const pString);

    static void ClearTemporaryPool();

  private:
    //  We won't create this class
    StringPool();
    explicit StringPool(const StringPool &Other);
    ~StringPool();

    StringPool &operator=(const StringPool &Other);

    static const AptNativeString *GetString(const char *const pText);

    static bool CheckContent();

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    static int32_t GetDeltaCreated(AptString *const pString);
    static int32_t GetDeltaDuplicated(AptString *const pString);
#endif

    static AptNativeString saConstant[SC_LAST];
    static AptString **spPool;
    static int32_t spPoolSize;
    static AptString *spFirstFree;

    friend class AptString;
};
