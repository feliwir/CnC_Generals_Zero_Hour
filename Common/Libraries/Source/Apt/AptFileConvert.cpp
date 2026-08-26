/**
 * Converts .apt/.const blobs built by a 32-bit swfc into native 64-bit layout.
 *
 * The walk below must mirror exactly what the resolve pipeline touches:
 *   - AptCharacterAnimation::Fixup   (AptAnimation.cpp)
 *   - AptMovie::resolve              (AptMovie.cpp)
 *   - AptFilter::Resolve             (AptObject/AptFilter.cpp)
 *   - AptActionInterpreter::_parseStream (AptActionInterpreter.cpp)
 * If a pointer field is added to any serialized struct, it must be converted here
 * (the layout static_asserts in _AptFileConvert.h will catch struct changes).
 *
 * The output is *unresolved* data: pointer slots hold offsets, character indices
 * and magic values exactly as a 64-bit swfc would have written them, so the
 * regular Resolve/Fixup/Unresolve pipeline runs on the converted blobs unchanged.
 */

#include "_Apt.h"
#include "AptCharacter/AptCharacter.h"
#include "AptObject/AptFilter.h"
#include "_AptFileConvert.h"

#if APT_PLATFORM_PTR_SIZE == 8

namespace
{

const uint64_t kParentAnimMagic    = 0x09876543;
const int kFuncPoolItemsMagic      = 0x98765432;
const uint64_t kFuncPoolArrayMagic = 0x12345678;

// mnFilterID values, see the comment in AptObject/AptFilter.h
enum
{
    FilterID_DropShadow    = 0,
    FilterID_Blur          = 1,
    FilterID_Glow          = 2,
    FilterID_Bevel         = 3,
    FilterID_GradientGlow  = 4,
    FilterID_ColorMatrix   = 6,
    FilterID_GradientBevel = 7,
};

/** Growable byte buffer holding offsets only, so reallocation is safe. */
// The action interpreter aligns inline structs on absolute addresses, so a
// converted blob only works if its base is pointer-aligned. The Dogma allocator
// behind APT_MALLOC_BLOCK guarantees DOGMA_ALLOCATION_GRANULARITY alignment.
static_assert(DOGMA_ALLOCATION_GRANULARITY >= 8,
              "converted .apt blobs need 8-byte-aligned allocations");

struct ConvertBuffer
{
    uint8_t *pData;
    size_t nSize;
    size_t nCap;

    void Init()
    {
        pData = NULL;
        nSize = 0;
        nCap  = 0;
    }

    void Destroy()
    {
        if (pData)
        {
            APT_FREE_BLOCK(pData, nCap);
        }
        Init();
    }

    bool Reserve(size_t nWanted)
    {
        if (nWanted <= nCap)
        {
            return true;
        }
        size_t nNewCap = nCap ? nCap : 4096;
        while (nNewCap < nWanted)
        {
            nNewCap *= 2;
        }
        uint8_t *pNew = (uint8_t *)APT_MALLOC_BLOCK(nNewCap);
        if (!pNew || ((uintptr_t)pNew & 7) != 0)
        {
            return false;
        }
        if (nSize)
        {
            memcpy(pNew, pData, nSize);
        }
        if (pData)
        {
            APT_FREE_BLOCK(pData, nCap);
        }
        pData = pNew;
        nCap  = nNewCap;
        return true;
    }

    /** Appends zeroed bytes and returns their offset, or 0 on failure (offset 0 is never handed out for real data). */
    uint64_t Alloc(size_t nBytes, size_t nAlign)
    {
        size_t nOff = (nSize + (nAlign - 1)) & ~(nAlign - 1);
        if (!Reserve(nOff + nBytes))
        {
            return (uint64_t)-1;
        }
        memset(pData + nSize, 0, nOff + nBytes - nSize);
        nSize = nOff + nBytes;
        return (uint64_t)nOff;
    }

    void PutBytes(const void *p, size_t n)
    {
        if (Reserve(nSize + n))
        {
            memcpy(pData + nSize, p, n);
            nSize += n;
        }
    }

    void PutU8(uint8_t v)
    {
        PutBytes(&v, 1);
    }

    void AlignTo(size_t nAlign)
    {
        size_t nOff = (nSize + (nAlign - 1)) & ~(nAlign - 1);
        if (Reserve(nOff))
        {
            memset(pData + nSize, 0, nOff - nSize);
            nSize = nOff;
        }
    }

    template <class T>
    T *At(uint64_t nOff)
    {
        return (T *)(pData + nOff);
    }
};

struct Converter
{
    const uint8_t *pSrc; // base of the 32-bit .apt blob
    ConvertBuffer arena; // the native-layout .apt blob under construction
    uint32_t nRootOldOff;
    uint64_t nRootNewOff;
    bool bFail;

    void Fail(const char *szMsg)
    {
        if (!bFail)
        {
            APT_DEBUGPRINT(APT_DEBUG_MSG_WARNING_LVL, "AptConvertFile32: %s\n", szMsg);
        }
        bFail = true;
    }

    template <class T>
    const T *SrcAt(uint32_t nOff) const
    {
        return (const T *)(pSrc + nOff);
    }
};

uint64_t ArenaAlloc(Converter &c, size_t nBytes, size_t nAlign)
{
    uint64_t nOff = c.arena.Alloc(nBytes, nAlign);
    if (nOff == (uint64_t)-1)
    {
        c.Fail("out of memory");
        return 0;
    }
    return nOff;
}

uint64_t CopyString(Converter &c, uint32_t nSrcOff)
{
    if (!nSrcOff)
    {
        return 0;
    }
    const char *sz = c.SrcAt<char>(nSrcOff);
    size_t nLen    = strlen(sz) + 1;
    uint64_t nNew  = ArenaAlloc(c, nLen, 1);
    if (nNew)
    {
        memcpy(c.arena.At<char>(nNew), sz, nLen);
    }
    return nNew;
}

/** Copies a raw array (floats, shorts, glyph entries, ...) whose layout is pointer-size independent. */
uint64_t CopyRawArray(Converter &c, uint32_t nSrcOff, size_t nBytes, size_t nAlign)
{
    if (!nSrcOff || nBytes == 0)
    {
        return 0;
    }
    uint64_t nNew = ArenaAlloc(c, nBytes, nAlign);
    if (nNew)
    {
        memcpy(c.arena.At<uint8_t>(nNew), c.pSrc + nSrcOff, nBytes);
    }
    return nNew;
}

/** Widens an array of 4-byte pointer slots (holding values, not offsets) to 8-byte slots. */
uint64_t WidenSlotArray(Converter &c, uint32_t nSrcOff, int nCount)
{
    if (!nSrcOff)
    {
        return 0;
    }
    uint64_t nNew = ArenaAlloc(c, (size_t)nCount * 8, 8);
    if (!nNew)
    {
        return 0;
    }
    const uint32_t *pOld = c.SrcAt<uint32_t>(nSrcOff);
    for (int i = 0; i < nCount; i++)
    {
        *c.arena.At<uint64_t>(nNew + (uint64_t)i * 8) = (uint64_t)pOld[i];
    }
    return nNew;
}

// ---------------------------------------------------------------------------
// Action stream transcoding
// ---------------------------------------------------------------------------

/**
 * Instruction-boundary map and patch list for one action stream. Branch targets,
 * with-block ends and function/try code sizes are byte quantities relative to the
 * stream, and the inline action structs change size and alignment between 32-bit
 * and 64-bit layout, so all of them must be recomputed from an old-offset ->
 * new-offset map once the whole stream has been transcoded.
 */
struct StreamContext
{
    enum PatchKind
    {
        Patch_BranchDelta,  // int32 delta relative to the post-struct instruction pointer
        Patch_WithEnd,      // pointer-slot delta relative to the post-struct instruction pointer
        Patch_FuncCodeSize, // int32 count of body bytes following the struct
        Patch_TrySizes,     // three consecutive uint32 block sizes following the struct
    };

