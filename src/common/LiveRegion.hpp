#ifndef _____LIVE_REGION_HPP
#define _____LIVE_REGION_HPP
#include "wxUtils.hpp"

#define LIVE_REGION_OFF 0
#define LIVE_REGION_POLITE 1
#define LIVE_REGION_ASSERTIVE 2

#ifdef __WIN32
bool SetLiveRegion (wxWindow* win, int value = LIVE_REGION_POLITE);
bool LiveRegionUpdated (wxWindow* win);
#else
bool SetLiveRegion (wxWindow* win, int value = LIVE_REGION_POLITE) { return false; }
bool LiveRegionUpdated (wxWindow* win) { return false; }
#endif

#endif
