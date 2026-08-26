#include "_Apt.h"
#include "AptStd/AptMath.h"

#include "MainInline.h"

#if !defined(APT_ENABLE_INLINE)
#include "AptObject/AptMath.inl"
#endif

// Private variables
AptMath::ClipTransformT *AptMath::m_pStackBase     = NULL;
AptMath::ClipTransformT *AptMath::m_pAllocatedBase = NULL;
uint16_t AptMath::m_nStackCapacity                 = 0;
uint16_t AptMath::m_nStackCount                    = 0;

/** Initializes the object on the top of the stack. */
void AptMath::ClipStackMakeUnit()
{
    ClipTransformT *pTransform;
    pTransform = _ClipStackGetTop();

    // unit position transform
    memset(pTransform->Pos44.m, 0x0, sizeof(Mat44T));
    pTransform->Pos44.m[MAT_A] = pTransform->Pos44.m[MAT_E] = pTransform->Pos44.m[MAT_I] = pTransform->Pos44.m[MAT_W] = 1.0f;

#if defined(APT_3D)
    pTransform->xrotation = 0;
    pTransform->yrotation = 0;
    pTransform->zposition = 0.0f;
#endif

    // unit color mul
    AptColorHelperScale &vColorMul4     = pTransform->vColorMul4;
    AptColorHelperTranslate &vColorAdd4 = pTransform->vColorAdd4;

#if defined(APT_USE_FLASH_COLOR_RANGE)
    vColorMul4.SetValuef(AptColorHelper::Red, 100.f);
    vColorMul4.SetValuef(AptColorHelper::Green, 100.f);
    vColorMul4.SetValuef(AptColorHelper::Blue, 100.f);
    vColorMul4.SetValuef(AptColorHelper::Alpha, 100.f);
    vColorAdd4.SetValuef(AptColorHelper::Red, 0.f);
    vColorAdd4.SetValuef(AptColorHelper::Green, 0.f);
    vColorAdd4.SetValuef(AptColorHelper::Blue, 0.f);
    vColorAdd4.SetValuef(AptColorHelper::Alpha, 0.f);
#else
    vColorMul4.SetValue(0xFFFFFFFF);
    vColorAdd4.SetValue(0x00000000);
#endif
}

/** Adds a new object to the top of the stack, and initializes it. */
void AptMath::ClipStackPushUnit()
{
    ClipStackPush();
    ClipStackMakeUnit();
}

/** Allocates and initializes the clip transform stack. @param nMaxTransforms number of transforms to allocate / use */
void AptMath::ClipStackInit(uint32_t nMaxTransforms)
{
    APT_ASSERT(nMaxTransforms <= 0xFFFF && "Max Transforms is currently bounded by a uint 16.");
#if defined(APT_PLATFORM_PLAYSTATION2)
    // Can use Scratchpad if enabled on PS2.
    if (AptOptIsEnabled(APT_OPT_SPRSTACK))
    {
        m_pAllocatedBase = m_pStackBase = (ClipTransformT *)0x70002000;
    }
    else
#endif
    {
        m_pAllocatedBase = (ClipTransformT *)APT_MALLOC_BLOCK((sizeof(ClipTransformT) * nMaxTransforms) + m_nMinimumAlignment);
        uintptr_t temp   = (uintptr_t)m_pAllocatedBase;
        if (temp & (m_nMinimumAlignment - 1))
        {
            temp += m_nMinimumAlignment;
            temp &= ~(m_nMinimumAlignment - 1);
        }
        m_pStackBase = (ClipTransformT *)temp;
    }

    m_nStackCapacity = nMaxTransforms;
    m_nStackCount    = 0;
    ClipStackMakeUnit();
}

/** Frees memory allocated in ClipStackInit(). */
void AptMath::ClipStackShutdown()
{
    if (m_pStackBase)
    {
#if defined(APT_PLATFORM_PLAYSTATION2)
        // Not deallocating scratchpad memory (didn't allocate it either :).
        if (!AptOptIsEnabled(APT_OPT_SPRSTACK))
#endif
        {
            APT_FREE_BLOCK(m_pAllocatedBase, (sizeof(ClipTransformT) * m_nStackCapacity) + m_nMinimumAlignment);
        }
    }

    m_pStackBase = 0;
}

