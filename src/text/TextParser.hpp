#ifndef _____TEXT_PARSER_HPP
#define _____TEXT_PARSER_HPP
#include "../common/stringUtils.hpp"
#include "../common/wxUtils.hpp"
#include <wx/regex.h>

enum TokenType {
TT_NONE,
TT_NAME,
TT_NUMBER,
TT_STRING,
TT_OPERATOR,
TT_COMMENT,
TT_CONTROL_KW,
TT_MODIFIER_KW,
TT_TYPE_KW,
TT_CLASS_KW,
TT_OTHER_KW,
TT_LAST
};

class TextLexer {
private:
wxString text;
wxString value;
size_t start=0, pos = 0;
int tokenType = TT_NONE;

protected:
wxString multiOperatorChars = ".:+-*/%=<>!?|&^~#[]";
wxString singleOperatorChars = "(){}\"'`";
wxString ignoredChars = " ,;\r\n\t";
std::vector<wxString> lineCommentStrings = { "//" };
std::vector<std::pair<wxString,wxString>> commentStrings = {  { "/*", "*/" } };
std::vector<std::pair<wxString,wxString>> quoteStrings = { {"\"", "\""}, {"'", "'"}, {"`", "`"} };
std::vector<wxString> controlKeywords = { "return", "if", "else", "for", "while", "do", "switch", "case", "default", "try", "catch", "finally" };
std::vector<wxString> modifierKeywords = { "public", "private", "protected", "static", "abstract", "final", "const"  };
std::vector<wxString> typeKeywords = { "int", "long", "boolean", "double", "float", "char", "short", "byte" };
std::vector<wxString> classKeywords = { "class", "struct", "record", "extends", "implements", "permits", "sealed" };
std::vector<wxString> otherKeywords = { "null", "true", "false", "import", "package" };

public:
bool Reset (const wxString& text);
bool Next ();

inline int GetType () { return tokenType; }
inline size_t GetStart () { return start; }
inline size_t GetEnd () { return pos; }
inline size_t GetLength () { return pos-start; }
inline const wxString& GetValue () { return value; }
inline const wxString& GetText () { return text; }
};

#endif

