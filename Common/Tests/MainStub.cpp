#include <SDL3/SDL.h>

const char *gAppPrefix = ""; /// So WB can have a different debug log file name.
SDL_Window *ApplicationWindow = NULL;

// Normally defined in the game's Main entry point (SDL3Main.cpp); the string
// tables are referenced by GameText.cpp in the engine library.
const char *g_strFile = "data\\Generals.str";
const char *g_csfFile = "data\\%s\\Generals.csf";

// TheSubsystemList is defined by the engine's GameEngine.cpp, which is linked
// from the game library.
