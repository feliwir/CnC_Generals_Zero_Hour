#include <gtest/gtest.h>

#include <Apt/Apt.h>

#include <cctype>
#include <chrono>
#include <cstdarg>
#include <cstring>
#include <string>
#include <vector>

#define SAGE_TEST_APT_DATA_DIR SAGE_TEST_DATA_DIR "/apt"

std::chrono::time_point<std::chrono::high_resolution_clock> gStartTime =
    std::chrono::high_resolution_clock::now();

void AptDebugPrint(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
}

uint64_t AptGetCurrentTime()
{
    // Return elapsed microseconds since the epoch
    auto now = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::microseconds>(now - gStartTime);
    return static_cast<uint64_t>(duration.count());
}

void AptLoadAnimationImpl(const char *szBaseName,
                          AptFilePtr pAsyncLoadContext)
{
    char aptFilePath[512];
    char constFilePath[512];
    snprintf(aptFilePath, sizeof(aptFilePath), "%s/%s.apt", SAGE_TEST_APT_DATA_DIR "/32_coupled" , szBaseName);
    snprintf(constFilePath, sizeof(constFilePath), "%s/%s.const", SAGE_TEST_APT_DATA_DIR "/32_coupled", szBaseName);

    FILE *aptFile = fopen(aptFilePath, "rb");
    if (!aptFile)
    {
        AptDebugPrint("Failed to open file: %s\n", aptFilePath);
        return;
    }

    // Load the aptFile into RAM
    fseek(aptFile, 0, SEEK_END);
    size_t fileSize = ftell(aptFile);
    fseek(aptFile, 0, SEEK_SET);
    void *aptFileData = malloc(fileSize);
    fread(aptFileData, 1, fileSize, aptFile);
    fclose(aptFile);

    FILE *constFile = fopen(constFilePath, "rb");
    if (!constFile)
    {
        AptDebugPrint("Failed to open file: %s\n", constFilePath);
        free(aptFileData);
        fclose(aptFile);
        return;
    }

    // Load the constFile into RAM
    fseek(constFile, 0, SEEK_END);
    size_t constFileSize = ftell(constFile);
    fseek(constFile, 0, SEEK_SET);
    void *constFileData = malloc(constFileSize);
    fread(constFileData, 1, constFileSize, constFile);
    fclose(constFile);

    // The .apt buffer stays owned by us (via the user-data handle) until Apt calls
    // pfnFreeAnimation; the .const buffer is released through pfnFreeConstantTable
    // as soon as the load completes.
    AptCompleteAnimationAsyncLoad(pAsyncLoadContext, aptFileData, constFileData,
                                  aptFileData);
}

AptAssetRenderingUnit AptLoadRenderingUnitImpl(AptAnimationUserData userData, int nID)
{
    // For this test, we don't have any rendering units to load, so we just call the completion callback immediately.
    AptAssetRenderingUnit unit = {};
    return unit;
}

AptAssetTexture AptLoadTextureImpl(AptAnimationUserData userData, int nID)
{
    // For this test, we don't have any textures to load, so we just return a null texture.
    AptAssetTexture texture = {};
    return texture;
}

void AptFreeRenderingUnitImpl(AptAssetRenderingUnit unit)
{
    // Nothing was allocated by AptLoadRenderingUnitImpl.
}

void AptFreeTextureImpl(AptAssetTexture texture)
{
    // Nothing was allocated by AptLoadTextureImpl.
}

void AptFreeConstantTable(void *pData)
{
    free(pData);
}

void AptFreeAnimation(void *pData)
{
    free(pData);
}

void AptSetupDefaultCallbacks(AptUserFunctions &funcs)
{
    funcs.pfnMemAlloc = malloc;
    funcs.pfnMemFree = free;
    funcs.pfnDebugPrint = AptDebugPrint;
    funcs.pfnGetCurrentTime = AptGetCurrentTime;
    funcs.pfnLoadAnimation = AptLoadAnimationImpl;
    funcs.pfnLoadRenderingUnit = AptLoadRenderingUnitImpl;
    funcs.pfnLoadTexture = AptLoadTextureImpl;
    funcs.pfnFreeRenderingUnit = AptFreeRenderingUnitImpl;
    funcs.pfnFreeTexture = AptFreeTextureImpl;
    funcs.pfnFreeConstantTable = AptFreeConstantTable;
    funcs.pfnFreeAnimation = AptFreeAnimation;
    funcs.pfnSetBackgroundColour = [](unsigned int nColour) {
        // Implement background color setting logic here if needed
    };
}

// Builds the standard init params used by most tests below.
static AptLibraryInitParams AptDefaultLibParams()
{
    AptLibraryInitParams libParams;
    AptSetupDefaultCallbacks(libParams.Funcs);
    return libParams;
}

