#pragma once
const unsigned int APT_SCARY_MAX_NATIVE_STRING_LENGTH = 256; //
const unsigned int APT_REFCOUNT_INF                   = 100; //

#include "AptGC.h"
#include "AptValue/AptValue.h"

enum AptProceduralProperty
{
    AptProceduralProperty_X,
    AptProceduralProperty_Y,
    AptProceduralProperty_XScale,
    AptProceduralProperty_YScale,
    AptProceduralProperty_Width,
    AptProceduralProperty_Height,
    AptProceduralProperty_Rotation,
    AptProceduralProperty_Alpha,
    AptProceduralProperty_TR,
    AptProceduralProperty_TG,
    AptProceduralProperty_TB,
    AptProceduralProperty_Visible,
#if defined(APT_3D)
    AptProceduralProperty_Z,
    AptProceduralProperty_ZScale,
    AptProceduralProperty_YRot,
    AptProceduralProperty_XRot,
#endif
    AptProceduralProperty_NUMBER
};

enum AptCharacterButtonRecordState
{
    AptCharacterButtonRecordState_None    = 0,
    AptCharacterButtonRecordState_Up      = 1,
    AptCharacterButtonRecordState_Over    = 2,
    AptCharacterButtonRecordState_Down    = 4,
    AptCharacterButtonRecordState_HitTest = 8,
};

using AptInput = unsigned int;
extern AptInput gNullInput;

#define UNDEF_OK true
#include "AptValue/AptCIH.h"

#include "AptValue/AptString.h"
#include "AptValue/AptInteger.h"
#include "AptValue/AptBoolean.h"
#include "AptValue/AptFloat.h"

#include "AptValue/AptRegister.h"
#include "AptValue/AptLookup.h"

// #define CHECK_OBJECT_ACTION_FLAGS 1
#include "AptObject/AptObject.h"
#include "AptValue/AptArray.h"
#include "AptNativeHash.h"
#include "AptObject/AptNativeFunction.h"

struct AptAction_DefineFunction;
struct AptCharacter;
class AptGlobal;
class AptError;

#include "AptObject/AptScriptFunction.h"

class AptCharacterInst;

extern AptMatrix gIdentityMatrix;
extern AptCXForm gIdentityCXForm;
extern AptNativeFunction *gpCBsetInterval;
extern AptNativeFunction *gpCBclearInterval;
extern AptNativeFunction *gpCBisNaN;
extern AptNativeFunction *gpCBunescape;
extern AptNativeFunction *gpCBescape;
extern AptNativeFunction *gpCBboolean;
extern AptNativeFunction *gpASSetPropFlags; // Added for Flash Player 7 / AS 2.0 support (Release 17.0); undocumented Flash function.
extern AptNativeFunction *gpCBparseInt;
extern AptNativeFunction *gpCBparseFloat;
#if APT_USE_MATH_OBJECT
extern AptMathObj *gpGlobalMathObject;
#endif
extern AptKey *gpGlobalKeyObject;
#if defined(APT_USE_MOUSE)
extern AptMouse *gpGlobalMouseObject;
#endif
extern AptNone *gpUndefinedValue;
extern AptCIHNone *gpUndefinedCIH; // Global Undefined AptCIH for reference replacement.
extern AptPrototype *gpObjectPrototype;
extern AptPrototype *gpFunctionPrototype;
extern AptValue *gpGlobalObjectPrototype;
extern AptValue *gpGlobalMovieclipPrototype;
#if defined(APT_USE_STAGE_OBJECT)
extern AptStage *gpGlobalStageObject;
#endif
extern AptError *gpGlobalErrorObject; // Added for Flash Player 7 / AS 2.0 support (Release 17.0).

#if defined(APT_ALTERNATE_INPUT)
extern AptAlternateInput *gpGlobalAltInputObject;
#endif

void AptValueInitialize(void);
void AptValueShutdown(int bQuiet = 0);
void AptValueShutdownRemaining();
