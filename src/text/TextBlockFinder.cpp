#include "TextBlockFinder.hpp"
#include "../common/Properties.hpp"
#include<wx/regex.h>


class IndentTextBlockFinder: public TextBlockFinder {
public:
void Reset (int direction = 0) override {}
int GetBlockLevel (const wxString& text) override { return realBeginningOfLine(text); }
bool IsLikeBlankLine (const wxString& text) override { return isBlank(text); }
bool OnEnter (const wxString& previousLine, wxString& newLine, int& curIndent, int& newIndent) { return true; }
};

class RegexTextBlockFinder: public TextBlockFinder {
private:
int direction = 1;
int level = 0;
wxRegEx *regOpen, *regClose, *regBlank;
public:
RegexTextBlockFinder (const wxString& o, const wxString& c, const wxString& b);
~RegexTextBlockFinder ();
void Reset (int direction = 0) override;
int GetBlockLevel (const wxString& text) override;
bool IsLikeBlankLine (const wxString& text) override;
bool OnEnter (const wxString& previousLine, wxString& newLine, int& curIndent, int& newIndent) override;
int CountOpens (const wxString& line);
int CountCloses (const wxString& line);
};


static int CountMatches (const wxString& text, wxRegEx& reg) {
size_t pos = 0, count = 0, start=0, len=0;
while (reg.Matches(text.data() + pos, 0, text.size() -pos)) {
count++;
reg.GetMatch(&start, &len, 0);
pos += start+len;
}
return count;
}

RegexTextBlockFinder::RegexTextBlockFinder (const wxString& o, const wxString& c, const wxString& b):
regOpen(new wxRegEx(o)),
regClose(new wxRegEx(c)),
regBlank(new wxRegEx(b))
{}

void RegexTextBlockFinder::Reset (int d) {
direction = d;
level = 0;
}

int RegexTextBlockFinder::CountOpens (const wxString& line) {
return CountMatches(line, *regOpen);
}

int RegexTextBlockFinder::CountCloses (const wxString& line) {
return CountMatches(line, *regClose);
}

int RegexTextBlockFinder::GetBlockLevel (const wxString& line) {
int opens = CountOpens(line), closes = CountCloses(line), l = level;
level += direction * (opens-closes);
return std::min(l, level);
}

bool RegexTextBlockFinder::IsLikeBlankLine (const wxString& text) {
return regBlank->Matches(text);
}

bool RegexTextBlockFinder::OnEnter (const wxString& line, wxString& newLine, int& curIndent, int& newIndent) {
int opens = CountOpens(line), closes = CountCloses(line);
newIndent = opens - closes;
curIndent = -closes;
return true;
}

RegexTextBlockFinder::~RegexTextBlockFinder () {
delete regOpen;
delete regClose;
delete regBlank;
}

void TextBlockFinder::Register (const std::string& name, const TextBlockFinder::Factory& factory) {
factories[name] = factory;
}

static void initTextBlockFinderFactories () {
TextBlockFinder::Register("indentation", [](auto&p){ return new IndentTextBlockFinder(); }  );
TextBlockFinder::Register("brace", [](auto&p){ return new RegexTextBlockFinder( "\\{", "\\}", "^[\\s{};]*$"); } );
TextBlockFinder::Register("xml", [](auto&p){ return new RegexTextBlockFinder("<(?!/).*?(?<!/)>",  "</.*?>", "^(?:\\s*</[-a-zA-Z_0-9:]+>)*\\s*$"); });
TextBlockFinder::Register("regex", [](auto&p){ 
return new RegexTextBlockFinder(
p.get("block_open_regex", ""),
p.get("block_close_regex", ""),
p.get("block_blank_regex", "")); 
} );
}

TextBlockFinder* TextBlockFinder::Create (const std::string& name, Properties& props) {
if (factories.empty()) initTextBlockFinderFactories();
auto it = factories.find(name);
if (it!=factories.end()) return (it->second)(props);
else return nullptr;
}