TEST(Apt, LibraryInitialization)
{
    AptLibraryInitParams libParams = AptDefaultLibParams();
    AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
    ASSERT_NE(hLib, nullptr);

    AptInitParams initParams;
    AptInitialize(hLib, &initParams);

    EXPECT_EQ(AptGetUserFuncs().pfnMemAlloc != nullptr, true);
    EXPECT_EQ(AptGetUserFuncs().pfnMemFree != nullptr, true);
    // pfnMemFreeSize is not supplied above; AptLibraryInitialize patches in a
    // passthru to pfnMemFree.
    EXPECT_EQ(AptGetUserFuncs().pfnMemFreeSize != nullptr, true);

    AptShutdown(hLib);
    AptLibraryShutdown(hLib);
}

TEST(Apt, Load32Coupled)
{
    AptLibraryInitParams libParams = AptDefaultLibParams();
    AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
    ASSERT_NE(hLib, nullptr);

    AptInitParams initParams;
    AptInitialize(hLib, &initParams);

    AptLoadAnimation("GuiTest", "_level0");
    AptUpdate(0);

    AptShutdown(hLib);
    AptLibraryShutdown(hLib);
}

// AptReset tears down and rebuilds the default target, the value system and the
// native function caches in place. It is the most order-sensitive entry point in
// the library, so exercise it with a live animation loaded either side.
//
// NOTE: AptReset currently leaks an entire AptTarget (~25KB; visible as
// "Total Bytes Used: 0x1D8" plus ~0x6278 outside DOGMA in the shutdown report).
// AptReset calls GetTarget()->Reset(), which already rebuilds the target in
// place, and then overwrites gpDefaultTarget with a freshly new'd AptTarget
// without deleting the old one. nTargetsCreated does not track the default
// target, so the assert in AptCommonShutdown does not catch it. This test
// characterises the current behaviour; it does not assert the leak away.
TEST(Apt, ResetCycle)
{
    AptLibraryInitParams libParams = AptDefaultLibParams();
    AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
    ASSERT_NE(hLib, nullptr);

    AptInitParams initParams;
    AptInitialize(hLib, &initParams);

    AptLoadAnimation("GuiTest", "_level0");
    AptUpdate(0);

    AptReset(hLib);

    // The rebuilt target must be usable: reload and run again.
    AptLoadAnimation("GuiTest", "_level0");
    AptUpdate(0);

    AptShutdown(hLib);
    AptLibraryShutdown(hLib);
}
//------------------------------------------------------------------------------
// Tests below exist to prove the library handle owns its state. None of them are
// expressible against the old global-based API.

// Counting allocators, one per cycle, to prove the callbacks belong to the
// handle rather than to a global that merely gets overwritten.
static int gAllocCountA = 0;
static int gAllocCountB = 0;
static void *AptCountingMallocA(size_t n) { ++gAllocCountA; return malloc(n); }
static void *AptCountingMallocB(size_t n) { ++gAllocCountB; return malloc(n); }

TEST(Apt, DifferentCallbacksPerCycle)
{
    gAllocCountA = 0;
    gAllocCountB = 0;

    {
        AptLibraryInitParams libParams = AptDefaultLibParams();
        libParams.Funcs.pfnMemAlloc = AptCountingMallocA;
        AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
        ASSERT_NE(hLib, nullptr);
        AptInitParams initParams;
        AptInitialize(hLib, &initParams);
        AptShutdown(hLib);
        AptLibraryShutdown(hLib);
    }
    EXPECT_GT(gAllocCountA, 0);
    EXPECT_EQ(gAllocCountB, 0);

    const int nAfterFirstCycle = gAllocCountA;

    {
        AptLibraryInitParams libParams = AptDefaultLibParams();
        libParams.Funcs.pfnMemAlloc = AptCountingMallocB;
        AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
        ASSERT_NE(hLib, nullptr);
        AptInitParams initParams;
        AptInitialize(hLib, &initParams);
        AptShutdown(hLib);
        AptLibraryShutdown(hLib);
    }
    // The second library used its own allocator, and did not touch the first's.
    EXPECT_GT(gAllocCountB, 0);
    EXPECT_EQ(gAllocCountA, nAfterFirstCycle);
}

