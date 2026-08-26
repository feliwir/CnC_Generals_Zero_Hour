#pragma once

/** @brief Returns the currently set sim target */
AptTarget *GetTargetSim()
{
    return AptGetLib()->mpCurrentTargetSim;
}

/** @brief Returns the currently set render target */
AptTarget *GetTargetRender()
{
    return AptGetLib()->mpCurrentTargetRender;
}
