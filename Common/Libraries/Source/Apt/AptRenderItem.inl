/**
    This is a placeholder file to implement functions for the AptRenderItem class as needed.
*/

const AptCXForm *AptRenderItem::GetColorMatrixConst() const
{
    if (mpColorMatrix == NULL)
    {
        return &gIdentityCXForm;
    }
    return mpColorMatrix;
}

const AptMatrix *AptRenderItem::GetPositionMatrixConst() const
{
    if (mpPositionMatrix == NULL)
    {
        return &gIdentityMatrix;
    }
    return mpPositionMatrix;
}
