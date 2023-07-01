#ifdef __WIN32
#define UNICODE
#include<winsock2.h>
#include<windows.h>
#include "objbase.h"
#include "oleacc.h"
#include "uiautomation.h"
#include "wxUtils.hpp"
#include "LiveRegion.hpp"

#define S_GUID(n,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) extern "C" const _GUID n = {l,w1,w2, {b1,b2,b3,b4,b5,b6,b7,b8}};

__CRT_UUID_DECL(IAccPropServices,0x6e26e776,0x04f0,0x495d,0x80,0xe4,0x33,0x30,0x35,0x2e,0x31,0x69);
S_GUID(CLSID_AccPropServices,0xb5f8350b,0x0548,0x48b1,0xa6,0xee,0x88,0xbd,0x00,0xb4,0xa5,0xe7);
S_GUID(IID_IAccPropServices,0x6e26e776,0x04f0,0x495d,0x80,0xe4,0x33,0x30,0x35,0x2e,0x31,0x69);
S_GUID(LiveSetting_Property_GUID, 0xc12bcd8e, 0x2a8e, 0x4950, 0x8a, 0xe7, 0x36, 0x25, 0x11, 0x1d, 0x58, 0xeb);
DEFINE_GUID(CLSID_AccPropServices,0xb5f8350b,0x0548,0x48b1,0xa6,0xee,0x88,0xbd,0x00,0xb4,0xa5,0xe7);
  DEFINE_GUID(IID_IAccPropServices,0x6e26e776,0x04f0,0x495d,0x80,0xe4,0x33,0x30,0x35,0x2e,0x31,0x69);
DEFINE_GUID(LiveSetting_Property_GUID, 0xc12bcd8e, 0x2a8e, 0x4950, 0x8a, 0xe7, 0x36, 0x25, 0x11, 0x1d, 0x58, 0xeb);

#define EVENT_OBJECT_LIVEREGIONCHANGED 0x8019

#define Assertive 2
#define Polite 1

static IAccPropServices* _pAccPropServices = NULL;

 
bool lrInit () {
if (_pAccPropServices) return true;
// Run when the UI is created.
HRESULT hr = -1;


static bool coInit = false;
if (!coInit) {
hr = CoInitialize(NULL);
coInit = SUCCEEDED(hr) || hr==S_FALSE;
if (!coInit) return false;
}

hr = CoCreateInstance(
    CLSID_AccPropServices,
    nullptr,
    CLSCTX_INPROC,
    IID_PPV_ARGS(&_pAccPropServices));
return SUCCEEDED(hr);
}


bool lrSetLiveRegion (HWND hwnd, int value) {
   // Set up the status label to be an assertive LiveRegion. The assertive property 
	// is used to request that the screen reader announces the update immediately. 
	// And alternative setting of polite requests that the screen reader completes 
	// any in-progress announcement before announcing the LiveRegion update.

HRESULT hr = -1; 
    VARIANT var;
    var.vt = VT_I4;
    var.lVal = value; 
 
    hr = _pAccPropServices->SetHwndProp(
        hwnd,
        OBJID_CLIENT,
        CHILDID_SELF,
        LiveSetting_Property_GUID, 
        var);
return SUCCEEDED(hr);
}
 
 
bool lrLiveRegionUpdated (HWND hwnd) {
// Run when the text on the label changes. Raise a UIA LiveRegionChanged 
// event so that a screen reader is made aware of a change to the LiveRegion.
// Make sure the updated text is set on the label before making this call.
NotifyWinEvent(
    EVENT_OBJECT_LIVEREGIONCHANGED, 
    hwnd, 
    OBJID_CLIENT, 
    CHILDID_SELF);
return true;
}

bool SetLiveRegion (wxWindow* win, int value) {
return lrInit() && lrSetLiveRegion((HWND)(win->GetHandle()), value);
}

bool LiveRegionUpdated (wxWindow* win) {
return lrLiveRegionUpdated((HWND)(win->GetHandle()));
}

#endif
