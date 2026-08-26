#pragma once

/*** Include files ********************************************************************************/

#include "AptDefine.h"
#include "AptMatrix.h"

#include <cstring> // for memset
#include <cstdint> // for uint32_t, uintptr_t

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

struct AptColorHelperScale;
struct AptColorHelperTranslate;

class AptMath
{

  public:
    AptMath();

    // 4x4 matrix enum
    using MatIndexT = enum MatIndex_t {
        MAT_A,
        MAT_B,
        MAT_C,
        MAT_P,
        MAT_D,
        MAT_E,
        MAT_F,
        MAT_Q,
        MAT_G,
        MAT_H,
        MAT_I,
        MAT_R,

        MAT_X,
        MAT_Y,
        MAT_Z,
        MAT_W,

        MAT_NUMELEMENTS
    };

    // 4x4 matrix type
    using Mat44T = struct Mat44_t
    {
        float m[MAT_NUMELEMENTS];
        Mat44_t()
        {
            // Opt Note: We know there's 16 floats in a Mat44 so
            // let's just explicitly list all the stores.
            m[MAT_A] = 0.0f;
            m[MAT_B] = 0.0f;
            m[MAT_C] = 0.0f;
            m[MAT_P] = 0.0f;

            m[MAT_D] = 0.0f;
            m[MAT_E] = 0.0f;
            m[MAT_F] = 0.0f;
            m[MAT_Q] = 0.0f;

            m[MAT_G] = 0.0f;
            m[MAT_H] = 0.0f;
            m[MAT_I] = 0.0f;
            m[MAT_R] = 0.0f;

            m[MAT_X] = 0.0f;
            m[MAT_Y] = 0.0f;
            m[MAT_Z] = 0.0f;
            m[MAT_W] = 0.0f;
        }
    };

    // 1x4 vector type
    using Vec4T = struct Vec4_t
    {
        float vx, vy, vz, vw;
    };

    // Clip transform: position transform, color scale, color add
    using ClipTransformT = struct ClipTransform_t
    {
        ClipTransform_t() : Pos44(),
                            vColorMul4(),
                            vColorAdd4()
        {
        }

        ClipTransform_t &operator=(const ClipTransform_t &other)
        {
            Pos44      = other.Pos44;
            vColorMul4 = other.vColorMul4;
            vColorAdd4 = other.vColorAdd4;
            return *this;
        }

        Mat44T Pos44;
        AptColorHelperScale vColorMul4;
        AptColorHelperTranslate vColorAdd4;
#if defined(APT_3D)
        // We need to store this values out of Pos44
        float xrotation;
        float yrotation;
        float zposition;
#endif
    };

    // clip transform stack
    static void ClipStackInit(uint32_t uiSize);
    static void ClipStackShutdown();
    static void ClipStackPushUnit();
    static void ClipStackMakeUnit();

#if defined(APT_3D)
    static ClipTransformT *ClipStackPush();
    static ClipTransformT *ClipStackPop();
    static ClipTransformT *_ClipStackGetTop();
    static bool ClipStackIsEmpty();
#else
    static APT_INLINE ClipTransformT *ClipStackPush();
    static APT_INLINE ClipTransformT *ClipStackPop();
    static APT_INLINE ClipTransformT *_ClipStackGetTop();
    static APT_INLINE bool ClipStackIsEmpty();
#endif // APT_3D

    /** @brief 2D matrix multiply */
    static void MatMul2d(AptMath::Mat44T *pOut, const AptMath::Mat44T *pA, const AptMath::Mat44T *pB);
    /** @brief 2d apt matrix to standard 4x4 */
    static void MatConvert(AptMath::Mat44T *pAptMat44, const AptMatrix *pAptMat);

#if defined(APT_3D)
    /** @brief 3D matrix multiply */
    static void MatMul3d(AptMath::Mat44T *pOut, const AptMath::Mat44T *pA, const AptMath::Mat44T *pB);
    static void MatRotate3d(AptMath::Mat44T *pMatrix, const float xRot, const float yRot, const float z, const float zScale = 1.f);

    static void MatMakeIdentity(AptMath::Mat44T *pOut);
    static void MatRotateYAxis(AptMath::Mat44T *pOut, const float angle);
    static void MatRotateXAxis(AptMath::Mat44T *pOut, const float angle);
#endif // #if defined(APT_3D)

    /** @brief standard 4x4 to 2d apt matrix */
    static void MatConvert(AptMatrix *pAptMat, const AptMath::Mat44T *pAptMat44);

  private:
    // Re-factored members, saves 4 bytes and allows for easier bounds checking.
    static AptMath::ClipTransformT *m_pStackBase; // This pointer is aligned.
    static AptMath::ClipTransformT *m_pAllocatedBase;
    const static uint8_t m_nMinimumAlignment = 0x10; // MUST be a power of two.
    static uint16_t m_nStackCapacity;
    static uint16_t m_nStackCount;

}; // end AptMath

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/
