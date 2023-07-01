#include "TextParser.hpp"
#include "../common/println.hpp"

std::vector<std::string> ttt = {
"TT_NONE",
"TT_NAME",
"TT_NUMBER",
"TT_STRING",
"TT_OPERATOR",
"TT_COMMENT"
};

bool TextLexer::Reset (const wxString& s) {
text = s;
pos = 0;
return true;
}

bool TextLexer::Next () {
if (pos>=text.size()) {
tokenType = TT_NONE;
return false;
}
start = pos;
wxChar ch = text[pos++];
if (std::string::npos!=ignoredChars.find(ch)) {
pos = text.find_first_not_of(ignoredChars, pos);
return Next();
}
else if (std::string::npos!=multiOperatorChars.find(ch)) {
pos = text.find_first_not_of(multiOperatorChars, pos);
tokenType = TT_OPERATOR;
}
else if (std::string::npos!=singleOperatorChars.find(ch)) {
tokenType = TT_OPERATOR;
}
else if (ch>='0' && ch<='9') {
pos = text.find_first_not_of("0123456789.abcdefABCDEF_LlXxBbSsHhUuPpRr", pos);
tokenType = TT_NUMBER;
}
else {
tokenType = TT_NAME;
while(pos<text.size() && (ch=text[pos]) && ignoredChars.find(ch)==std::string::npos && singleOperatorChars.find(ch)==std::string::npos && multiOperatorChars.find(ch)==std::string::npos) pos++;
}

value = pos==std::string::npos? text.substr(start) : text.substr(start, pos-start);
if (tokenType==TT_OPERATOR && lineCommentStrings.end()!=std::find(lineCommentStrings.begin(), lineCommentStrings.end(), value)) {
pos = text.find('\n', pos);
value = pos==std::string::npos? text.substr(start) : text.substr(start, pos-start);
tokenType = TT_COMMENT;
}

if (tokenType==TT_OPERATOR) {
auto q = std::find_if(quoteStrings.begin(), quoteStrings.end(), [&](auto&p){ return p.first==value; });
if (q != quoteStrings.end()) {
pos = text.find(q->second, pos);
if (pos!=std::string::npos) pos+=q->second.size();
value = pos==std::string::npos? text.substr(start) : text.substr(start, pos-start);
tokenType = TT_STRING;
}}

if (tokenType==TT_OPERATOR) {
auto cmt  = std::find_if(commentStrings.begin(), commentStrings.end(), [&](auto&p){ return p.first==value; });
if (cmt!=commentStrings.end()) {
pos = text.find(cmt->second, pos);
if (pos!=std::string::npos) pos+=cmt->second.size();
value = pos==std::string::npos? text.substr(start) : text.substr(start, pos-start);
tokenType = TT_COMMENT;
}}

if (tokenType==TT_NAME) {
#define K(N,V) else if (std::find(V.begin(), V.end(), value)!=V.end()) tokenType = N;
if (false) {}
K(TT_CONTROL_KW, controlKeywords)
K(TT_MODIFIER_KW, modifierKeywords)
K(TT_TYPE_KW, typeKeywords)
K(TT_CLASS_KW, classKeywords)
K(TT_OTHER_KW, otherKeywords)
#undef K
}

println("\"{}\", {}, {}, {}, {}", U(value), ttt[tokenType], start, pos, pos-start);
return true;
}
