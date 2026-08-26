#include "_Apt.h"

#include "MainInline.h"

const char *AptDebugVersionString()
{
    // XXX note that this string is sneakily formatted for a lame sed script in makedevrelease.pl
    return "Apt-3.02.02" // version
           " built on "
           "5/23/2014 2:30:37 PM" // date
        ;
}
