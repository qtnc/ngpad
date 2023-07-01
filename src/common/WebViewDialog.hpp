#ifndef ___WEBVIEWPOPUP___1
#define ___WEBVIEWPOPUP___1
#include "wxUtils.hpp"
#include<memory>
#include<cstdint>

struct WebViewDialog: wxDialog {
static bool initialized;
struct wxWebView* webview;
uint64_t loadTime;
bool magicClicked;
wxString curURL;

WebViewDialog  (wxWindow* parent, const wxString& initialURL = "about:blank");
void SetPage (const wxString& html);
void LoadURL (const wxString& url);
void DoMagicClick ();

void OnURLClicked (const wxString& url, bool newWindow);
void OnActivate (wxActivateEvent& e);
void OnInitDialog (wxInitDialogEvent& e);
void OnDocumentLoaded (struct wxWebViewEvent& e);
void OnNavigating (wxWebViewEvent& e);
void OnTitleChanged (wxWebViewEvent& e);
void OnNewWindow (wxWebViewEvent& e);
void OnWebviewCharHook (wxKeyEvent& e);

static void initialize ();
static void Open (wxWindow* parent, const wxString& url);
};

#endif
