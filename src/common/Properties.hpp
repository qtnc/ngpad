#ifndef _____PROPERTIES_HPP
#define _____PROPERTIES_HPP
#include<sstream>
#include<unordered_map>

class Properties {
private:
typedef std::unordered_map<std::string, std::string>  map_type;
map_type map;

public:
std::string& operator[] (const std::string& key) { return map[key]; }
map_type& entries () { return map; }
const map_type& entries () const { return map; }

void clear () {
map.clear();
}

std::string get (const std::string& key, const std::string& def) const { 
auto it = map.find(key);
return it==map.end()? def : it->second;
}

std::string get (const std::string& key, const char* def = nullptr) const { 
auto it = map.find(key);
return it==map.end()? (def? def : "") : it->second;
}

template <class T> get (const std::string& key, const T& def) const {
auto it = map.find(key);
if (it==map.end()) return def;
T value;
std::istringstream in(it->second);
in >> std::boolalpha >> value;
return value;
}

bool put (const std::string& key, const std::string& value, bool replace=true) {
if (!replace && map.find(key)!=map.end()) return false;
map[key] = value;
return true;
}

bool put (const std::string& key, const char* value, bool replace=true) {
if (!replace && map.find(key)!=map.end()) return false;
map[key] = value;
return true;
}

template <class T> bool put (const std::string& key, const T& value, bool replace=true) {
std::ostringstream out;
out << std::boolalpha << value;
return put(key, out.str(), replace);
}

bool contains (const std::string& key) {
return map.find(key)!=map.end();
}

bool remove (const std::string& key) {
auto it = map.find(key);
if (it==map.end()) return false;
map.erase(it);
return true;
}

void load (std::istream& in);
void save (std::ostream& out);
};


#endif
