#pragma once

// Each version segment should fit into a single unsigned byte (0-255).
#define APT_VERSION_MAJOR 3
#define APT_VERSION_MAJOR_REVISION 2
#define APT_VERSION_MINOR 2
#define APT_VERSION_MINOR_REVISION 0

#define APT_VERSION (((APT_VERSION_MAJOR & 0xff) << 24) | ((APT_VERSION_MAJOR_REVISION & 0xff) << 16) | ((APT_VERSION_MINOR & 0xff) << 8) | (APT_VERSION_MINOR_REVISION & 0xff))

#define APT_VERSION_AT_LEAST(major, majorRev, minor, minorRev) (APT_VERSION >= (((major) << 24) | ((majorRev) << 16) | ((minor) << 8) | (minorRev)))
