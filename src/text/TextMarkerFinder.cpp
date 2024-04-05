#include "TextMarkerFinder.hpp"
#include "../common/Properties.hpp"
#include <boost/algorithm/find_backward.hpp>
#include<unordered_map>

TextMarker* TextMarkerFinder::FindMarker (size_t initialPos, const wxString& pattern) {
auto marker = FindNextMarker(initialPos, pattern);
if (marker) return marker;
return FindPreviousMarker(initialPos, pattern);
}

TextMarker* TextMarkerFinder::FindNextMarker (size_t pos, const wxString& pattern) {
auto it = std::find_if(markers.begin(), markers.end(), [&](auto&m){ return m.start > pos && NameMatches(m.name, pattern); });
return it!=markers.end()? &*it : nullptr;
}

TextMarker* TextMarkerFinder::FindPreviousMarker (size_t pos, const wxString& pattern) {
auto it = boost::algorithm::find_if_backward(markers.begin(), markers.end(), [&](auto&m){ return m.start < pos && NameMatches(m.name, pattern); });
return it!=markers.end()? &*it : nullptr;
}

bool TextMarkerFinder::NameMatches (const wxString& name, const wxString& pattern) {
return pattern.empty() || name==pattern || ends_with(name, pattern) || wxMatchWild(pattern, name, false);
}

class RegexTextMarkerFinder: public TextMarkerFinder {
private:
wxRegEx *splitReg, *branchReg, *leafReg, *excludeReg, *openReg, *closeReg;
int branchDisplayNameIndex, branchNameIndex, leafDisplayNameIndex, leafNameIndex;

public:

RegexTextMarkerFinder (Properties& props):
splitReg(nullptr), branchReg(nullptr), leafReg(nullptr), excludeReg(nullptr), openReg(nullptr), closeReg(nullptr),
branchDisplayNameIndex(1), branchNameIndex(2), leafDisplayNameIndex(1), leafNameIndex(2)
 {
std::string openRegS, closeRegS;
std::string blockType = props.get("block_type", "");
if (blockType=="brace") {
openRegS = "\\{\\s*$";
closeRegS = "\\}\\s*$";
}

auto splitRegS = props.get("marker_split_regex", "");
auto branchRegS = props.get("marker_branch_regex", "");
auto leafRegS = props.get("marker_leaf_regex", "");
auto excludeRegS = props.get("marker_exclude_regex", "");
openRegS = props.get("marker_open_regex", openRegS);
closeRegS = props.get("marker_close_regex", closeRegS);
branchDisplayNameIndex = props.get("marker_branch_display_name_index", branchDisplayNameIndex);
leafDisplayNameIndex = props.get("marker_leaf_display_name_index", leafDisplayNameIndex);
branchNameIndex = props.get("marker_branch_name_index", branchNameIndex);
leafNameIndex = props.get("marker_leaf_name_index", leafNameIndex);

#define C(N) if (!N##S.empty()) N = new wxRegEx(U(N##S), wxRE_NEWLINE);
C(splitReg)
C(branchReg)
C(leafReg)
C(excludeReg)
C(openReg)
C(closeReg)
#undef C
}

~RegexTextMarkerFinder () {
delete splitReg;
delete branchReg;
delete leafReg;
delete excludeReg;
delete closeReg;
delete openReg;
}

size_t getLevelFromIndentation (const wxString& text, std::vector<size_t>& levels) {
size_t l=0, n = text.find_first_not_of(" \t");
if (n==std::string::npos) n=0;
while (l<levels.size() && n>levels[l]) l++;
if (l>=levels.size()) levels.push_back(n);
return l;
}

bool Reset (const wxString& text) override {
size_t pos = 0, start=0, end=0, length=0, level=0, subStart=0, subLength=0;
std::vector<size_t> levels = { 0 };

markers.clear();
while (splitReg && splitReg->Matches(text.data() +pos, 0, text.size() -pos)) {
splitReg->GetMatch(&start, &length, 0);
end = pos + start + length;
start = pos;
length = end-start;
pos = end;

if (!openReg && !closeReg) level = getLevelFromIndentation(text.substr(start, length), levels);
if (branchReg && branchReg->Matches(text.data() + start, 0, length)) {
branchReg->GetMatch(&subStart, &subLength, branchNameIndex);
wxString name = text.substr(start + subStart, subLength);
branchReg->GetMatch(&subStart, &subLength, branchDisplayNameIndex);
wxString displayName = text.substr(start + subStart, subLength);
if (!excludeReg || !excludeReg->Matches(text.data()+start, 0, length))
markers.emplace_back(start+subStart, start+subStart+subLength, 0, level, name, displayName);
}
if (leafReg && leafReg->Matches(text.data()+start, 0, length)) {
leafReg->GetMatch(&subStart, &subLength, leafNameIndex);
wxString name = text.substr(start + subStart, subLength);
leafReg->GetMatch(&subStart, &subLength, leafDisplayNameIndex);
wxString displayName = text.substr(start + subStart, subLength);
if (!excludeReg || !excludeReg->Matches(text.data()+start, 0, length))
markers.emplace_back(start+subStart, start+subStart+subLength, 0, level, name, displayName);
}
if (openReg && openReg->Matches(text.data() + start, 0, length)) level++;
if (closeReg && closeReg->Matches(text.data() + start, 0, length)) level--;
}
return true;
}

}; // class RegexTextMarkerFinder

class MarkdownTextMarkerFinder: public TextMarkerFinder {
public:
bool Reset (const wxString& text) {
wxRegEx reg("^\\s*(#+|title:)\\s*(.*?)\\s*#*\\s*$", wxRE_NEWLINE | wxRE_ICASE);
size_t pos=0, start=0, end=0, length=0, subStart=0, subLength=0, level=0;

markers.clear();
while (reg.Matches(text.data() +pos, 0, text.size() -pos)) {
reg.GetMatch(&start, &length, 0);

reg.GetMatch(&subStart, &subLength, 2);
wxString heading = text.substr(pos+subStart, subLength);
reg.GetMatch(&subStart, &subLength, 1);
wxString decl = text.substr(pos+subStart, subLength);
level = decl[0]!='#'? 0 : subLength -1;
markers.emplace_back(pos+subStart, pos+subStart+subLength, 0, level, heading, heading);

pos += start + length;
}
return true;
}

}; // MarkdownTextMarkerFinder

void TextMarkerFinder::Register (const std::string& name, const TextMarkerFinder::Factory& factory) {
factories[name] = factory;
}

void registerXMLTextMarkerFinder ();
void registerJsonTextMarkerFinder ();

static void initTextMarkerFinderFactories () {
TextMarkerFinder::Register("regex", [](auto&p){ return new RegexTextMarkerFinder(p); });
TextMarkerFinder::Register("markdown", [](auto&p){  return new MarkdownTextMarkerFinder(); });
registerXMLTextMarkerFinder();
registerJsonTextMarkerFinder();
}

TextMarkerFinder* TextMarkerFinder::Create (const std::string& name, Properties& props) {
if (factories.empty()) initTextMarkerFinderFactories();
auto it = factories.find(name);
if (it!=factories.end()) return (it->second)(props);
else return nullptr;
}




