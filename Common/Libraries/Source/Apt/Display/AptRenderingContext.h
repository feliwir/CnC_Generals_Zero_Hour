/**
 * Tracks the current vertex matrix and colour transform stacks used while rendering.
 */

#pragma once

#include "AptStd/AptMatrix.h"
#include "AptStd/AptCXForm.h"

struct AptRect;

struct AptRenderingContext
{
#if !defined(APT_3D)
    AptMatrix curVertexMatrix;
    AptMatrix *pVertexMatrixInterator;
    AptMatrix aVertexMatrixStack[16];
#else
    AptMath::Mat44T curVertexMatrix;
    AptMath::Mat44T *pVertexMatrixInterator;
    AptMath::Mat44T aVertexMatrixStack[16];
#endif

    AptCXForm curCXForm;
    AptCXForm *pCXFormInterator;
    AptCXForm aCXFormStack[16];

#if (APT_DEBUG_LEVEL >= APT_DEBUG_NORMAL)
    int nCXFormStack;
    int nVertexMatrixStack;
#endif

  public:
    AptRenderingContext(void);
    void pushColourTransform(void);
    void popColourTransform(void);
    void appendColourTransform(const AptCXForm *pCXForm);

    void pushVertexMatrix(void);
    void popVertexMatrix(void);

#if !defined(APT_3D)
    void getVertexMatrix(AptMatrix *pMatrix) const;
    void setVertexMatrix(AptMatrix *pSetMatrix);
    void appendVertexMatrix(const AptMatrix *pMatrix);
#else
    void getVertexMatrix(AptMath::Mat44T *pMatrix) const;
    void setVertexMatrix(AptMath::Mat44T *pSetMatrix);
    void appendVertexMatrix(const AptMath::Mat44T *pMatrix);
    static void multMatrix3D(const AptMath::Mat44T *pA, const AptMath::Mat44T *pB, AptMath::Mat44T *pOut);
#endif

    static void multMatrix(const AptMatrix *pA, const AptMatrix *pB, AptMatrix *pOut);

    static void expandBoundingRect(const AptRect *pNewRect, const AptMatrix *pCurrentTransform, AptRect *pBoundingRect);

    APT_NEW_DELETE_OPERATORS
};
