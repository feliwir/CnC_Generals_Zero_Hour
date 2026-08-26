#pragma once

class AptMovieClip : public AptObject
{
  public:
    APT_VALUE_GC_NEW_DELETE_OPERATORS

    AptMovieClip() : AptObject(AptVFT_MovieClip)
    {
    }

  protected:
    APT_INLINE
    virtual ~AptMovieClip() {}
};
