#pragma once

#define APTMOVIE_MAXDUPLICATES 256
#define APTMOVIE_LABELHASHSIZE 2

struct AptFrame;
struct AptConstFile;
struct AptDisplayList;
class AptPseudoDisplayList;

class AptNativeHash;
class AptCIH;

/** @brief A movie is a "sprite" in swf terminology (and a Movie Clip in the flash gui). */
struct AptMovie
{
    int nFrames;
    AptFrame *aFrames;
    AptNativeHash *phLabels;

  public:
    void resolve(unsigned char *pBase, AptConstFile *aConstantFile, intptr_t *pnCurrentConstantIndex);
    void unresolve(unsigned char *pBase, intptr_t *pnCurrentConstantIndex);
    void DoTemporaryFrameControls(AptPseudoDisplayList *pPseudoDisplayList, int nFrame) const;
    void doFrameControls(AptDisplayList *pDisplayList, AptCIH *pSprInst, int nFrame) const;
    void queueFrameActions(AptCIH *pInst, int nFrame) const;
    void runFrameActions(AptCIH *pInst, int nFrame) const;
    int labelToFrame(const AptNativeString *pLabel) const; // returns the frame labeled accordingly, returns -1 if not found
};
