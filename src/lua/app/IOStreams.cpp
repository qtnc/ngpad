#include<variant>
#include "base.hpp"
#include <wx/filename.h>
#include <wx/wfstream.h>
#include <wx/mstream.h>
#include<wx/process.h>
#include "MemoryInputStreamWithBuffer.hpp"
#include "../../common/println.hpp"

LuaRegisterReferenceType(wxInputStream);
LuaRegisterReferenceType(wxOutputStream);
LuaRegisterReferenceType(wxProcess);
LuaRegisterReferenceType(wxProcessEvent);
LuaRegisterValueType(wxFileName);

struct LuaEventHandler* BindLuaHandler (lua_State* L, int idx, wxEvtHandler& obj, wxEventType type, int id1, int id2);

static std::string wxisRead (wxInputStream& in, size_t max, size_t timeout, char until) {
std::string out;
char c = 0;
auto lastReadTime = mtime();
while (!in.Eof()) {
auto time = mtime();
if (in.CanRead()) {
c = 0;
in.Read(&c, 1);
lastReadTime = time;
if (until=='\n' && c=='\r') continue;
if (until && c==until) break;
out+=c;
}
else Sleep(10);
if (out.empty()) continue;
if (in.LastRead()!=1 || (max>0 && out.size()>=max) || (timeout>0 && time-lastReadTime>timeout)) break;
}
return out;
}

static std::string iReadAll (wxInputStream& in, size_t timeout) {
return wxisRead(in, 0, timeout, 0);
}

static std::string iReadLine  (wxInputStream& in, size_t timeout) {
return wxisRead(in, 0, timeout, '\n');
}

static std::string iReadN (wxInputStream& in, size_t max, size_t timeout) {
return wxisRead(in, max, timeout, 0);
}

static inline wxSeekMode modeFromString (const std::string& sMode) {
wxSeekMode mode = wxFromStart;
if (!sMode.empty()) switch(sMode[0]){
case 'b': case 'B': case 's': case 'S': mode = wxFromStart; break;
case 'c': case 'C': case 'r': case 'R': mode = wxFromCurrent; break;
case 'e': case 'E': mode = wxFromEnd; break;
}
return mode;
}

static wxFileOffset iSeek (wxInputStream& in, wxFileOffset pos, const std::string& mode) {
return in.SeekI(pos, modeFromString(mode));
}

static wxFileOffset oSeek (wxOutputStream& out, wxFileOffset pos, const std::string& mode) {
return out.SeekO(pos, modeFromString(mode));
}

static int oWrite (wxOutputStream& out, const std::string& s) {
out.WriteAll(s.data(), s.size());
return out.LastWrite();
}

static size_t iUnread (wxInputStream& in, const std::string& s) {
return in.Ungetch(s.data(), s.size());
}

static void iReadTo (wxInputStream& in, wxOutputStream& out) {
in.Read(out);
}

static void oWriteFrom (wxOutputStream& out, wxInputStream& in) {
out.Write(in);
}

static std::string oToString (wxOutputStream* oOut) {
if (!oOut) return "";
auto mOut = dynamic_cast<wxMemoryOutputStream*>(oOut);
if (!mOut) return "";
std::string buf;
buf.resize(mOut->TellO());
mOut->CopyTo( const_cast<char*>(buf.data()), buf.size());
return buf;
}

static wxInputStream* iFromBuffer (const std::string& s) {
auto buf = new std::string(s);
return new MemoryInputStreamWithBuffer(buf);
}

static inline wxInputStream* iOpen1 (const wxString& fn) {
return new wxFileInputStream(fn);
}

static inline wxOutputStream* oOpen1 (const wxString& fn) {
if (fn.empty()) return new wxMemoryOutputStream();
else return new wxFileOutputStream(fn);
}

static wxInputStream* iOpen (std::variant<wxString, wxFileName> fn) {
struct Visitor {
wxInputStream* operator() (const wxString& s) { 
println("iOpen with string: {}", U(s));
return iOpen1(s); 
}
wxInputStream* operator() (const wxFileName& fn) { 
println("iOpen with path: {}", U(fn.GetAbsolutePath()));
return iOpen1(fn.GetAbsolutePath()); 
}
};
return std::visit(Visitor(), fn);
}

static wxOutputStream* oOpen (std::variant<wxString, wxFileName> fn) {
struct Visitor {
wxOutputStream* operator() (const wxString& s) { return oOpen1(s); }
wxOutputStream* operator() (const wxFileName& fn) { return oOpen1(fn.GetAbsolutePath()); }
};
return std::visit(Visitor(), fn);
}

static wxProcess* ProcessCreate (const wxString& cmd) {
return wxProcess::Open(cmd, wxEXEC_ASYNC | wxEXEC_HIDE_CONSOLE);
}

static int ProcessBindTerminate (lua_State* L) {
auto& proc = lua_get<wxProcess&>(L, 1);
auto re = BindLuaHandler(L, -1, proc, wxEVT_END_PROCESS, wxID_ANY, wxID_ANY);
lua_pushlightuserdata(L, re);
return 1;
}