/**
 * Multiplies two 2d position matrices, assuming the 3rd column is (0, 0, 1).
 * @param pOut receives the product matrix
 * @param pA first matrix
 * @param pB second matrix
 */
void AptMath::MatMul2d(Mat44T *pOut, const Mat44T *pA, const Mat44T *pB)
{
    Mat44T temp  = *pA;
    Mat44T temp2 = *pB;

    pOut->m[MAT_A] = temp2.m[MAT_A] * temp.m[MAT_A] + temp2.m[MAT_B] * temp.m[MAT_D];
    pOut->m[MAT_B] = temp2.m[MAT_A] * temp.m[MAT_B] + temp2.m[MAT_B] * temp.m[MAT_E];
    pOut->m[MAT_D] = temp2.m[MAT_D] * temp.m[MAT_A] + temp2.m[MAT_E] * temp.m[MAT_D];
    pOut->m[MAT_E] = temp2.m[MAT_D] * temp.m[MAT_B] + temp2.m[MAT_E] * temp.m[MAT_E];
    pOut->m[MAT_X] = temp2.m[MAT_X] * temp.m[MAT_A] + temp2.m[MAT_Y] * temp.m[MAT_D] + temp.m[MAT_X];
    pOut->m[MAT_Y] = temp2.m[MAT_X] * temp.m[MAT_B] + temp2.m[MAT_Y] * temp.m[MAT_E] + temp.m[MAT_Y];
}

/** Converts a 2d matrix into a 3d matrix with z coordinate = 0. */
void AptMath::MatConvert(Mat44T *pAptMat44, const AptMatrix *pAptMat)
{
    memset(pAptMat44->m, 0x0, sizeof(Mat44T));
    pAptMat44->m[MAT_A] = pAptMat->a;
    pAptMat44->m[MAT_B] = pAptMat->b;
    pAptMat44->m[MAT_D] = pAptMat->c;
    pAptMat44->m[MAT_E] = pAptMat->d;
    pAptMat44->m[MAT_I] = 1.0f;
    pAptMat44->m[MAT_X] = pAptMat->tx;
    pAptMat44->m[MAT_Y] = pAptMat->ty;
    pAptMat44->m[MAT_W] = 1.0f;
}

/** Converts a 3d matrix into a 2d matrix, ignoring the z coordinate. */
void AptMath::MatConvert(AptMatrix *pAptMat, const Mat44T *pAptMat44)
{
    pAptMat->a  = pAptMat44->m[MAT_A];
    pAptMat->b  = pAptMat44->m[MAT_B];
    pAptMat->c  = pAptMat44->m[MAT_D];
    pAptMat->d  = pAptMat44->m[MAT_E];
    pAptMat->tx = pAptMat44->m[MAT_X];
    pAptMat->ty = pAptMat44->m[MAT_Y];
}

#if defined(APT_3D)

/** Pushes a new transform onto the clip transform stack and returns it. */
AptMath::ClipTransformT *AptMath::ClipStackPush()
{
    APT_ASSERT((m_nStackCount + 1) < m_nStackCapacity);
    return &m_pStackBase[++m_nStackCount];
}

/** Pops the top transform off the clip transform stack and returns it. */
AptMath::ClipTransformT *AptMath::ClipStackPop()
{
    APT_ASSERT(m_nStackCount < m_nStackCapacity);
    return &m_pStackBase[--m_nStackCount];
}

/** @return the transform on the top of the clip transform stack, without popping it. */
AptMath::ClipTransformT *AptMath::_ClipStackGetTop()
{
    APT_ASSERT(m_nStackCount < m_nStackCapacity);
    return (&m_pStackBase[m_nStackCount]);
}

/** @return true if the clip transform stack is empty. */
bool AptMath::ClipStackIsEmpty()
{
    APT_ASSERT(m_nStackCount < m_nStackCapacity);
    return (m_nStackCount == 0);
}

/**
 * Multiplies two 3d position matrices (4 x 4), assuming the 4th column is (0, 0, 0, 1).
 * @param pOut receives the product matrix
 * @param pA first matrix
 * @param pB second matrix
 */
