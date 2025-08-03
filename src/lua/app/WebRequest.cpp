#include<variant>
#include "base.hpp"
#include <wx/webrequest.h>
#include "../../app/App.hpp"
#include "MemoryInputStreamWithBuffer.hpp"
#include "../../common/println.hpp"

LuaRegisterReferenceType(wxInputStream);
LuaRegisterReferenceType(wxOutputStream);
LuaRegisterValueType(wxWebRequest);
LuaRegisterValueType(wxWebResponse);
LuaRegisterTypeAlias(wxWebRequestBase, wxWebRequest);

std::unordered_map<int, int> reqmap;

void wrSetData (wxWebRequest& request, std::variant<std::string, wxInputStream*> input, const wxString& contentType, wxFileOffset size) {
struct Visitor {
wxWebRequest& request;
const wxString& contentType;
wxFileOffset& size;
Visitor (wxWebRequest& request, const wxString& contentType, wxFileOffset& size): request(request), contentType(contentType), size(size)  {}
void operator() (wxInputStream* in) {
if (!in || in->IsOk()) return;
request.SetData(in, contentType, size);
}
void operator() (const std::string& s) {
if (s.empty()) return;
size = s.size();
auto ns = new std::string(s);
wxInputStream* in = new MemoryInputStreamWithBuffer(ns);
operator()(in);
}
};
std::visit(Visitor(request, contentType, size), input);
}

wxWebRequest wrCreate (const wxString& url, const wxString& method, std::variant<std::string, wxInputStream*> data, const wxString& contentType, wxFileOffset size) {
static int id = 0;
wxWebRequest request = wxWebSession::GetDefault().CreateRequest( &wxGetApp(), url, ++id);
if (!contentType.empty()) wrSetData(request, data, contentType, size);
if (!method.empty()) request.SetMethod(method);
return request;
}

static int wrStart (lua_State* L) {
auto& request = lua_get<wxWebRequest&>(L, 1);
reqmap[request.GetId()] = luaL_ref(L, LUA_REGISTRYINDEX);
request.Start();
return 0;
}

static void requestStateChanged (wxWebRequestEvent& e) {
switch(e.GetState()) {
case wxWebRequest::State_Completed:
case wxWebRequest::State_Failed:
case wxWebRequest::State_Cancelled:
auto it = reqmap.find(e.GetRequest().GetId());
if (it!=reqmap.end()) {
auto response = e.GetResponse();
int ref = it->second;
RunEDT([=](){
lua_State* L = wxGetApp() .GetLuaState();
lua_rawgeti(L, LUA_REGISTRYINDEX, ref);
lua_push(L, response);
if (LUA_OK != luaL_call(L, 1, 1)) throw std::runtime_error(lua_tostring(L, -1));
luaL_unref(L, LUA_REGISTRYINDEX, ref);
});
reqmap.erase(it);
}
break;
}}


export int luaopen_WebRequest (lua_State* L) {
wxGetApp() .Bind(wxEVT_WEBREQUEST_STATE, &requestStateChanged);

lua_pushglobaltable(L);
//T Object containing a response to a web request
Binding::LuaClass<wxWebResponse>(L, "WebResponse")
//G integer: size in bytes of the response, or -1 if unknown
.getter("contentLength", &wxWebResponse::GetContentLength)
//M Get a response header 
//P name: string: nil: header name
//R string: header value, or empty string if not found
.method("getHeader", &wxWebResponse::GetHeader)
//G string: Content type of the response
.getter("contentType", &wxWebResponse::GetMimeType)
//G integer: HTTP response code of the response, for example 200
.getter("status", &wxWebResponse::GetStatus)
//G string: Text of the HTTP response code, for example 'OK'
.getter("statusText", &wxWebResponse::GetStatusText)
//G InputStream: Get an input stream for reading the response content
.getter("responseStream", &wxWebResponse::GetStream)
//G string: suggested file name for downloading a file
.getter("suggestedFileName", &wxWebResponse::GetSuggestedFileName)
//G string: final URL of the resource. This can be different from the original requested URL in case of redirection.
.getter("url", &wxWebResponse::GetURL)
//G string: file name where the response is stored if storing in a file was requested
.getter("responseFileName", &wxWebResponse::GetDataFile)
//G string: content of the response as a string. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.
.getter("responseText", &wxWebResponse::AsString)
.pop();

//T Class allowing to make web requests
Binding::LuaClass<wxWebRequest>(L, "WebRequest")
//C Create a new web request
//P url: string: nil: URL of the request
//P method: string: 'GET': HTTP method of the request
//P data: string|InputStream: nil: a string or an input stream to send as request content. The method is automatically changed from 'GET' to 'POST'  if necessary.
//P contentType: string: nil: Content-Type for request content
//P contentLength: integer: nil: length of the request content in bytes
.constructor(&wrCreate, {"url", "method", "data", "contentType", "contentLength"})
//M Set the request body
//P data: string|InputStream: nil: string or input stream of the request body
//P contentType: string: nil: Content-Type of the request body
//P contentLength: integer: nil: length of the data in bytes
.method("setData", &wrSetData)
//M SEt a request header
//P name: string: nil: header name
//P value: string: nil: header value
.method("setHeader", &wxWebRequest::SetHeader)
//S string: HTTP method of the request
.setter("method", &wxWebRequest::SetMethod)
//A boolean: whether to ignore SSL/TLS certificate verifications. Note that disabling SSL/TLS certificate verifications make requests less secure.
.boolProperty("verifyDisabled", &wxWebRequest::IsPeerVerifyDisabled, &wxWebRequest::DisablePeerVerify)
//M Effectively launch the web request. The request is launched asynchronously, and a callback function is called when the response is fully received.
//P callback: function: nil: function to call when the response is received. The fonction receive a WebResponse object as parameter.
.method("submit", &wrStart)
.destructor()
.pop();
lua_getglobal(L, "WebRequest");
return 1;
}
