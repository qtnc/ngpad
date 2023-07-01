#include "Properties.hpp"
#include "stringUtils.hpp"

void Properties::load (std::istream& in) {
std::string line;
while(std::getline(in, line)) {
trim(line);
if (line.empty() || line[0]=='#' || line[0]==';') continue;
auto i = line.find('=');
if (i==std::string::npos) continue;
std::string key = line.substr(0, i), value = line.substr(i+1);
while (ends_with(value, "\\") && std::getline(in, line)) {
trim(line);
value.pop_back();
value += '\n';
value += line;
}
unescape(value);
trim(key); trim(value);
put(key, value);
}}

void Properties::save (std::ostream& out) {
for (auto& it: entries()) {
auto& key = it.first;
auto& value = it.second;
out << key << '=' << value << std::endl;
}}