    struct MapEntry
    {
        uint32_t nOldRel;
        uint32_t nNewRel;
    };

    struct Patch
    {
        PatchKind eKind;
        uint32_t nOldAfter;  // old stream-relative offset just past the aligned struct
        uint32_t nNewAfter;  // new stream-relative offset just past the aligned struct
        uint32_t nFieldRel;  // new stream-relative offset of the field(s) to patch
        uint32_t v0, v1, v2; // old values (delta / sizes)
    };

    ConvertBuffer mapBuf;
    ConvertBuffer patchBuf;
    int nMapEntries;
    int nPatches;

    void Init()
    {
        mapBuf.Init();
        patchBuf.Init();
        nMapEntries = 0;
        nPatches    = 0;
    }

    void Destroy()
    {
        mapBuf.Destroy();
        patchBuf.Destroy();
    }

    void AddBoundary(uint32_t nOldRel, uint32_t nNewRel)
    {
        MapEntry e = {nOldRel, nNewRel};
        mapBuf.PutBytes(&e, sizeof(e));
        nMapEntries++;
    }

    void AddPatch(const Patch &p)
    {
        patchBuf.PutBytes(&p, sizeof(p));
        nPatches++;
    }

    /** Maps an old stream-relative offset to the new one; boundaries are recorded in increasing order. */
    bool Lookup(uint32_t nOldRel, uint32_t *pnNewRel)
    {
        const MapEntry *a = mapBuf.At<MapEntry>(0);
        int lo = 0, hi = nMapEntries - 1;
        while (lo <= hi)
        {
            int mid = (lo + hi) / 2;
            if (a[mid].nOldRel == nOldRel)
            {
                *pnNewRel = a[mid].nNewRel;
                return true;
            }
            if (a[mid].nOldRel < nOldRel)
            {
                lo = mid + 1;
            }
            else
            {
                hi = mid - 1;
            }
        }
        return false;
    }
};

uint64_t ConvertEventActionSet(Converter &c, uint32_t nSrcOff);

/**
 * Transcodes one action bytecode stream, mirroring the opcode dispatch of
 * AptActionInterpreter::_parseStream. Inline structs are re-emitted in native
 * layout (source aligned to 4 bytes, destination to 8), raw immediate payloads
 * are copied verbatim, and strings/arrays referenced from inline structs are
 * copied into the arena. Returns the arena offset of the new stream.
 */
uint64_t ConvertActionStream(Converter &c, uint32_t nSrcOff)
{
    if (!nSrcOff || c.bFail)
    {
        return 0;
    }

    StreamContext ctx;
    ctx.Init();
    ConvertBuffer out;
    out.Init();

    uint32_t nSrcAbs = nSrcOff; // absolute offset in the source blob; the 32-bit
                                // builder aligned inline structs on absolute
                                // positions, i.e. on blob-relative offsets

    for (;;)
    {
        if (c.bFail)
        {
            break;
        }

        ctx.AddBoundary(nSrcAbs - nSrcOff, (uint32_t)out.nSize);

        uint8_t nOp = *c.SrcAt<uint8_t>(nSrcAbs);
        nSrcAbs++;
        out.PutU8(nOp);

        if (nOp == AptActionEnd)
        {
            ctx.AddBoundary(nSrcAbs - nSrcOff, (uint32_t)out.nSize);
            break;
        }
        if (nOp >= LastAptAction)
        {
            c.Fail("unknown action opcode");
            break;
        }

        switch (nOp)
        {
        // one raw byte of immediate data
        case AptActionDictCallFuncPop:
        case AptActionDictCallFuncSetVar:
        case AptActionDictCallMethodPop:
        case AptActionDictCallMethodSetVar:
        case AptActionPushByte:
        case AptActionStringDictByteGetVar:
        case AptActionStringDictByteGetMember:
        case AptActionPushStringDictByte:
        case AptActionPushRegister:
        {
            out.PutBytes(c.SrcAt<uint8_t>(nSrcAbs), 1);
            nSrcAbs += 1;
            break;
        }

        // two raw bytes
        case AptActionPushWord:
        case AptActionPushStringDictWord:
        {
            out.PutBytes(c.SrcAt<uint8_t>(nSrcAbs), 2);
            nSrcAbs += 2;
            break;
        }

        // four raw bytes
        case AptActionPushDWord:
        case AptActionPushFloat:
        case AptActionTraceStart:
        {
            out.PutBytes(c.SrcAt<uint8_t>(nSrcAbs), 4);
            nSrcAbs += 4;
            break;
        }

        case AptActionPushString:
        case AptActionPushStringGetVar:
        case AptActionPushStringGetMember:
        case AptActionPushStringSetVar:
        case AptActionPushStringSetMember:
        {
            nSrcAbs                                     = (nSrcAbs + 3) & ~3u;
            const AptAction_PushStringT<uint32_t> *pOld = c.SrcAt<AptAction_PushStringT<uint32_t>>(nSrcAbs);
            nSrcAbs += sizeof(*pOld);

            AptAction_PushString oNew;
            memset(&oNew, 0, sizeof(oNew));
            oNew.szStringToBePushed = (char *)(uintptr_t)CopyString(c, pOld->szStringToBePushed.value);
            out.AlignTo(8);
            out.PutBytes(&oNew, sizeof(oNew));
            break;
        }

        case AptActionPush:
        case AptActionDefineDictionary:
        {
            nSrcAbs                               = (nSrcAbs + 3) & ~3u;
            const AptAction_PushT<uint32_t> *pOld = c.SrcAt<AptAction_PushT<uint32_t>>(nSrcAbs);
            nSrcAbs += sizeof(*pOld);

            // apItems is an out-of-line array of pointer slots holding constant-table
            // indices; widen the values
            AptAction_Push oNew;
            memset(&oNew, 0, sizeof(oNew));
            oNew.items.nItems  = pOld->items.nItems;
            oNew.items.apItems = (AptValue **)(uintptr_t)WidenSlotArray(c, pOld->items.apItems.value, pOld->items.nItems);
            out.AlignTo(8);
            out.PutBytes(&oNew, sizeof(oNew));
            break;
        }

        case AptActionBranchIfTrue:
        case AptActionBranchAlways:
        case AptActionBranchIfFalse:
        {
            nSrcAbs                             = (nSrcAbs + 3) & ~3u;
            const AptAction_BranchAddress *pOld = c.SrcAt<AptAction_BranchAddress>(nSrcAbs);
            nSrcAbs += sizeof(*pOld);

            out.AlignTo(8);
            StreamContext::Patch p;
            p.eKind     = StreamContext::Patch_BranchDelta;
            p.nFieldRel = (uint32_t)out.nSize;
            p.v0        = (uint32_t)pOld->nTargetDelta;
            p.v1 = p.v2                  = 0;
            AptAction_BranchAddress oNew = {0};
            out.PutBytes(&oNew, sizeof(oNew));
            p.nOldAfter = nSrcAbs - nSrcOff;
            p.nNewAfter = (uint32_t)out.nSize;
            ctx.AddPatch(p);
            break;
        }

        case AptActionGetUrl:
        {
            nSrcAbs                                 = (nSrcAbs + 3) & ~3u;
            const AptAction_GetUrlT<uint32_t> *pOld = c.SrcAt<AptAction_GetUrlT<uint32_t>>(nSrcAbs);
            nSrcAbs += sizeof(*pOld);

            AptAction_GetUrl oNew;
            memset(&oNew, 0, sizeof(oNew));
            oNew.szUrl = (char *)(uintptr_t)CopyString(c, pOld->szUrl.value);
            oNew.szWin = (char *)(uintptr_t)CopyString(c, pOld->szWin.value);
            out.AlignTo(8);
            out.PutBytes(&oNew, sizeof(oNew));
            break;
        }

        case AptActionGotoFrame:
        case AptActionGotoFrame2:
        case AptActionStoreRegister:
        {
            // 4-byte pointer-free structs, identical in both layouts
            nSrcAbs = (nSrcAbs + 3) & ~3u;
            out.AlignTo(8);
            out.PutBytes(c.SrcAt<uint8_t>(nSrcAbs), 4);
            nSrcAbs += 4;
            break;
        }

        case AptActionSetTarget:
        {
            nSrcAbs                                    = (nSrcAbs + 3) & ~3u;
            const AptAction_SetTargetT<uint32_t> *pOld = c.SrcAt<AptAction_SetTargetT<uint32_t>>(nSrcAbs);
            nSrcAbs += sizeof(*pOld);

            AptAction_SetTarget oNew;
            memset(&oNew, 0, sizeof(oNew));
            oNew.szTarget = (char *)(uintptr_t)CopyString(c, pOld->szTarget.value);
            out.AlignTo(8);
            out.PutBytes(&oNew, sizeof(oNew));
            break;
        }

        case AptActionGotoLabel:
        {
            nSrcAbs                                    = (nSrcAbs + 3) & ~3u;
            const AptAction_GotoLabelT<uint32_t> *pOld = c.SrcAt<AptAction_GotoLabelT<uint32_t>>(nSrcAbs);
            nSrcAbs += sizeof(*pOld);

            AptAction_GotoLabel oNew;
            memset(&oNew, 0, sizeof(oNew));
            oNew.szLabel = (char *)(uintptr_t)CopyString(c, pOld->szLabel.value);
            out.AlignTo(8);
            out.PutBytes(&oNew, sizeof(oNew));
            break;
        }

        case AptActionDefineFunction:
        {
            nSrcAbs                                         = (nSrcAbs + 3) & ~3u;
            const AptAction_DefineFunctionT<uint32_t> *pOld = c.SrcAt<AptAction_DefineFunctionT<uint32_t>>(nSrcAbs);
            nSrcAbs += sizeof(*pOld);

            // parameter list: array of char* slots, each referencing a string
            uint64_t nParamsNew = 0;
            if (pOld->aszParams.value)
            {
                nParamsNew                 = ArenaAlloc(c, (size_t)pOld->nParams * 8, 8);
                const uint32_t *pOldParams = c.SrcAt<uint32_t>(pOld->aszParams.value);
                for (int i = 0; i < pOld->nParams && !c.bFail; i++)
                {
                    *c.arena.At<uint64_t>(nParamsNew + (uint64_t)i * 8) = CopyString(c, pOldParams[i]);
                }
            }

            AptAction_DefineFunction oNew;
            memset(&oNew, 0, sizeof(oNew));
            oNew.szName               = (const char *)(uintptr_t)CopyString(c, pOld->szName.value);
            oNew.nParams              = pOld->nParams;
            oNew.aszParams            = (char **)(uintptr_t)nParamsNew;
            oNew.nCodeSize            = 0; // patched below
            oNew.constantPool.nItems  = kFuncPoolItemsMagic;
            oNew.constantPool.apItems = (AptValue **)(uintptr_t)kFuncPoolArrayMagic;

            out.AlignTo(8);
            StreamContext::Patch p;
            p.eKind     = StreamContext::Patch_FuncCodeSize;
            p.nFieldRel = (uint32_t)(out.nSize + offsetof(AptAction_DefineFunction, nCodeSize));
            p.v0        = (uint32_t)pOld->nCodeSize;
            p.v1 = p.v2 = 0;
            out.PutBytes(&oNew, sizeof(oNew));
            p.nOldAfter = nSrcAbs - nSrcOff;
            p.nNewAfter = (uint32_t)out.nSize;
            ctx.AddPatch(p);
            break;
        }

        case AptActionDefineFunction2:
        {
            nSrcAbs                                          = (nSrcAbs + 3) & ~3u;
            const AptAction_DefineFunction2T<uint32_t> *pOld = c.SrcAt<AptAction_DefineFunction2T<uint32_t>>(nSrcAbs);
            nSrcAbs += sizeof(*pOld);

            // parameter list: array of AptRegisterParam, 8 bytes each in 32-bit layout
            uint64_t nParamsNew = 0;
            if (pOld->aszParams.value)
            {
                nParamsNew = ArenaAlloc(c, (size_t)pOld->nParams * sizeof(AptRegisterParam), 8);
                for (int i = 0; i < pOld->nParams && !c.bFail; i++)
                {
                    const AptRegisterParamT<uint32_t> *pOldParam =
                        c.SrcAt<AptRegisterParamT<uint32_t>>(pOld->aszParams.value + (uint32_t)(i * sizeof(AptRegisterParamT<uint32_t>)));
                    uint64_t nName              = CopyString(c, pOldParam->szParamName.value);
                    AptRegisterParam *pNewParam = c.arena.At<AptRegisterParam>(nParamsNew + i * sizeof(AptRegisterParam));
                    pNewParam->nRegister        = pOldParam->nRegister;
                    pNewParam->szParamName      = (char *)(uintptr_t)nName;
                }
            }

            AptAction_DefineFunction2 oNew;
            memset(&oNew, 0, sizeof(oNew));
            oNew.szName               = (const char *)(uintptr_t)CopyString(c, pOld->szName.value);
            oNew.nParams              = pOld->nParams;
            oNew.nRegisterCount       = pOld->nRegisterCount;
            oNew.nFlags               = pOld->nFlags;
            oNew.aszParams            = (AptRegisterParam *)(uintptr_t)nParamsNew;
            oNew.nCodeSize            = 0; // patched below
            oNew.constantPool.nItems  = kFuncPoolItemsMagic;
            oNew.constantPool.apItems = (AptValue **)(uintptr_t)kFuncPoolArrayMagic;

            out.AlignTo(8);
            StreamContext::Patch p;
            p.eKind     = StreamContext::Patch_FuncCodeSize;
            p.nFieldRel = (uint32_t)(out.nSize + offsetof(AptAction_DefineFunction2, nCodeSize));
            p.v0        = (uint32_t)pOld->nCodeSize;
            p.v1 = p.v2 = 0;
            out.PutBytes(&oNew, sizeof(oNew));
            p.nOldAfter = nSrcAbs - nSrcOff;
            p.nNewAfter = (uint32_t)out.nSize;
            ctx.AddPatch(p);
            break;
        }

        case AptActionWith:
        {
            nSrcAbs                               = (nSrcAbs + 3) & ~3u;
            const AptAction_WithT<uint32_t> *pOld = c.SrcAt<AptAction_WithT<uint32_t>>(nSrcAbs);
            nSrcAbs += sizeof(*pOld);

            out.AlignTo(8);
            StreamContext::Patch p;
            p.eKind     = StreamContext::Patch_WithEnd;
            p.nFieldRel = (uint32_t)out.nSize;
            p.v0        = pOld->pEnd.value; // delta relative to the post-struct instruction pointer
            p.v1 = p.v2 = 0;
            AptAction_With oNew;
            memset(&oNew, 0, sizeof(oNew));
            out.PutBytes(&oNew, sizeof(oNew));
            p.nOldAfter = nSrcAbs - nSrcOff;
            p.nNewAfter = (uint32_t)out.nSize;
            ctx.AddPatch(p);
            break;
        }

        case AptActionTry:
        {
            nSrcAbs                                               = (nSrcAbs + 3) & ~3u;
            const AptAction_TryCatchFinallyBlockT<uint32_t> *pOld = c.SrcAt<AptAction_TryCatchFinallyBlockT<uint32_t>>(nSrcAbs);
            nSrcAbs += sizeof(*pOld);

            AptAction_TryCatchFinallyBlock oNew;
            memset(&oNew, 0, sizeof(oNew));
            // the uFlags/uAlignment1/uAlignment2/uCaughtRegister byte block sits at
            // offset 12 in both layouts (uFlags is private, so copy it by position)
            memcpy((uint8_t *)&oNew + 12, (const uint8_t *)pOld + 12, 4);
            if (!pOld->putCaughtObjectInRegister())
            {
                oNew.szCaughtVarName = (char *)(uintptr_t)CopyString(c, pOld->szCaughtVarName.value);
            }
            else
            {
                oNew.szCaughtVarName = (char *)(uintptr_t)pOld->szCaughtVarName.value;
            }

            out.AlignTo(8);
            StreamContext::Patch p;
            p.eKind     = StreamContext::Patch_TrySizes;
            p.nFieldRel = (uint32_t)out.nSize; // uTry/uCatch/uFinallyCodeSize are the first three fields
            p.v0        = pOld->uTryCodeSize;
            p.v1        = pOld->uCatchCodeSize;
            p.v2        = pOld->uFinallyCodeSize;
            out.PutBytes(&oNew, sizeof(oNew));
            p.nOldAfter = nSrcAbs - nSrcOff;
            p.nNewAfter = (uint32_t)out.nSize;
            ctx.AddPatch(p);
            break;
        }

        default:
            // every other opcode carries no payload (see _parseStream)
            break;
        }
    }

    uint64_t nStreamNew = 0;
    if (!c.bFail)
    {
        nStreamNew = ArenaAlloc(c, out.nSize, 8);
        if (nStreamNew)
        {
            memcpy(c.arena.At<uint8_t>(nStreamNew), out.pData, out.nSize);
        }
    }

    // apply the delta/size patches now that every instruction boundary is known
    for (int i = 0; i < ctx.nPatches && !c.bFail; i++)
    {
        const StreamContext::Patch *p = ctx.patchBuf.At<StreamContext::Patch>((uint64_t)i * sizeof(StreamContext::Patch));
        uint32_t nNewTarget           = 0;
        switch (p->eKind)
        {
        case StreamContext::Patch_BranchDelta:
        case StreamContext::Patch_WithEnd:
        {
            uint32_t nOldTarget = p->nOldAfter + (uint32_t)(int32_t)p->v0;
            if (!ctx.Lookup(nOldTarget, &nNewTarget))
            {
                c.Fail("branch target is not an instruction boundary");
                break;
            }
            int32_t nNewDelta = (int32_t)(nNewTarget - p->nNewAfter);
            if (p->eKind == StreamContext::Patch_BranchDelta)
            {
                *c.arena.At<int32_t>(nStreamNew + p->nFieldRel) = nNewDelta;
            }
            else
            {
                *c.arena.At<uint64_t>(nStreamNew + p->nFieldRel) = (uint64_t)(int64_t)nNewDelta;
            }
            break;
        }
        case StreamContext::Patch_FuncCodeSize:
        {
            if (!ctx.Lookup(p->nOldAfter + p->v0, &nNewTarget))
            {
                c.Fail("function body end is not an instruction boundary");
                break;
            }
            *c.arena.At<int32_t>(nStreamNew + p->nFieldRel) = (int32_t)(nNewTarget - p->nNewAfter);
            break;
        }
        case StreamContext::Patch_TrySizes:
        {
            uint32_t nTryEnd, nCatchEnd, nFinallyEnd;
            if (!ctx.Lookup(p->nOldAfter + p->v0, &nTryEnd) ||
                !ctx.Lookup(p->nOldAfter + p->v0 + p->v1, &nCatchEnd) ||
                !ctx.Lookup(p->nOldAfter + p->v0 + p->v1 + p->v2, &nFinallyEnd))
            {
                c.Fail("try/catch/finally block end is not an instruction boundary");
                break;
            }
            uint32_t *pSizes = c.arena.At<uint32_t>(nStreamNew + p->nFieldRel);
            pSizes[0]        = nTryEnd - p->nNewAfter;
            pSizes[1]        = nCatchEnd - nTryEnd;
            pSizes[2]        = nFinallyEnd - nCatchEnd;
            break;
        }
        }
    }

    out.Destroy();
    ctx.Destroy();
    return c.bFail ? 0 : nStreamNew;
}

// ---------------------------------------------------------------------------
// Structural walk
// ---------------------------------------------------------------------------

uint64_t ConvertEventActionSet(Converter &c, uint32_t nSrcOff)
{
    if (!nSrcOff || c.bFail)
    {
        return 0;
    }
    const AptEventActionSetT<uint32_t> *pOld = c.SrcAt<AptEventActionSetT<uint32_t>>(nSrcOff);
    int nBlocks                              = pOld->nEventActions;
    uint32_t nBlocksOld                      = pOld->aEventActions.value;

    uint64_t nSetNew    = ArenaAlloc(c, sizeof(AptEventActionSet), 8);
    uint64_t nBlocksNew = nBlocksOld ? ArenaAlloc(c, (size_t)nBlocks * sizeof(AptEventActionBlock), 8) : 0;

    for (int i = 0; i < nBlocks && !c.bFail; i++)
    {
        const AptEventActionBlockT<uint32_t> *pOldBlock =
            c.SrcAt<AptEventActionBlockT<uint32_t>>(nBlocksOld + (uint32_t)(i * sizeof(AptEventActionBlockT<uint32_t>)));
        uint64_t nStream = ConvertActionStream(c, pOldBlock->actions.aActionStream.value);

        AptEventActionBlock *pNewBlock   = c.arena.At<AptEventActionBlock>(nBlocksNew + (uint64_t)i * sizeof(AptEventActionBlock));
        pNewBlock->nTriggers             = pOldBlock->nTriggers;
        pNewBlock->nKeyCode              = pOldBlock->nKeyCode;
        pNewBlock->actions.aActionStream = (unsigned char *)(uintptr_t)nStream;
    }

    AptEventActionSet *pNewSet = c.arena.At<AptEventActionSet>(nSetNew);
    pNewSet->nEventActions     = nBlocks;
    pNewSet->aEventActions     = (AptEventActionBlock *)(uintptr_t)nBlocksNew;
    return nSetNew;
}

uint64_t ConvertFilter(Converter &c, uint32_t nSrcOff)
{
    if (!nSrcOff || c.bFail)
    {
        return 0;
    }
    uint32_t nFilterID = *c.SrcAt<uint32_t>(nSrcOff);
    switch (nFilterID)
    {
    case FilterID_GradientGlow:
    case FilterID_GradientBevel:
    {
        const AptFilterGradientGlowT<uint32_t> *pOld = c.SrcAt<AptFilterGradientGlowT<uint32_t>>(nSrcOff);
        uint64_t nColors                             = CopyRawArray(c, pOld->mpGradColors.value, (size_t)pOld->mnNumColors * sizeof(uint32_t), 4);
        uint64_t nRatios                             = CopyRawArray(c, pOld->mpGradRatios.value, pOld->mnNumColors, 1);
        uint64_t nNew                                = ArenaAlloc(c, sizeof(AptFilterGradientGlow), 8);
        AptFilterGradientGlow *pNew                  = c.arena.At<AptFilterGradientGlow>(nNew);
        pNew->mnFilterID                             = pOld->mnFilterID;
        pNew->mnNumColors                            = pOld->mnNumColors;
        pNew->mpGradColors                           = (uint32_t *)(uintptr_t)nColors;
        pNew->mpGradRatios                           = (uint8_t *)(uintptr_t)nRatios;
        pNew->mnBlurX                                = pOld->mnBlurX;
        pNew->mnBlurY                                = pOld->mnBlurY;
        pNew->mnAngle                                = pOld->mnAngle;
        pNew->mnDistance                             = pOld->mnDistance;
        pNew->mnStrength                             = pOld->mnStrength;
        pNew->mnFlags                                = pOld->mnFlags;
        return nNew;
    }
    // the remaining filter structs contain no pointers, so their layout is
    // pointer-size independent and they can be copied verbatim
    case FilterID_DropShadow:
        return CopyRawArray(c, nSrcOff, sizeof(AptFilterDropShadow), 8);
    case FilterID_Blur:
        return CopyRawArray(c, nSrcOff, sizeof(AptFilterBlur), 8);
    case FilterID_Glow:
        return CopyRawArray(c, nSrcOff, sizeof(AptFilterGlow), 8);
    case FilterID_Bevel:
        return CopyRawArray(c, nSrcOff, sizeof(AptFilterBevel), 8);
    case FilterID_ColorMatrix:
        return CopyRawArray(c, nSrcOff, sizeof(AptFilterColorMatrix), 8);
    default:
        c.Fail("unknown filter id");
        return 0;
    }
}

uint64_t ConvertControl(Converter &c, uint32_t nSrcOff)
{
    if (!nSrcOff || c.bFail)
    {
        return 0;
    }
    const AptControlT<uint32_t> *pOld = c.SrcAt<AptControlT<uint32_t>>(nSrcOff);
    uint64_t nNewOff                  = ArenaAlloc(c, sizeof(AptControl), 8);

    switch (pOld->eType)
    {
    case AptControlType_DoAction:
    {
        uint64_t nStream                   = ConvertActionStream(c, pOld->action.actions.aActionStream.value);
        AptControl *pNew                   = c.arena.At<AptControl>(nNewOff);
        pNew->action.actions.aActionStream = (unsigned char *)(uintptr_t)nStream;
        break;
    }
    case AptControlType_DoInitAction:
    {
        uint64_t nStream                       = ConvertActionStream(c, pOld->initAction.actions.aActionStream.value);
        AptControl *pNew                       = c.arena.At<AptControl>(nNewOff);
        pNew->initAction.nSpriteID             = pOld->initAction.nSpriteID;
        pNew->initAction.actions.aActionStream = (unsigned char *)(uintptr_t)nStream;
        break;
    }
    case AptControlType_FrameLabel:
    {
        uint64_t nLabel          = CopyString(c, pOld->frameLabel.szLabel.value);
        AptControl *pNew         = c.arena.At<AptControl>(nNewOff);
        pNew->frameLabel.szLabel = (char *)(uintptr_t)nLabel;
        break;
    }
    case AptControlType_PlaceObject2:
    case AptControlType_PlaceObject3:
    {
        const bool bPO3 = (pOld->eType == AptControlType_PlaceObject3);
        // AptControlPlaceObject3 extends AptControlPlaceObject2's field list, and
        // the mirror guarantees matching prefixes, so read through placeObject3
        // and only touch the PO3-only fields when bPO3
        uint64_t nName       = CopyString(c, pOld->placeObject3.szName.value);
        uint64_t nActions    = ConvertEventActionSet(c, pOld->placeObject3.pActions.value);
        uint64_t nFilters    = 0;
        uint32_t nNumFilters = 0;
        if (bPO3)
        {
            nNumFilters = pOld->placeObject3.nNumFilters;
            if (pOld->placeObject3.ppFilters.value)
            {
                nFilters                  = ArenaAlloc(c, (size_t)nNumFilters * 8, 8);
                const uint32_t *pOldSlots = c.SrcAt<uint32_t>(pOld->placeObject3.ppFilters.value);
                for (uint32_t k = 0; k < nNumFilters && !c.bFail; k++)
                {
                    uint64_t nFilter                                  = ConvertFilter(c, pOldSlots[k]);
                    *c.arena.At<uint64_t>(nFilters + (uint64_t)k * 8) = nFilter;
                }
            }
        }

        AptControl *pNew            = c.arena.At<AptControl>(nNewOff);
        AptControlPlaceObject3 *pPO = &pNew->placeObject3;
        pPO->eFlags                 = pOld->placeObject3.eFlags;
        pPO->nDepth                 = pOld->placeObject3.nDepth;
        pPO->nCharacterID           = pOld->placeObject3.nCharacterID;
        pPO->matrix                 = pOld->placeObject3.matrix;
        pPO->ncxform                = pOld->placeObject3.ncxform;
        pPO->fRatio                 = pOld->placeObject3.fRatio;
        pPO->szName                 = (char *)(uintptr_t)nName;
        pPO->nClipDepth             = pOld->placeObject3.nClipDepth;
        pPO->pActions               = (AptEventActionSet *)(uintptr_t)nActions;
        if (bPO3)
        {
            pPO->nBlendMode  = pOld->placeObject3.nBlendMode;
            pPO->nNumFilters = nNumFilters;
            pPO->ppFilters   = (AptFilter **)(uintptr_t)nFilters;
        }
        break;
    }
    case AptControlType_RemoveObject2:
    {
        c.arena.At<AptControl>(nNewOff)->removeObject2.nDepth = pOld->removeObject2.nDepth;
        break;
    }
    case AptControlType_BackgroundColour:
    {
        c.arena.At<AptControl>(nNewOff)->backgroundColour.nColour = pOld->backgroundColour.nColour;
        break;
    }
#if defined(APT_USE_SOUND_OBJECT)
    case AptControlType_StartSound:
    {
        c.arena.At<AptControl>(nNewOff)->startSound.nID = pOld->startSound.nID;
        break;
    }
    case AptControlType_StartSoundStream:
    {
        c.arena.At<AptControl>(nNewOff)->startSoundStream.nID = pOld->startSoundStream.nID;
        break;
    }
#endif
    default:
    {
        c.Fail("unknown control type");
        break;
    }
    }

    c.arena.At<AptControl>(nNewOff)->eType = pOld->eType;
    return nNewOff;
}

/** Converts the frame/control tree of a movie; mirrors AptMovie::resolve. */
void ConvertMovie(Converter &c, const AptMovieT<uint32_t> *pOldMovie, uint64_t nCharNewOff, size_t nMovieFieldOff)
{
    int nFrames         = pOldMovie->nFrames;
    uint32_t nFramesOld = pOldMovie->aFrames.value;
    uint64_t nFramesNew = nFramesOld ? ArenaAlloc(c, (size_t)nFrames * sizeof(AptFrame), 8) : 0;

    for (int i = 0; i < nFrames && !c.bFail; i++)
    {
        const AptFrameT<uint32_t> *pOldFrame =
            c.SrcAt<AptFrameT<uint32_t>>(nFramesOld + (uint32_t)(i * sizeof(AptFrameT<uint32_t>)));
        int nControls      = pOldFrame->nControls;
        uint32_t nSlotsOld = pOldFrame->apControls.value;
        uint64_t nSlotsNew = nSlotsOld ? ArenaAlloc(c, (size_t)nControls * 8, 8) : 0;
        for (int j = 0; j < nControls && !c.bFail; j++)
        {
            uint64_t nControl                                  = ConvertControl(c, c.SrcAt<uint32_t>(nSlotsOld)[j]);
            *c.arena.At<uint64_t>(nSlotsNew + (uint64_t)j * 8) = nControl;
        }
        AptFrame *pNewFrame   = c.arena.At<AptFrame>(nFramesNew + (uint64_t)i * sizeof(AptFrame));
        pNewFrame->nControls  = nControls;
        pNewFrame->apControls = (AptControl **)(uintptr_t)nSlotsNew;
    }

    AptMovie *pNewMovie = (AptMovie *)(c.arena.At<uint8_t>(nCharNewOff) + nMovieFieldOff);
    pNewMovie->nFrames  = nFrames;
    pNewMovie->aFrames  = (AptFrame *)(uintptr_t)nFramesNew;
    pNewMovie->phLabels = NULL; // constructed at resolve time, dead in the file
}

/** Converts one character node; mirrors the per-type switch in AptCharacterAnimation::Fixup. */
void FillCharacter(Converter &c, const AptCharacterT<uint32_t> *pOld, uint64_t nNewOff)
{
    {
        AptCharacter *pNew = c.arena.At<AptCharacter>(nNewOff);
        pNew->eType        = pOld->eType;
        pNew->pParentAnim  = (AptCharacter *)(uintptr_t)pOld->pParentAnim.value;
        APT_ASSERT(pOld->pParentAnim.value == kParentAnimMagic);
    }

    switch (pOld->eType)
    {
    case AptCharacterType_Shape:
    {
        AptCharacter *pNew  = c.arena.At<AptCharacter>(nNewOff);
        pNew->shape.rBounds = pOld->shape.rBounds;
        // holds the character index, not an offset
        pNew->shape.pRenderUnit = (AptRenderableGeometry *)(uintptr_t)pOld->shape.pRenderUnit.value;
        break;
    }
    case AptCharacterType_Text:
    {
        uint64_t nInitialText    = CopyString(c, pOld->text.szInitialText.value);
        uint64_t nVariable       = CopyString(c, pOld->text.szVariable.value);
        AptCharacter *pNew       = c.arena.At<AptCharacter>(nNewOff);
        pNew->text.rBounds       = pOld->text.rBounds;
        pNew->text.nFontID       = pOld->text.nFontID;
        pNew->text.eAlignment    = pOld->text.eAlignment;
        pNew->text.nColour       = pOld->text.nColour;
        pNew->text.fFontHeight   = pOld->text.fFontHeight;
        pNew->text.bReadOnly     = pOld->text.bReadOnly;
        pNew->text.bMultiLine    = pOld->text.bMultiLine;
        pNew->text.bWordWrap     = pOld->text.bWordWrap;
        pNew->text.szInitialText = (char *)(uintptr_t)nInitialText;
        pNew->text.szVariable    = (char *)(uintptr_t)nVariable;
        break;
    }
    case AptCharacterType_Font:
    {
        uint64_t nName = CopyString(c, pOld->font.szName.value);
        // glyph slots hold character indices, not offsets
        uint64_t nGlyphs    = WidenSlotArray(c, pOld->font.apGlyphs.value, pOld->font.nGlyphs);
        AptCharacter *pNew  = c.arena.At<AptCharacter>(nNewOff);
        pNew->font.szName   = (char *)(uintptr_t)nName;
        pNew->font.nGlyphs  = pOld->font.nGlyphs;
        pNew->font.apGlyphs = (AptCharacter **)(uintptr_t)nGlyphs;
        break;
    }
#if defined(APT_USE_BUTTONS)
    case AptCharacterType_Button:
    {
        const AptCharacterButtonT<uint32_t> &oldBtn = pOld->button;
        uint64_t nVertexTable                       = CopyRawArray(c, oldBtn.mHitTestVertexTable.value,
                                                                   (size_t)oldBtn.mHitTestVertexCount * 2 * sizeof(float), 8);
        uint64_t nIndexTable                        = CopyRawArray(c, oldBtn.mHitTestIndexTable.value,
                                                                   (size_t)oldBtn.mHitTestTriangles * 3 * sizeof(short), 8);

        uint64_t nRecords = 0;
        if (oldBtn.aButtonRecords.value)
        {
            nRecords = ArenaAlloc(c, (size_t)oldBtn.nButtonRecords * sizeof(AptCharacterButtonRecord), 8);
            for (int j = 0; j < oldBtn.nButtonRecords; j++)
            {
                const AptCharacterButtonRecordT<uint32_t> *pOldRec =
                    c.SrcAt<AptCharacterButtonRecordT<uint32_t>>(oldBtn.aButtonRecords.value + (uint32_t)(j * sizeof(AptCharacterButtonRecordT<uint32_t>)));
                AptCharacterButtonRecord *pNewRec =
                    c.arena.At<AptCharacterButtonRecord>(nRecords + (uint64_t)j * sizeof(AptCharacterButtonRecord));
                pNewRec->eStates = pOldRec->eStates;
                // holds a character index, not an offset
                pNewRec->pCharacter = (AptCharacter *)(uintptr_t)pOldRec->pCharacter.value;
                pNewRec->nLayer     = pOldRec->nLayer;
                pNewRec->matrix     = pOldRec->matrix;
                pNewRec->cxform     = pOldRec->cxform;
            }
        }

        uint64_t nConditions = 0;
        if (oldBtn.aActionConditions.value)
        {
            nConditions = ArenaAlloc(c, (size_t)oldBtn.nActionConditions * sizeof(AptActionConditionBlock), 8);
            for (int j = 0; j < oldBtn.nActionConditions && !c.bFail; j++)
            {
                const AptActionConditionBlockT<uint32_t> *pOldCond =
                    c.SrcAt<AptActionConditionBlockT<uint32_t>>(oldBtn.aActionConditions.value + (uint32_t)(j * sizeof(AptActionConditionBlockT<uint32_t>)));
                uint64_t nStream = ConvertActionStream(c, pOldCond->actions.aActionStream.value);
                AptActionConditionBlock *pNewCond =
                    c.arena.At<AptActionConditionBlock>(nConditions + (uint64_t)j * sizeof(AptActionConditionBlock));
                pNewCond->nConditions           = pOldCond->nConditions;
                pNewCond->actions.aActionStream = (unsigned char *)(uintptr_t)nStream;
            }
        }

        uint64_t nSound = 0;
        if (oldBtn.pButtonSound.value)
        {
            const AptCharacterButtonSoundT<uint32_t> *pOldSound =
                c.SrcAt<AptCharacterButtonSoundT<uint32_t>>(oldBtn.pButtonSound.value);
            nSound                             = ArenaAlloc(c, sizeof(AptCharacterButtonSound), 8);
            AptCharacterButtonSound *pNewSound = c.arena.At<AptCharacterButtonSound>(nSound);
            // all four hold character indices, not offsets
            pNewSound->pOverUpToIdle     = (AptCharacter *)(uintptr_t)pOldSound->pOverUpToIdle.value;
            pNewSound->pIdleToOverUp     = (AptCharacter *)(uintptr_t)pOldSound->pIdleToOverUp.value;
            pNewSound->pOverUpToOverDown = (AptCharacter *)(uintptr_t)pOldSound->pOverUpToOverDown.value;
            pNewSound->pOverDownToOverUp = (AptCharacter *)(uintptr_t)pOldSound->pOverDownToOverUp.value;
        }

        AptCharacter *pNew                = c.arena.At<AptCharacter>(nNewOff);
        pNew->button.bIsMenu              = oldBtn.bIsMenu;
        pNew->button.mHitTestBoundingRect = oldBtn.mHitTestBoundingRect;
        pNew->button.mHitTestTriangles    = oldBtn.mHitTestTriangles;
        pNew->button.mHitTestVertexCount  = oldBtn.mHitTestVertexCount;
        pNew->button.mHitTestVertexTable  = (float *)(uintptr_t)nVertexTable;
        pNew->button.mHitTestIndexTable   = (short *)(uintptr_t)nIndexTable;
        pNew->button.nButtonRecords       = oldBtn.nButtonRecords;
        pNew->button.aButtonRecords       = (AptCharacterButtonRecord *)(uintptr_t)nRecords;
        pNew->button.nActionConditions    = oldBtn.nActionConditions;
        pNew->button.aActionConditions    = (AptActionConditionBlock *)(uintptr_t)nConditions;
        pNew->button.pButtonSound         = (AptCharacterButtonSound *)(uintptr_t)nSound;
        break;
    }
#endif
    case AptCharacterType_Sprite:
    {
        ConvertMovie(c, &pOld->sprite.movie, nNewOff, offsetof(AptCharacter, sprite.movie));
        break;
    }
#if defined(APT_USE_SOUND_OBJECT)
    case AptCharacterType_Sound:
    {
        // holds the character index, not an offset
        c.arena.At<AptCharacter>(nNewOff)->sound.zID = (AptAssetSound)(uintptr_t)pOld->sound.zID.value;
        break;
    }
#endif
    case AptCharacterType_Bitmap:
    {
        // holds the character index, not an offset
        c.arena.At<AptCharacter>(nNewOff)->bitmap.zID = (AptAssetTexture)(uintptr_t)pOld->bitmap.zID.value;
        break;
    }
    case AptCharacterType_Morph:
    {
        // both hold character indices (mapped by AptCharacterAnimation::Link)
        AptCharacter *pNew          = c.arena.At<AptCharacter>(nNewOff);
        pNew->morph.pStartCharacter = (AptCharacter *)(uintptr_t)pOld->morph.pStartCharacter.value;
        pNew->morph.pEndCharacter   = (AptCharacter *)(uintptr_t)pOld->morph.pEndCharacter.value;
        break;
    }
    case AptCharacterType_StaticText:
    {
        const AptCharacterStaticTextT<uint32_t> &oldST = pOld->statictext;
        uint64_t nRecords                              = 0;
        if (oldST.aRecords.value)
        {
            nRecords = ArenaAlloc(c, (size_t)oldST.nFontRecords * sizeof(AptCharacterStaticTextRecords), 8);
            for (int j = 0; j < oldST.nFontRecords; j++)
            {
                const AptCharacterStaticTextRecordsT<uint32_t> *pOldRec =
                    c.SrcAt<AptCharacterStaticTextRecordsT<uint32_t>>(oldST.aRecords.value + (uint32_t)(j * sizeof(AptCharacterStaticTextRecordsT<uint32_t>)));
                uint64_t nGlyphs = CopyRawArray(c, pOldRec->aGlyphs.value,
                                                (size_t)pOldRec->nGlyphs * sizeof(AptCharacterGlyphEntry), 8);
                AptCharacterStaticTextRecords *pNewRec =
                    c.arena.At<AptCharacterStaticTextRecords>(nRecords + (uint64_t)j * sizeof(AptCharacterStaticTextRecords));
                pNewRec->nFontID  = pOldRec->nFontID;
                pNewRec->cxform   = pOldRec->cxform;
                pNewRec->fXOffset = pOldRec->fXOffset;
                pNewRec->fYOffset = pOldRec->fYOffset;
                pNewRec->fScale   = pOldRec->fScale;
                pNewRec->nGlyphs  = pOldRec->nGlyphs;
                pNewRec->aGlyphs  = (AptCharacterGlyphEntry *)(uintptr_t)nGlyphs;
            }
        }
        AptCharacter *pNew            = c.arena.At<AptCharacter>(nNewOff);
        pNew->statictext.rBounds      = oldST.rBounds;
        pNew->statictext.matrix       = oldST.matrix;
        pNew->statictext.nFontRecords = oldST.nFontRecords;
        pNew->statictext.aRecords     = (AptCharacterStaticTextRecords *)(uintptr_t)nRecords;
        break;
    }
    case AptCharacterType_Video:
    case AptCharacterType_None:
    {
        // no payload (see AptCharacterAnimation::Fixup)
        break;
    }
    default:
    {
        c.Fail("unexpected character type");
        break;
    }
    }
}

/** Converts the root animation character; mirrors AptCharacterAnimation::Fixup's top level. */
void ConvertRoot(Converter &c)
{
    const AptCharacterT<uint32_t> *pOldRoot = c.SrcAt<AptCharacterT<uint32_t>>(c.nRootOldOff);
    if (pOldRoot->eType != AptCharacterType_Animation)
    {
        c.Fail("main character is not an animation");
        return;
    }
    const AptCharacterAnimationT<uint32_t> &oldAnim = pOldRoot->animation;

    c.nRootNewOff = ArenaAlloc(c, sizeof(AptCharacter), 8);

    // characters
    int nCharacters    = oldAnim.nCharacters;
    uint64_t nSlotsNew = ArenaAlloc(c, (size_t)nCharacters * 8, 8);
    for (int i = 0; i < nCharacters && !c.bFail; i++)
    {
        uint32_t nOldChar = c.SrcAt<uint32_t>(oldAnim.apCharacters.value)[i];
        uint64_t nNewChar;
        if (nOldChar == 0)
        {
            nNewChar = 0; // imported characters are linked at load time
        }
        else if (nOldChar == c.nRootOldOff)
        {
            nNewChar = c.nRootNewOff; // the animation itself, at index 0
        }
        else
        {
            const AptCharacterT<uint32_t> *pOldChar = c.SrcAt<AptCharacterT<uint32_t>>(nOldChar);
            nNewChar                                = ArenaAlloc(c, sizeof(AptCharacter), 8);
            FillCharacter(c, pOldChar, nNewChar);
        }
        *c.arena.At<uint64_t>(nSlotsNew + (uint64_t)i * 8) = nNewChar;
    }

    // exports
    int nExports         = oldAnim.nExports;
    uint64_t nExportsNew = 0;
    if (oldAnim.aExports.value)
    {
        nExportsNew = ArenaAlloc(c, (size_t)nExports * sizeof(AptExport), 8);
        for (int i = 0; i < nExports && !c.bFail; i++)
        {
            const AptExportT<uint32_t> *pOldExp =
                c.SrcAt<AptExportT<uint32_t>>(oldAnim.aExports.value + (uint32_t)(i * sizeof(AptExportT<uint32_t>)));
            uint64_t nName     = CopyString(c, pOldExp->szName.value);
            AptExport *pNewExp = c.arena.At<AptExport>(nExportsNew + (uint64_t)i * sizeof(AptExport));
            pNewExp->szName    = (char *)(uintptr_t)nName;
            pNewExp->nID       = pOldExp->nID;
        }
    }

    // imports
    int nImports         = oldAnim.nImports;
    uint64_t nImportsNew = 0;
    if (oldAnim.aImports.value)
    {
        nImportsNew = ArenaAlloc(c, (size_t)nImports * sizeof(AptImport), 8);
        for (int i = 0; i < nImports && !c.bFail; i++)
        {
            const AptImportT<uint32_t> *pOldImp =
                c.SrcAt<AptImportT<uint32_t>>(oldAnim.aImports.value + (uint32_t)(i * sizeof(AptImportT<uint32_t>)));
            uint64_t nFile     = CopyString(c, pOldImp->szFile.value);
            uint64_t nName     = CopyString(c, pOldImp->szName.value);
            AptImport *pNewImp = c.arena.At<AptImport>(nImportsNew + (uint64_t)i * sizeof(AptImport));
            pNewImp->szFile    = (char *)(uintptr_t)nFile;
            pNewImp->szName    = (char *)(uintptr_t)nName;
            pNewImp->nID       = pOldImp->nID;
            // the AptFilePtr slot is dead in the file; carry the raw value over
            *(uint64_t *)&pNewImp->file = (uint64_t)pOldImp->file.value;
        }
    }

    // the movie of the root animation
    ConvertMovie(c, &oldAnim.movie, c.nRootNewOff, offsetof(AptCharacter, animation.movie));

    if (c.bFail)
    {
        return;
    }

    AptCharacter *pNewRoot          = c.arena.At<AptCharacter>(c.nRootNewOff);
    pNewRoot->eType                 = AptCharacterType_Animation;
    pNewRoot->pParentAnim           = (AptCharacter *)(uintptr_t)pOldRoot->pParentAnim.value;
    AptCharacterAnimation *pNewAnim = &pNewRoot->animation;
    pNewAnim->nCharacters           = nCharacters;
    pNewAnim->apCharacters          = (AptCharacter **)(uintptr_t)nSlotsNew;
    pNewAnim->nWidth                = oldAnim.nWidth;
    pNewAnim->nHeight               = oldAnim.nHeight;
    pNewAnim->nMillisecondsPerFrame = oldAnim.nMillisecondsPerFrame;
    pNewAnim->nImports              = nImports;
    pNewAnim->aImports              = (AptImport *)(uintptr_t)nImportsNew;
    pNewAnim->nExports              = nExports;
    pNewAnim->aExports              = (AptExport *)(uintptr_t)nExportsNew;
    pNewAnim->nCurrentConstantIndex = 0; // runtime state, dead in the file
}

/**
 * Synthesizes a new-style header tag for the converted blob so that
 * AptGetPtrSize/IsBuiltForDecoupling/GetSwfVersion (and the debug-build
 * APT_CHECK_FILE_BUILD_TYPE asserts) see a well-formed 64-bit file.
 * Layout: "Apt Data:<decouple>:<swf version>:<ptr size>".
 */
void WriteHeaderTag(Converter &c)
{
    uint64_t nHdr = ArenaAlloc(c, 16, 8);
    APT_ASSERT(nHdr == 0);
    const char *szSrcTag = (const char *)c.pSrc;
    char cDecoupled      = (szSrcTag[8] == ':') ? szSrcTag[9] : '0';
    char cSwfVersion     = (szSrcTag[10] == ':') ? szSrcTag[11] : '6';

    char *pTag = c.arena.At<char>(nHdr);
    memcpy(pTag, "Apt Data", 8);
    pTag[8]  = ':';
    pTag[9]  = cDecoupled;
    pTag[10] = ':';
    pTag[11] = cSwfVersion;
    pTag[12] = ':';
    pTag[13] = '8';
    pTag[14] = 0;
    pTag[15] = 0;
}

/** Builds a self-contained native-layout .const blob. */
void *ConvertConstFile(Converter &c, const AptConstFile32 *pOldConst, size_t *pnSize)
{
    const uint8_t *pOldBase = (const uint8_t *)pOldConst;
    int nConstants          = pOldConst->nConstants;
    const AptConstantTableT<uint32_t> *pOldTable =
        (const AptConstantTableT<uint32_t> *)(pOldBase + pOldConst->aConstants);

    // measure the string payload
    size_t nStringBytes = 0;
    for (int i = 0; i < nConstants; i++)
    {
        if (pOldTable[i].eType == AptVFT_StringValue)
        {
            nStringBytes += strlen((const char *)(pOldBase + pOldTable[i].uValue.value)) + 1;
        }
    }

    size_t nSize      = sizeof(AptConstFile) + (size_t)nConstants * sizeof(AptConstantTable) + nStringBytes;
    uint8_t *pNewBase = (uint8_t *)APT_MALLOC_BLOCK(nSize);
    if (!pNewBase)
    {
        c.Fail("out of memory for the converted const table");
        return NULL;
    }
    memset(pNewBase, 0, nSize);

    AptConstFile *pNewConst = (AptConstFile *)pNewBase;
    memcpy(pNewConst->aMagic, pOldConst->aMagic, sizeof(pNewConst->aMagic));
    pNewConst->pMainCharacter = (AptCharacter *)(uintptr_t)c.nRootNewOff; // offset into the converted .apt blob
    pNewConst->nConstants     = nConstants;
    pNewConst->aConstants     = (AptConstantTable *)(uintptr_t)sizeof(AptConstFile); // blob-relative, resolved by Resolve()

    AptConstantTable *pNewTable = (AptConstantTable *)(pNewBase + sizeof(AptConstFile));
    size_t nStringOff           = sizeof(AptConstFile) + (size_t)nConstants * sizeof(AptConstantTable);
    for (int i = 0; i < nConstants; i++)
    {
        pNewTable[i].eType = pOldTable[i].eType;
        if (pOldTable[i].eType == AptVFT_StringValue)
        {
            const char *szOld = (const char *)(pOldBase + pOldTable[i].uValue.value);
            size_t nLen       = strlen(szOld) + 1;
            memcpy(pNewBase + nStringOff, szOld, nLen);
            pNewTable[i].szString = (char *)(uintptr_t)nStringOff; // blob-relative, resolved per use by _parseStream
            nStringOff += nLen;
        }
        else
        {
            // float/int/bool/register/lookup payloads occupy the low 4 bytes of the union
            *(uint64_t *)&pNewTable[i].szString = (uint64_t)pOldTable[i].uValue.value;
        }
    }

    *pnSize = nSize;
    return pNewBase;
}

} // anonymous namespace

