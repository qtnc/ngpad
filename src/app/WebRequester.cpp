#include "WebRequester.hpp"
#include "App.hpp"
#include "../common/MemoryInputStreamWithBuffer.hpp"
#include "../common/println.hpp"

std::unique_ptr<WebRequester> WebRequester::instance;

static WebRequester& Get ();
void WebRequester::SetBody (wxWebRequest& request, const wxString& contentType, const std::string& data) {
auto ns = new std::string(data);
wxInputStream* in = new MemoryInputStreamWithBuffer(ns);
if (in->IsOk()) request.SetData(in, contentType, data.size());
else delete in;
}

wxWebRequest WebRequester::Create (const wxString& url, const wxString& method) {
wxWebRequest request = wxWebSession::GetDefault().CreateRequest( &wxGetApp(), url, ++id);
if (!method.empty()) request.SetMethod(method);
return request;
}

int WebRequester::Send (wxWebRequest& request, const WebRequestEventListener& listener, wxCriticalSection* cs) {
int reqId = request.GetId();
listeners[reqId] = { listener, cs };
request.Start();
return reqId;
}

void WebRequester::OnRequestStateChanged (wxWebRequestEvent& e) {
switch(e.GetState()) {
case wxWebRequest::State_Completed:
case wxWebRequest::State_Failed:
case wxWebRequest::State_Cancelled:
auto it = listeners.find(e.GetRequest().GetId());
if (it!=listeners.end()) {
auto& info = it->second;
if (info.cs) RunEDTSync(*info.cs, [=](){ info.listener(e); });
else RunEDTSync([=](){ info.listener(e); });
listeners.erase(it);
}
break;
}}

WebRequester::WebRequester (): listeners(), id(0) {
wxGetApp() .Bind(wxEVT_WEBREQUEST_STATE, &WebRequester::OnRequestStateChanged, this);
}

WebRequester& WebRequester::Get () {
if (!instance) instance = std::make_unique<WebRequester>();
return *instance;
}

