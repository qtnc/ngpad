#ifndef _____LIVE_TEXT_APPENDER
#define _____LIVE_TEXT_APPENDER
#include "../common/wxUtils.hpp"
#include <wx/stream.h>
#include <wx/txtstrm.h>
#include <wx/process.h>
#include<thread>

class LiveTextAppender {
private:
wxTextEntryBase* te;
int& insertionPoint;
int realInsertionPoint;

public:
LiveTextAppender (wxTextEntryBase* te, LiveTextAppender* otherAppender = nullptr);
int& GetInsertionPoint () { return insertionPoint; }
virtual void Append (const wxString& text, bool say = true);
virtual ~LiveTextAppender () {}
};

class CommandLiveTextAppender: public LiveTextAppender {
private:
wxInputStream& in;
wxProcess* proc;
wxMBConv& conv;
std::thread thread;
volatile bool stopped;

public:
CommandLiveTextAppender (wxInputStream& in, wxProcess* proc, wxTextEntryBase* te, wxMBConv& conv, LiveTextAppender* otherAppender = nullptr);
void Run ();
void Stop () { stopped=true; }
virtual ~CommandLiveTextAppender ();
};

class LiveTextSubmitter {
private:
wxTextEntryBase* te;
int& insertionPoint;
int realInsertionPoint;

public:
LiveTextSubmitter (wxTextEntryBase* te, LiveTextAppender* otherAppender=nullptr);
virtual void Submit ();
virtual void Submit (const wxString& text) = 0;
virtual ~LiveTextSubmitter() {}
};

class CommandLiveTextSubmitter: public LiveTextSubmitter  {
private:
wxTextOutputStream out;
wxProcess* proc;

public:
CommandLiveTextSubmitter (wxOutputStream& out, wxProcess* proc, wxTextEntryBase* te, wxMBConv& conv, LiveTextAppender* otherAppender = nullptr);
void Submit (const wxString& text) override;
};

class LuaConsoleTextAppender: public LiveTextAppender {
private:
static LuaConsoleTextAppender* instance;
struct wxView* view;

public:
static LuaConsoleTextAppender* GetInstance ();
LuaConsoleTextAppender (wxView* view0, wxTextEntryBase* te0);
wxView* GetView () { return view; }
virtual ~LuaConsoleTextAppender();
};

class LuaConsoleTextSubmitter: public LiveTextSubmitter {
private:
wxString buffer;

public:
LuaConsoleTextSubmitter (wxTextEntryBase* te0, LiveTextAppender* otherAppender = nullptr);
void Submit (const wxString& text) override;
};


#endif
