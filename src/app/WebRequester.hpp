#ifndef ____WEBREQUESTER_HPP
#define ____WEBREQUESTER_HPP
#include "constants.hpp"
#include "../common/wxUtils.hpp"
#include "Worker.hpp"
#include <wx/webrequest.h>
#include<string>
#include<vector>
#include<unordered_map>
#include<functional>
#include<memory>


class WebRequester {
public: 
typedef std::function<void(const wxWebRequestEvent&)> WebRequestEventListener;

private:
static std::unique_ptr<WebRequester> instance;
struct ListenerInfo { WebRequestEventListener listener; wxCriticalSection* cs; };
std::unordered_map<int, ListenerInfo> listeners;
int id;

private:
void OnRequestStateChanged (wxWebRequestEvent& e);

public:
static WebRequester& Get ();
WebRequester (); // need to make public because of std::make_unique
wxWebRequest Create (const wxString& url, const wxString& method = wxEmptyString);
void SetBody (wxWebRequest& request, const wxString& contentType, const std::string& data);
int Send (wxWebRequest& request, const WebRequestEventListener& listener, wxCriticalSection* cs = nullptr);
};

#endif
