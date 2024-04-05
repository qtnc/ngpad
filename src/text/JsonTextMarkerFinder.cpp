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

struct JsonSax {
std::vector<TextMarker>& markers;
std::istream& in;
int level, lastPos;
std::string lastKey;
const std::string& text;

JsonSax (std::vector<TextMarker>& m, std::istream& i, const std::string& t):
markers(m), in(i), level(-1), lastKey(), lastPos(0), text(t)   {}

void addMarker (int start, int end, const std::string& key, const std::string& value, int level) {
if (level<0 || key.empty()) return;
std::string kv;
if (value.empty()) kv = key;
else kv = key + ": " + value;
size_t column=0, line=0;
positionToXY(text, start, column, line);
markers.emplace_back(start, end, line, level, U(key), U(kv) );
}

int regLastPos () {
lastPos = in.tellg();
return lastPos;
}

bool null () { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, lastKey, "null", level);
return true; 
}

bool boolean (bool val) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, lastKey, val?"true":"false", level);
return true; 
}

bool number_integer (long long val) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, lastKey, std::to_string(val), level);
return true; 
}

bool number_unsigned (unsigned long long val) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, lastKey, std::to_string(val), level);
return true; 
}

bool number_float (double val, const std::string& s) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, lastKey, s, level);
return true; 
}

bool string (const std::string& s) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, lastKey, s, level);
return true; 
}

bool binary (const json::binary_t& val) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, lastKey, "binary", level);
return true; 
}

bool key (const std::string& s) { 
lastKey = s;
return true;
}

bool start_array (size_t n) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, lastKey, "array", level);
level++;
return true; 
}

bool end_array () { 
level--;
return true; 
}

bool start_object (size_t n) { 
int start = lastPos;
int end = regLastPos();
addMarker(start, end, lastKey, "object", level);
level++;
return true; 
}

bool end_object () { 
level--;
return true; 
}

bool parse_error (size_t pos, const std::string& token, const std::exception& ex) { 
return false; 
}

};//JsonSax




bool JsonTextMarkerFinder::Reset (const wxString& text) {
std::string utfText = U(text);
std::istringstream in(utfText);
JsonSax sax(markers, in, utfText);
markers.clear();
json::sax_parse(in, &sax);
return true;
}

void registerJsonTextMarkerFinder () {
TextMarkerFinder::Register("json", [](auto&p){ return new JsonTextMarkerFinder(); });
}
