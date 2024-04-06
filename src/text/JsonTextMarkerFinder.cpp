#include "TextMarkerFinder.hpp"
#include <wx/sstream.h>
#include <wx/log.h>
#include "../common/stringUtils.hpp"
#include "nlohmann/json.hpp"
#include "../common/println.hpp"


using json = nlohmann::json;

class JsonTextMarkerFinder: public TextMarkerFinder {
public:
bool Reset (const wxString& text) override;
virtual ~JsonTextMarkerFinder () {}
};

struct TMJSObj {
std::string key;
int index;
bool array;
TMJSObj (bool b): key(), index(0), array(b) {}
std::string nextkey () {
if (array) return fmt::format("[{}]", index++);
else return key;
}
};

struct TMJsonSax {
std::vector<TextMarker>& markers;
std::istream& in;
int level, lastPos;
const std::string& text;
std::vector<TMJSObj> objstack;

TMJsonSax (std::vector<TextMarker>& m, std::istream& i, const std::string& t):
markers(m), in(i), level(-1), lastPos(0), text(t)   {}

inline void addMarker (int start, int end, const std::string& key, const std::string& value, int level) {
if (level<0 || objstack.empty()) return;
size_t column=0, line=0;
positionToXY(text, start, column, line);
markers.emplace_back(start, end, line+1, level, U(key), U(fmt::format("{}: {}", key, value)) );
}

inline int regLastPos () {
lastPos = in.tellg();
return lastPos;
}

inline std::string curkey () {
return objstack.empty()? "" : objstack.back().nextkey();
}

bool null () { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, curkey(), "null", level);
return true; 
}

bool boolean (bool val) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, curkey(), val?"true":"false", level);
return true; 
}

bool number_integer (long long val) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, curkey(), std::to_string(val), level);
return true; 
}

bool number_unsigned (unsigned long long val) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, curkey(), std::to_string(val), level);
return true; 
}

bool number_float (double val, const std::string& s) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, curkey(), s, level);
return true; 
}

bool string (const std::string& s) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, curkey(), s, level);
return true; 
}

bool binary (const json::binary_t& val) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, curkey(), fmt::format("binary ({} bytes)", val.size()), level);
return true; 
}

bool key (const std::string& s) { 
objstack.back().key = s;
return true;
}

bool start_array (size_t n) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, curkey(), "array", level);
objstack.emplace_back(true);
level++;
return true; 
}

bool end_array () { 
level--;
objstack.pop_back();
return true; 
}

bool start_object (size_t n) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, curkey(), "object", level);
objstack.emplace_back(false);
level++;
return true; 
}

bool end_object () { 
level--;
objstack.pop_back();
return true; 
}

bool parse_error (size_t pos, const std::string& token, const std::exception& ex) { 
addMarker(pos, pos, "ERROR", ex.what(), 0);
return false; 
}

};//TMJsonSax



bool JsonTextMarkerFinder::Reset (const wxString& text) {
std::string utfText = U(text);
std::istringstream in(utfText);
TMJsonSax sax(markers, in, utfText);
markers.clear();
json::sax_parse(in, &sax);
return true;
}

void registerJsonTextMarkerFinder () {
TextMarkerFinder::Register("json", [](auto&p){ return new JsonTextMarkerFinder(); });
}
