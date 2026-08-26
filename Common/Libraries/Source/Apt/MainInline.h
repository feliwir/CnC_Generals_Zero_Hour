/**
 * Pulls in the .inl files with the actual bodies of APT_INLINE-declared functions, when
 * APT_ENABLE_INLINE is set (release builds of the Apt library itself).
 */

#pragma once
#if defined(APT_ENABLE_INLINE)

// AptLibrary.inl is deliberately not listed here; AptLibrary.h pulls it in itself.
#include "AptValue/AptValue.inl"
#include "AptNativeHash.inl"
#include "AptActionInterpreter.inl"
#include "AptValue/AptValueVector.inl"
#include "AptValue/AptString.inl"
#include "AptValue/AptBoolean.inl"
#include "AptObject/AptObject.inl"
#include "AptObject/AptGlobalObject.inl"
#include "AptObject/AptGlobalExtensionObject.inl"
#include "AptObject/AptMath.inl"
#include "string/EAString.inl"

// The .inl files below define members of AptRenderItem, AptCharacterInst (and its
// subclasses) and AptCIH, so the full class definitions must be visible here.
#include "AptCharacterInst.h"

#include "AptRenderItem.inl"
#include "AptCharacterInst.inl"
#include "AptCIH.inl"

#include "Apt.inl"
#endif