bool AptConvertFile32(const void *pAptData, const void *pConstTable,
                      void **ppNewAptData, size_t *pnNewAptSize,
                      void **ppNewConstTable, size_t *pnNewConstSize)
{
    APT_ASSERT(pAptData && pConstTable);
#if !defined(APT_SYSTEM_LITTLE_ENDIAN)
#error "AptConvertFile32 assumes a little-endian host"
#endif

    Converter c;
    c.pSrc = (const uint8_t *)pAptData;
    c.arena.Init();
    c.bFail       = false;
    c.nRootNewOff = 0;

    const AptConstFile32 *pOldConst = (const AptConstFile32 *)pConstTable;
    c.nRootOldOff                   = pOldConst->pMainCharacter;

    WriteHeaderTag(c);
    ConvertRoot(c);

    void *pNewConst      = NULL;
    size_t nNewConstSize = 0;
    if (!c.bFail)
    {
        pNewConst = ConvertConstFile(c, pOldConst, &nNewConstSize);
    }

    if (c.bFail)
    {
        c.arena.Destroy();
        if (pNewConst)
        {
            APT_FREE_BLOCK(pNewConst, nNewConstSize);
        }
        return false;
    }

    *ppNewAptData    = c.arena.pData;
    *pnNewAptSize    = c.arena.nCap; // APT_FREE_BLOCK needs the allocation size
    *ppNewConstTable = pNewConst;
    *pnNewConstSize  = nNewConstSize;
    return true;
}

#endif // APT_PLATFORM_PTR_SIZE == 8
