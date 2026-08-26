#include "_AptInternalDefines.h"
#if APT_USE_FILTERS
#if defined(APT_SYSTEM_BIG_ENDIAN)

APT_FILTER_ALIGNED(unsigned char, g_arrFiltersAptFile[]) = {
#if (APT_PLATFORM_PTR_SIZE == 8)
#include "./filters_big_apt_64.h"
#else
#include "./filters_big_apt.h"
#endif
};

APT_FILTER_ALIGNED(unsigned char, g_arrFiltersConstFile[]) = {
#if (APT_PLATFORM_PTR_SIZE == 8)
#include "./filters_big_const_64.h"
#else
#include "./filters_big_const.h"
#endif
};

#endif
#endif
