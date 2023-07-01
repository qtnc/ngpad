#include "WebViewDialog.hpp"
#include "wx/webview.h"
#include <wx/uiaction.h>
#ifdef __WIN32
#include "wx/msw/webview_ie.h"
#endif

extern unsigned long long mtime ();

bool WebViewDialog::initialized = false;

void WebViewDialog::initialize () {
if (initialized) return;
#ifdef __WIN32
wxWebViewIE::MSWSetEmulationLevel(wxWEBVIEWIE_EMU_IE11);
#endif
initialized=true;
}

WebViewDialog::WebViewDialog (wxWindow* parent, const wxString& initialURL):
wxDialog(parent, wxID_ANY, "WebViewDialog", wxDefaultPosition, wxDefaultSize, wxCAPTION | wxRESIZE_BORDER | wxSYSTEM_MENU | wxCLOSE_BOX)
{
curURL = initialURL;
magicClicked=false;
initialize();
webview = wxWebView::New(this, wxID_ANY, "about:blank", wxDefaultPosition, wxSize(640, 480), wxWebViewBackendDefault);
webview->EnableContextMenu(false);
SetPage("<html><h1>Loading...</h1></html>");

auto sizer = new wxBoxSizer(wxVERTICAL);
sizer->Add(webview, 1, wxEXPAND);

Bind(wxEVT_INIT_DIALOG, &WebViewDialog::OnInitDialog, this);
Bind(wxEVT_ACTIVATE, &WebViewDialog::OnActivate, this);
webview->Bind(wxEVT_WEBVIEW_TITLE_CHANGED, &WebViewDialog::OnTitleChanged, this);
webview->Bind(wxEVT_WEBVIEW_NEWWINDOW, &WebViewDialog::OnNewWindow, this);
webview->Bind(wxEVT_WEBVIEW_NAVIGATING, &WebViewDialog::OnNavigating, this);
webview->Bind(wxEVT_WEBVIEW_LOADED, &WebViewDialog::OnDocumentLoaded, this);
webview->Bind(wxEVT_CHAR_HOOK, &WebViewDialog::OnWebviewCharHook, this);
if (initialURL[0]=='<') SetPage(initialURL);
else LoadURL(initialURL); 
SetSizerAndFit(sizer);
webview->SetFocus();
}

void WebViewDialog::SetPage (const wxString& str) {
curURL = "about:blank";
webview->SetPage(str, curURL);
}

void WebViewDialog::LoadURL (const wxString& url) {
webview->LoadURL(url);
curURL = url;
}

void WebViewDialog::DoMagicClick () {
if (magicClicked) return;
webview->SetFocus();
wxUIActionSimulator bot;
auto position = webview->GetPosition();
auto size = webview->GetSize();
position = webview->ClientToScreen(position);
position.x += 10; position.y += 10;
bot.MouseMove(position);
bot.MouseClick();
magicClicked=true;
}

void WebViewDialog::Open (wxWindow* parent, const wxString& url) {
WebViewDialog wd(parent, url);
wd.ShowModal();
}

void WebViewDialog:: OnWebviewCharHook (wxKeyEvent& e) {
if (e.GetModifiers()==0 && e.GetKeyCode()==WXK_ESCAPE) EndModal(wxOK);
if (e.GetModifiers()==wxMOD_CONTROL && e.GetKeyCode()=='N') wxLaunchDefaultBrowser(curURL, wxBROWSER_NEW_WINDOW);
e.Skip();
}

void WebViewDialog:: OnTitleChanged (wxWebViewEvent& e) {
SetTitle(e.GetString());
}

void WebViewDialog:: OnNewWindow (wxWebViewEvent& e) {
OnURLClicked(e.GetURL(), true);
}

void WebViewDialog:: OnNavigating (wxWebViewEvent& e) {
OnURLClicked(e.GetURL(), false); 
loadTime=mtime(); 
}

void WebViewDialog:: OnDocumentLoaded (wxWebViewEvent& e) {
curURL = e.GetURL();
}

void WebViewDialog::OnInitDialog (wxInitDialogEvent& e) {
DoMagicClick();  
e.Skip();
}

void WebViewDialog:: OnActivate (wxActivateEvent& e) {
if (e.GetActive()) {
DoMagicClick(); 
}
else {
magicClicked=false;  
}
e.Skip(); 
}

void WebViewDialog::OnURLClicked (const wxString& url, bool newWindow) {
if (newWindow) wxLaunchDefaultBrowser(url, wxBROWSER_NEW_WINDOW);
else curURL = url;
}
