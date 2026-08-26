#pragma once

/*** Include files ********************************************************************************/

/*** Defines **************************************************************************************/

/*** Macros ***************************************************************************************/

/*** Type Definitions *****************************************************************************/

/*** Variables ************************************************************************************/

/*** Functions ************************************************************************************/

void AptDisplayList::_drawCharacterInstOpti(const AptRenderItem *pCIH)
{

    AptMath::ClipTransformT *pOutTransform;
    AptMath::ClipTransformT *pCurTransform;
    AptMath::Mat44T *pMatrix;

    pCurTransform = AptMath::_ClipStackGetTop();

    asm volatile(" # Load the current transform into vu0 registers
                 lqc2 vf4,
                 0x00(% 0) #load : Pos44[0].xyzw
                                       lqc2 vf5,
                                   0x10(% 0) #load : Pos44[1].xyzw
                                                         lqc2 vf6,
                                                     0x20(% 0) #load : Pos44[2].xyzw
                                                                           lqc2 vf7,
                                                                       0x30(% 0) #load : Pos44[3].xyzw
                                                                                         "
                 : : "r"(pCurTransform));

    pOutTransform = AptMath::ClipStackPush();

    // swizzle Apt matrix to AptMath::Mat44T
    pMatrix = &pOutTransform->Pos44;
    AptMath::MatConvert(pMatrix, pCIH->GetPositionMatrixConst());

#if defined(APT_3D)
    // Passing the information about the 3d rotation to the ClipTransform
    // It is used to calculate the mask position
    pOutTransform->xrotation = (int32_t)pCIH->GetXRotation();
    pOutTransform->yrotation = (int32_t)pCIH->GetYRotation();
#endif

    // extract the color transform data
    // Color multiply / accumulate could probably be done in VU code as well, but don't need to go there just yet :)
    pOutTransform->vColorMul4.Copy(&(pCIH->GetColorMatrixConst()->scale));
    pOutTransform->vColorAdd4.Copy(&(pCIH->GetColorMatrixConst()->translate));

    pOutTransform->vColorMul4.SetValue(AptColorHelper::Alpha, (int32_t)((pOutTransform->vColorMul4.GetA() * pCurTransform->vColorMul4.GetA()) / 255));
    pOutTransform->vColorMul4.SetValue(AptColorHelper::Red, (int32_t)((pOutTransform->vColorMul4.GetR() * pCurTransform->vColorMul4.GetR()) / 255));
    pOutTransform->vColorMul4.SetValue(AptColorHelper::Green, (int32_t)((pOutTransform->vColorMul4.GetG() * pCurTransform->vColorMul4.GetG()) / 255));
    pOutTransform->vColorMul4.SetValue(AptColorHelper::Blue, (int32_t)((pOutTransform->vColorMul4.GetB() * pCurTransform->vColorMul4.GetB()) / 255));

    pOutTransform->vColorAdd4.SetValue(AptColorHelper::Alpha, pOutTransform->vColorAdd4.GetA() + pCurTransform->vColorAdd4.GetA());
    pOutTransform->vColorAdd4.SetValue(AptColorHelper::Red, pOutTransform->vColorAdd4.GetR() + pCurTransform->vColorAdd4.GetR());
    pOutTransform->vColorAdd4.SetValue(AptColorHelper::Green, pOutTransform->vColorAdd4.GetG() + pCurTransform->vColorAdd4.GetG());
    pOutTransform->vColorAdd4.SetValue(AptColorHelper::Blue, pOutTransform->vColorAdd4.GetB() + pCurTransform->vColorAdd4.GetB());

    asm volatile(" # Concatenate transforms
                 lqc2 vf08,
                 0x00(% 0) #load vtx transform
                     lqc2 vf09,
                 0x10(% 0) #lqc2 vf11, 0x30(% 0) #vmulax.xyzw ACC, vf04, vf08x #stack.transform *= transform vmaddy.xyzw vf08, vf05, vf08y #vmulax.xyzw ACC, vf04, vf09x #vmaddy.xyzw vf09, vf05, vf09y #vmulax.xyzw ACC, vf04, vf11x #vmadday.xyzw ACC, vf05, vf11y #vmaddw.xyzw vf11, vf07, vf11w #vmove.xyzw vf04, vf08 #vmove.xyzw vf05, vf09 #vmove.xy vf07, vf11 #sqc2 vf4, 0x00(% 1) #store : Pos44[0].xyzw
                                                                                                                                                                                                                                                                                                                                                                                                         sqc2 vf5,
                                                                                                                                                                                                                                                                                                                                                                                                     0x10(% 1) #store : Pos44[1].xyzw
                                                                                                                                                                                                                                                                                                                                                                                                                            sqc2 vf6,
                                                                                                                                                                                                                                                                                                                                                                                                                        0x20(% 1) #store : Pos44[2].xyzw
                                                                                                                                                                                                                                                                                                                                                                                                                                               sqc2 vf7,
                                                                                                                                                                                                                                                                                                                                                                                                                                           0x30(% 1) #store : Pos44[3].xyzw
                                                                                                                                                                                                                                                                                                                                                                                                                                                              "
                 : : "r"(pMatrix), "r"(pOutTransform));
}