// Three full cycles. Pass/fail alone proves little here - what matters is that
// the pool accounting for cycles 2 and 3 agrees. Cycle 1 may differ on first
// touch; a divergence between 2 and 3 means state survived a shutdown.
TEST(Apt, ThreeLibraryCyclesAreStable)
{
    size_t aBytesUsed[3] = {0, 0, 0};
    size_t aItemsAllocated[3] = {0, 0, 0};

    for (int i = 0; i < 3; ++i)
    {
        AptLibraryInitParams libParams = AptDefaultLibParams();
        AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
        ASSERT_NE(hLib, nullptr);

        AptInitParams initParams;
        AptInitialize(hLib, &initParams);
        AptLoadAnimation("GuiTest", "_level0");
        AptUpdate(0);
        AptShutdown(hLib);

        aBytesUsed[i] = GetNonGCPoolManager()->GetTotalBytesUsed();
        aItemsAllocated[i] = GetNonGCPoolManager()->mnItemsAllocated;

        AptLibraryShutdown(hLib);
    }

    EXPECT_EQ(aBytesUsed[1], aBytesUsed[2]);
    EXPECT_EQ(aItemsAllocated[1], aItemsAllocated[2]);
}

// The library root must actually be cleared on shutdown. Checked behaviourally:
// AptLibraryInitialize refuses to build a second library while one is live, so a
// success here proves the first was fully torn down.
TEST(Apt, LibraryCanBeRecreatedAfterShutdown)
{
    AptLibraryInitParams libParams = AptDefaultLibParams();
    AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
    ASSERT_NE(hLib, nullptr);
    AptInitParams initParams;
    AptInitialize(hLib, &initParams);
    AptShutdown(hLib);
    AptLibraryShutdown(hLib);

    AptLibraryInitParams libParams2 = AptDefaultLibParams();
    AptLibraryHandle hLib2 = AptLibraryInitialize(&libParams2);
    EXPECT_NE(hLib2, nullptr);
    if (hLib2 != nullptr)
    {
        AptLibraryShutdown(hLib2);
    }
}

// A second library must be refused while one is live.
TEST(Apt, SecondLiveLibraryIsRefused)
{
    AptLibraryInitParams libParams = AptDefaultLibParams();
    AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
    ASSERT_NE(hLib, nullptr);

    AptLibraryInitParams libParams2 = AptDefaultLibParams();
    EXPECT_EQ(AptLibraryInitialize(&libParams2), nullptr);

    AptLibraryShutdown(hLib);
}

// Opt flags used to live in a global that no shutdown path ever reset, so they
// leaked from one init into the next. They belong to the handle now.
TEST(Apt, OptFlagsDoNotSurviveShutdown)
{
    {
        AptLibraryInitParams libParams = AptDefaultLibParams();
        libParams.nOptFlags = APT_OPT_SPRTREE;
        AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
        ASSERT_NE(hLib, nullptr);
        EXPECT_TRUE(AptOptGetFlags(hLib) & APT_OPT_SPRTREE);
        AptLibraryShutdown(hLib);
    }
    {
        AptLibraryInitParams libParams = AptDefaultLibParams();
        libParams.nOptFlags = 0;
        AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
        ASSERT_NE(hLib, nullptr);
        // Under the old design gAptOptFlags was never reset, so SPRTREE would
        // still be set here.
        EXPECT_FALSE(AptOptGetFlags(hLib) & APT_OPT_SPRTREE);
        AptLibraryShutdown(hLib);
    }
}

TEST(Apt, OptFlagsToggle)
{
    AptLibraryInitParams libParams = AptDefaultLibParams();
    AptLibraryHandle hLib = AptLibraryInitialize(&libParams);
    ASSERT_NE(hLib, nullptr);

    EXPECT_FALSE(AptOptGetFlags(hLib) & APT_OPT_SPRSTACK);
    AptOptEnable(hLib, APT_OPT_SPRSTACK);
    EXPECT_TRUE(AptOptGetFlags(hLib) & APT_OPT_SPRSTACK);
    AptOptDisable(hLib, APT_OPT_SPRSTACK);
    EXPECT_FALSE(AptOptGetFlags(hLib) & APT_OPT_SPRSTACK);

    AptLibraryShutdown(hLib);
}

// Missing the mandatory allocator callbacks must fail rather than crash.
TEST(Apt, LibraryInitFailsWithoutAllocator)
{
    AptLibraryInitParams libParams; // Funcs left empty: no pfnMemAlloc/pfnMemFree
    EXPECT_EQ(AptLibraryInitialize(&libParams), nullptr);
    EXPECT_EQ(AptLibraryInitialize(nullptr), nullptr);

    // A failed init must not have consumed the single library slot.
    AptLibraryInitParams good = AptDefaultLibParams();
    AptLibraryHandle hLib = AptLibraryInitialize(&good);
    EXPECT_NE(hLib, nullptr);
    if (hLib != nullptr)
    {
        AptLibraryShutdown(hLib);
    }
}