void AptMath::MatMul3d(Mat44T *pOut, const Mat44T *pA, const Mat44T *pB)
{
    float a = pB->m[AptMath::MAT_A] * pA->m[AptMath::MAT_A] + pB->m[AptMath::MAT_B] * pA->m[AptMath::MAT_D] + pB->m[AptMath::MAT_C] * pA->m[AptMath::MAT_G] + pB->m[AptMath::MAT_P] * pA->m[AptMath::MAT_X];
    float b = pB->m[AptMath::MAT_A] * pA->m[AptMath::MAT_B] + pB->m[AptMath::MAT_B] * pA->m[AptMath::MAT_E] + pB->m[AptMath::MAT_C] * pA->m[AptMath::MAT_H] + pB->m[AptMath::MAT_P] * pA->m[AptMath::MAT_Y];
    float c = pB->m[AptMath::MAT_A] * pA->m[AptMath::MAT_C] + pB->m[AptMath::MAT_B] * pA->m[AptMath::MAT_F] + pB->m[AptMath::MAT_C] * pA->m[AptMath::MAT_I] + pB->m[AptMath::MAT_P] * pA->m[AptMath::MAT_Z];

    float d = pB->m[AptMath::MAT_D] * pA->m[AptMath::MAT_A] + pB->m[AptMath::MAT_E] * pA->m[AptMath::MAT_D] + pB->m[AptMath::MAT_F] * pA->m[AptMath::MAT_G] + pB->m[AptMath::MAT_Q] * pA->m[AptMath::MAT_X];
    float e = pB->m[AptMath::MAT_D] * pA->m[AptMath::MAT_B] + pB->m[AptMath::MAT_E] * pA->m[AptMath::MAT_E] + pB->m[AptMath::MAT_F] * pA->m[AptMath::MAT_H] + pB->m[AptMath::MAT_Q] * pA->m[AptMath::MAT_Y];
    float f = pB->m[AptMath::MAT_D] * pA->m[AptMath::MAT_C] + pB->m[AptMath::MAT_E] * pA->m[AptMath::MAT_F] + pB->m[AptMath::MAT_F] * pA->m[AptMath::MAT_I] + pB->m[AptMath::MAT_Q] * pA->m[AptMath::MAT_Z];

    float g = pB->m[AptMath::MAT_G] * pA->m[AptMath::MAT_A] + pB->m[AptMath::MAT_H] * pA->m[AptMath::MAT_D] + pB->m[AptMath::MAT_I] * pA->m[AptMath::MAT_G] + pB->m[AptMath::MAT_Q] * pA->m[AptMath::MAT_X];
    float h = pB->m[AptMath::MAT_G] * pA->m[AptMath::MAT_B] + pB->m[AptMath::MAT_H] * pA->m[AptMath::MAT_E] + pB->m[AptMath::MAT_I] * pA->m[AptMath::MAT_H] + pB->m[AptMath::MAT_Q] * pA->m[AptMath::MAT_Y];
    float i = pB->m[AptMath::MAT_G] * pA->m[AptMath::MAT_C] + pB->m[AptMath::MAT_H] * pA->m[AptMath::MAT_F] + pB->m[AptMath::MAT_I] * pA->m[AptMath::MAT_I] + pB->m[AptMath::MAT_Q] * pA->m[AptMath::MAT_Z];

    float x = pB->m[AptMath::MAT_X] * pA->m[AptMath::MAT_A] + pB->m[AptMath::MAT_Y] * pA->m[AptMath::MAT_D] + pB->m[AptMath::MAT_Z] * pA->m[AptMath::MAT_G] + pB->m[AptMath::MAT_W] * pA->m[AptMath::MAT_X];
    float y = pB->m[AptMath::MAT_X] * pA->m[AptMath::MAT_B] + pB->m[AptMath::MAT_Y] * pA->m[AptMath::MAT_E] + pB->m[AptMath::MAT_Z] * pA->m[AptMath::MAT_H] + pB->m[AptMath::MAT_W] * pA->m[AptMath::MAT_Y];
    float z = pB->m[AptMath::MAT_X] * pA->m[AptMath::MAT_C] + pB->m[AptMath::MAT_Y] * pA->m[AptMath::MAT_F] + pB->m[AptMath::MAT_Z] * pA->m[AptMath::MAT_I] + pB->m[AptMath::MAT_W] * pA->m[AptMath::MAT_Z];

    pOut->m[AptMath::MAT_A] = a;
    pOut->m[AptMath::MAT_B] = b;
    pOut->m[AptMath::MAT_C] = c;

    pOut->m[AptMath::MAT_D] = d;
    pOut->m[AptMath::MAT_E] = e;
    pOut->m[AptMath::MAT_F] = f;

    pOut->m[AptMath::MAT_G] = g;
    pOut->m[AptMath::MAT_H] = h;
    pOut->m[AptMath::MAT_I] = i;

    pOut->m[AptMath::MAT_X] = x;
    pOut->m[AptMath::MAT_Y] = y;
    pOut->m[AptMath::MAT_Z] = z;
}

