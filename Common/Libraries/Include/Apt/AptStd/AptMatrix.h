#pragma once
#include "AptDefine.h"

struct AptMatrix
{
    void AptMatrixCopy(const AptMatrix *pMatrix)
    {
        if (pMatrix != NULL)
        {
            a  = pMatrix->a;
            b  = pMatrix->b;
            c  = pMatrix->c;
            d  = pMatrix->d;
            tx = pMatrix->tx;
            ty = pMatrix->ty;
        }
    }

    bool AptMatrixInverse(const AptMatrix *pMatrix)
    {
        bool bHasInverse = (pMatrix != NULL);
        if (bHasInverse)
        {
            // The determinant can be different than 1 if we have an homotece
            float determinant = pMatrix->a * pMatrix->d - pMatrix->b * pMatrix->c;
            bHasInverse       = (determinant != 0.f);
            if (bHasInverse)
            {
                a  = pMatrix->d / determinant;
                b  = -pMatrix->b / determinant;
                c  = -pMatrix->c / determinant;
                d  = pMatrix->a / determinant;
                tx = (-pMatrix->tx * pMatrix->d + pMatrix->ty * pMatrix->c) / determinant;
                ty = (pMatrix->tx * pMatrix->b - pMatrix->ty * pMatrix->a) / determinant;
            }
        }
        return bHasInverse; // Returns false if the inverse matrix cannot be calculated
    }

    float a, b, c, d;
    float tx, ty;

    APT_NEW_DELETE_OPERATORS

    // Added Metric defines
};