void AptDisplayList::_drawCharacterInstOpti(const AptMatrix *pCurrMatrix, const AptCXForm *pCurrCXForm)
{

    AptMath::ClipTransformT *pOutTransform;
    AptMath::ClipTransformT *pCurTransform;
    AptMath::Mat44T *pMatrix;

    pCurTransform = AptMath::_ClipStackGetTop();

    asm volatile(" # Load the current transform into vu0 registers
                 lqc2 vf4,
                 0x00(% 0) #load : Pos44[0].xyzw
                                       lqc2 vf5,
                                   0x10(% 0) #load : Pos44[1].xyzw
                                                         lqc2 vf6,
                                                     0x20(% 0) #load : Pos44[2].xyzw
                                                                           lqc2 vf7,
                                                                       0x30(% 0) #load : Pos44[3].xyzw
                                                                                         "
                 : : "r"(pCurTransform));

    pOutTransform = AptMath::ClipStackPush();

    // swizzle Apt matrix to AptMath::Mat44T
    pMatrix = &pOutTransform->Pos44;
    AptMath::MatConvert(pMatrix, pCurrMatrix);

    // extract the color transform data
    // Color multiply / accumulate could probably be done in VU code as well, but don't need to go there just yet :)
    pOutTransform->vColorMul4.Copy(&(pCurrCXForm->scale));
    pOutTransform->vColorAdd4.Copy(&(pCurrCXForm->translate));

    pOutTransform->vColorMul4.SetValue(AptColorHelper::Alpha, (int32_t)((pOutTransform->vColorMul4.GetA() * pCurTransform->vColorMul4.GetA()) / 255));
    pOutTransform->vColorMul4.SetValue(AptColorHelper::Red, (int32_t)((pOutTransform->vColorMul4.GetR() * pCurTransform->vColorMul4.GetR()) / 255));
    pOutTransform->vColorMul4.SetValue(AptColorHelper::Green, (int32_t)((pOutTransform->vColorMul4.GetG() * pCurTransform->vColorMul4.GetG()) / 255));
    pOutTransform->vColorMul4.SetValue(AptColorHelper::Blue, (int32_t)((pOutTransform->vColorMul4.GetB() * pCurTransform->vColorMul4.GetB()) / 255));

    pOutTransform->vColorAdd4.SetValue(AptColorHelper::Alpha, pOutTransform->vColorAdd4.GetA() + pCurTransform->vColorAdd4.GetA());
    pOutTransform->vColorAdd4.SetValue(AptColorHelper::Red, pOutTransform->vColorAdd4.GetR() + pCurTransform->vColorAdd4.GetR());
    pOutTransform->vColorAdd4.SetValue(AptColorHelper::Green, pOutTransform->vColorAdd4.GetG() + pCurTransform->vColorAdd4.GetG());
    pOutTransform->vColorAdd4.SetValue(AptColorHelper::Blue, pOutTransform->vColorAdd4.GetB() + pCurTransform->vColorAdd4.GetB());

    asm volatile(" # Concatenate transforms
                 lqc2 vf08,
                 0x00(% 0) #load vtx transform
                     lqc2 vf09,
                 0x10(% 0) #lqc2 vf11, 0x30(% 0) #vmulax.xyzw ACC, vf04, vf08x #stack.transform *= transform vmaddy.xyzw vf08, vf05, vf08y #vmulax.xyzw ACC, vf04, vf09x #vmaddy.xyzw vf09, vf05, vf09y #vmulax.xyzw ACC, vf04, vf11x #vmadday.xyzw ACC, vf05, vf11y #vmaddw.xyzw vf11, vf07, vf11w #vmove.xyzw vf04, vf08 #vmove.xyzw vf05, vf09 #vmove.xy vf07, vf11 #sqc2 vf4, 0x00(% 1) #store : Pos44[0].xyzw
                                                                                                                                                                                                                                                                                                                                                                                                         sqc2 vf5,
                                                                                                                                                                                                                                                                                                                                                                                                     0x10(% 1) #store : Pos44[1].xyzw
                                                                                                                                                                                                                                                                                                                                                                                                                            sqc2 vf6,
                                                                                                                                                                                                                                                                                                                                                                                                                        0x20(% 1) #store : Pos44[2].xyzw
                                                                                                                                                                                                                                                                                                                                                                                                                                               sqc2 vf7,
                                                                                                                                                                                                                                                                                                                                                                                                                                           0x30(% 1) #store : Pos44[3].xyzw
                                                                                                                                                                                                                                                                                                                                                                                                                                                              "
                 : : "r"(pMatrix), "r"(pOutTransform));
}