export int luaopen_InputStream (lua_State* L) {
lua_pushglobaltable(L);
//T Input stream class
Binding::LuaClass<wxInputStream>(L, "InputStream")
//M Checks if the stream can be read without blocking
//R boolean: true if a read operation can be done without blocking
.method("canRead", &wxInputStream::CanRead)
//M Read all the contents til the end of the stream
//P timeout: integer: 0: the maximum amount of time (in miliseconds) to wait for contents, 0 or negative means infinite
//R string: content read from the stream. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.
.method("readAll", &iReadAll, {"timeout"})
//M Read a line of text from the stream, until \n or \r\n, without including it in the returned string. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.
//P timeout: integer: 0: the maximum amount of time (in miliseconds) to wait for contents, 0 or negative means infinite
//R string: content read from the stream. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.
.method("readLine", &iReadLine, {"timeout"})
//M Read contents from the stream
//P count: integer: nil: maximum number of bytes to read from the stream
//P timeout: integer: 0: the maximum amount of time (in miliseconds) to wait for contents, 0 or negative means infinite
//R string: content read from the stream. It may be less than the maximum requested, including 0. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.
.method("read", &iReadN, {"count", "timeout"})
//M Seek in the stream
//P position: integer: nil: position to seek to
//P mode: string: 'current': one of 'begin', 'current' or 'end' to seek respectively from the beginning of the stream, its current position, or from the end.
//R integer: the new position of the stream, or -1 if unknown
.method("seek", &iSeek)
//M Tell the current position of the stream
//R integer: the current position of the stream, or -1 if unknown
.method("tell", &wxInputStream::TellI)
//M Put back some content in the stream. That content will be read again.
//P data: string: nil: data to put back in the stream
.method("unread", &iUnread)
//M Write all content read from this input stream to an output stream
//P out: OutputStream: nil: Output stream to write to
.method("readTo", &iReadTo)
//M Closes the stream
//.method("close", &wxInputStream::Close)
//M Closes the stream
//.method("__close", &wxInputStream::Close)
//F Create an input stream from an input string
//P content: string: '': content of the input stream
//R InputStream: created input stream
.method("from", &iFromBuffer)
//C Create a new input stream
//P file: string|Path: nil: file name or path object referencing the file to open
.constructor(&iOpen, {"file"})
.destructor()
.pop();
lua_getglobal(L, "InputStream");
return 1;
}

export int luaopen_OutputStream (lua_State* L) {
lua_pushglobaltable(L);
//T Output stream class
Binding::LuaClass<wxOutputStream>(L, "OutputStream")
//M Write content to the stream
//P data: string: nil: data to write
//R integer: number of bytes written
.method("write", &oWrite)
//M Write all data read from an input stream
//P input: InputStream: nil: input stream to read from
.method("writeFrom", &oWriteFrom)
//M Seek in the stream
//P position: integer: nil: position to seek to
//P anse: string: 'current': one of 'begin', 'current' or 'end' to seek respectively from the beginning of the stream, its current position, or from the end.
//R integer: the new position of the stream, or -1 if unknown
.method("seek", &oSeek)
//M Tells the current position of the stream
//R integer: the position of the stream, or -1 if unknown
.method("tell", &wxOutputStream::TellO)
//M Close the stream
.method("close", &wxOutputStream::Close)
//M Close the stream
.method("__close", &wxOutputStream::Close)
//M Converts all the content written to this stream into a string. It works only for in-memory buffers created with the no-args constructor.
//R string: content of the stream as a string. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.
.method("toString", &oToString)
//C Create a new output stream. You can open a file by passing one parameter, or create an in-memory stream by passing no parameter.
//P file: string|Path: nil: File name or path referencing a file to open for writing, or nil to create an in-memory stream
.constructor(&oOpen, {"file"})
.destructor()
.pop();
lua_getglobal(L, "OutputStream");
return 1;
}

export int luaopen_ProcessEvent (lua_State* L) {
lua_getglobal(L, "Event");
lua_pop(L, 1);
lua_pushglobaltable(L);
//T Event notified when a process terminates
Binding::LuaClass<wxProcessEvent>(L, "ProcessEvent")
.parent<wxEvent>()
//G integer: exit code of the process, usually 0 for success, any positive value when the process failed, and negative value when the process couldn't be executed at all
.getter("exitCode", &wxProcessEvent::GetExitCode)
//G integer: Process ID of the process
.getter("pid", &wxProcessEvent::GetPid)
.pop();
lua_getglobal(L, "ProcessEvent");
return 1;
}

export int luaopen_Process (lua_State* L) {
lua_getglobal(L, "InputStream");
lua_getglobal(L, "OutputStream");
lua_getglobal(L, "ProcessEvent");
lua_pop(L, 3);
lua_pushglobaltable(L);
//T Process class allowing to execute external commands
Binding::LuaClass<wxProcess>(L, "Process")
//C Create and launch a new process
//P command: string: nil: command-line containing the path to the executable to run as well as all command-line parameters
.constructor(&ProcessCreate, {"command"})
//G InputStream: input stream connected to the standard output stream of the process
.getter("inputStream", &wxProcess::GetInputStream)
//G OutputStream: output stream connected to the standard input stream of the process
.getter("outputStream", &wxProcess::GetOutputStream)
//G InputStream: input stream connected to the standard error stream of the process
.getter("errorStream", &wxProcess::GetErrorStream)
//G boolean: tells if there is data available on the input stream
.boolGetter("inputAvailable", &wxProcess::IsInputAvailable)
//G boolean: tells if there is data available on the error stream
.boolGetter("errorAvailable", &wxProcess::IsErrorAvailable)
//M Close the output stream connected to the standard input stream of the process, indicating to it that you aren't going to send data anymore
.method("closeOutput", &wxProcess::CloseOutput)
//G integer: process ID of the process
.getter("pid", &wxProcess::GetPid)
//M Attempt to activate the main window of the process
.method("activate", &wxProcess::Activate)
//M Bind an event listener that will be triggered when the process terminates
//P handler: function: nil: handler that will be called when the process terminates
.method("bind", &ProcessBindTerminate)
.pop();
lua_getglobal(L, "Process");
return 1;
}