/** Builds the 4 x 4 identity matrix (1 in the diagonal, 0 elsewhere). */
void AptMath::MatMakeIdentity(Mat44T *pOut)
{
    memset(pOut->m, 0x0, sizeof(Mat44T));
    pOut->m[MAT_A] = pOut->m[MAT_E] = pOut->m[MAT_I] = pOut->m[MAT_W] = 1.0f;
}

/**
 * Rotates a 3d position matrix around the Y axis.
 * @param pMatrix matrix to be rotated
 * @param angle rotation angle in radians
 */
void AptMath::MatRotateYAxis(Mat44T *pMatrix, const float angle)
{
    float rotC = cosf(angle);
    float rotS = sinf(angle);

    float matA = pMatrix->m[MAT_A];
    float matC = pMatrix->m[MAT_C];
    float matD = pMatrix->m[MAT_D];
    float matF = pMatrix->m[MAT_F];
    float matG = pMatrix->m[MAT_G];
    float matI = pMatrix->m[MAT_I];

    pMatrix->m[MAT_A] = matA * rotC + matC * rotS;
    pMatrix->m[MAT_C] = -matA * rotS + matC * rotC;

    pMatrix->m[MAT_D] = matD * rotC + matF * rotS;
    pMatrix->m[MAT_F] = -matD * rotS + matF * rotC;

    pMatrix->m[MAT_G] = matG * rotC + matI * rotS;
    pMatrix->m[MAT_I] = -matG * rotS + matI * rotC;
}

/**
 * Rotates a 3d position matrix around the X axis.
 * @param pMatrix matrix to be rotated
 * @param angle rotation angle in radians
 */
void AptMath::MatRotateXAxis(Mat44T *pMatrix, const float angle)
{
    float rotC = cosf(angle);
    float rotS = sinf(angle);

    float matB = pMatrix->m[MAT_B];
    float matC = pMatrix->m[MAT_C];
    float matE = pMatrix->m[MAT_E];
    float matF = pMatrix->m[MAT_F];
    float matH = pMatrix->m[MAT_H];
    float matI = pMatrix->m[MAT_I];

    pMatrix->m[MAT_B] = matB * rotC - matC * rotS;
    pMatrix->m[MAT_C] = matB * rotS + matC * rotC;

    pMatrix->m[MAT_E] = matE * rotC - matF * rotS;
    pMatrix->m[MAT_F] = matE * rotS + matF * rotC;

    pMatrix->m[MAT_H] = matH * rotC - matI * rotS;
    pMatrix->m[MAT_I] = matH * rotS + matI * rotC;
}

/**
 * Transforms a matrix (generally a 2d matrix converted by MatConvert) in 3d space.
 * In Flash rotation doesn't transform the x and y position: parent rotation transforms
 * child objects' x, y. That's why the rotation functions here don't transform X, Y, Z.
 * @param pMatrix matrix to be transformed
 * @param xRot rotation angle around the x axis, in degrees
 * @param yRot rotation angle around the y axis, in degrees
 * @param zPosition position in z
 * @param zScale scaling in z
 */
void AptMath::MatRotate3d(Mat44T *pMatrix, const float xRot, const float yRot, const float zPosition, const float zScale)
{
    // Start with scaling before we rotate
    pMatrix->m[MAT_I] = zScale;

    const float kRadPerDegree = 0.01745329f;

    // If there is a x or y axis rotation then rotate the matrix
    if (0.0f != yRot)
    {
        MatRotateYAxis(pMatrix, yRot * kRadPerDegree);
    }

    if (0.0f != xRot)
    {
        MatRotateXAxis(pMatrix, xRot * kRadPerDegree);
    }

    // Set the z position
    pMatrix->m[MAT_Z] = zPosition;
}
#endif // APT_3D
