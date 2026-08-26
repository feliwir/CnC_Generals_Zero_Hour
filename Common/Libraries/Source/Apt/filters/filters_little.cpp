#include "_AptInternalDefines.h"

#if APT_USE_FILTERS
#if !defined(APT_SYSTEM_BIG_ENDIAN)

#if defined(APT_DECOUPLED_RENDERING)
APT_FILTER_ALIGNED(unsigned char, g_arrFiltersAptFile[]) = {
#if (APT_PLATFORM_PTR_SIZE == 8)
#include "./filters_little_apt_64.h"
#else
#include "./filters_little_apt.h"
#endif
};

APT_FILTER_ALIGNED(unsigned char, g_arrFiltersConstFile[]) = {
#if (APT_PLATFORM_PTR_SIZE == 8)
#include "./filters_little_const_64.h"
#else
#include "./filters_little_const.h"
#endif
};
#else
APT_FILTER_ALIGNED(unsigned char, g_arrFiltersAptFile[]) = {
#if (APT_PLATFORM_PTR_SIZE == 8)
#include "./filters_little_apt_coupled_64.h"
#else
#include "./filters_little_apt_coupled.h"
#endif
};

APT_FILTER_ALIGNED(unsigned char, g_arrFiltersConstFile[]) = {
#if (APT_PLATFORM_PTR_SIZE == 8)
#include "./filters_little_const_coupled_64.h"
#else
#include "./filters_little_const_coupled.h"
#endif
};
#endif

#endif
#endif
