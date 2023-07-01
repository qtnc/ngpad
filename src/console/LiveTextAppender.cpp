#include "LiveTextAppender.hpp"
#include "../app/App.hpp"

void AdjustLineEnding (wxString& text, int lineEnding);
unsigned long long mtime ();
int LuaSimpleEval (const wxString& eval, wxString* result);

LiveTextAppender::LiveTextAppender (wxTextEntryBase* te0, LiveTextAppender* otherAppender):
te(te0), realInsertionPoint(0), insertionPoint(otherAppender? otherAppender->GetInsertionPoint() : realInsertionPoint) 
{
realInsertionPoint = te->GetInsertionPoint();
}

LiveTextSubmitter::LiveTextSubmitter (wxTextEntryBase* te0, LiveTextAppender* otherAppender):
te(te0), realInsertionPoint(0), insertionPoint(otherAppender? otherAppender->GetInsertionPoint() : realInsertionPoint) 
{
realInsertionPoint = te->GetInsertionPoint();
}

CommandLiveTextSubmitter::CommandLiveTextSubmitter (wxOutputStream& out0, wxProcess* proc0, wxTextEntryBase* te0, wxMBConv& conv0, LiveTextAppender* otherAppender):
LiveTextSubmitter(te0, otherAppender), out(out0, wxEOL_NATIVE, conv0), proc(proc0)  
{ }

CommandLiveTextAppender::CommandLiveTextAppender (wxInputStream& in0, wxProcess* proc0, wxTextEntryBase* te0, wxMBConv& conv0, LiveTextAppender* otherAppender):
LiveTextAppender(te0, otherAppender), in(in0), proc(proc0), conv(conv0), thread(), stopped(false)
{
if (proc) proc->Bind(wxEVT_END_PROCESS, [&](auto&e){ Stop(); e.Skip(); });
thread = std::thread([&](){ Run(); });
}

CommandLiveTextAppender::~CommandLiveTextAppender () {
Stop();
if (thread.joinable()) thread.join();
}

void LiveTextAppender::Append (const wxString& s, bool say) {
long start, end, l = insertionPoint;
te->GetSelection(&start, &end);
te->SetInsertionPoint(insertionPoint);
te->WriteText(s);
insertionPoint = te->GetInsertionPoint();
if (say) wxGetApp().SayText(s);
if (start!=l || end!=l) te->SetSelection(start, end);
}

void LiveTextSubmitter::Submit () {
long last = te->GetLastPosition();
wxString text = te->GetRange(insertionPoint, last);
insertionPoint = last;
if (text.empty()) return;
Submit(text);
}

LuaConsoleTextAppender* LuaConsoleTextAppender::GetInstance () { return instance; };
LuaConsoleTextAppender* LuaConsoleTextAppender::instance = nullptr;

LuaConsoleTextAppender::LuaConsoleTextAppender (wxView* view0, wxTextEntryBase* te0):
LiveTextAppender(te0), view(view0) 
{
instance = this;
}

LuaConsoleTextAppender::~LuaConsoleTextAppender() {
if (instance==this) instance=nullptr;
}


LuaConsoleTextSubmitter::LuaConsoleTextSubmitter (wxTextEntryBase* te0, LiveTextAppender* otherAppender):
LiveTextSubmitter(te0, otherAppender) 
{}

void LuaConsoleTextSubmitter::Submit (const wxString& text) {
int result = 0;
wxString msg;
if (text==".clear\n") { }
else {
buffer += text;
result = LuaSimpleEval(buffer, &msg);
if (!msg.empty() && msg[msg.size() -1]!='\n') msg+='\n';
}
if (result!=10) buffer.clear();
else buffer += '\n';
auto appender = LuaConsoleTextAppender::GetInstance();
if (appender) {
appender->Append(msg);
appender->Append(result==10? "..." : ">>>", false);
}
}

void CommandLiveTextSubmitter::Submit (const wxString& text) {
if (text[text.size()-1]!='\n') out.WriteString(text + '\n');
else out.WriteString(text);
out.Flush();
}

void CommandLiveTextAppender::Run () {
char buf[4096] = {0};
int i=0;
auto lastReadTime = mtime();
while (!stopped && !in.Eof()) {
auto& c = buf[i];
auto time = mtime();
if (stopped) break;
if (in.CanRead()) {
i++;
c = 0;
in.Read(&c, 1);
lastReadTime = time;
}
else Sleep(10);
if (i<=0) continue;
if (c=='\n' || !c || i>=4094 || in.LastRead()!=1 || time-lastReadTime>250) {
buf[i] = 0;
wxString s(buf, conv, i);
AdjustLineEnding(s, LE_LF);
i=0;
RunEDT([=](){
Append(s);
});
}
}
stopped = true;
}

