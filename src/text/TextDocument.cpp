#include "TextDocument.hpp"
#include "TextView.hpp"
#include "TextEditor.hpp"
#include "TextCommand.hpp"
#include "../app/App.hpp"
#include "../common/SingleChoiceDialog.hpp"
#include "../common/stringUtils.hpp"
#include "../common/ObjectClientData.hpp"
#include <wx/stream.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>
#include <wx/ffile.h>
#include <wx/regex.h>
#include <wx/artprov.h>
#include<sstream>
#include<memory>
#include "../common/println.hpp"

int GetEncodingFromName (const std::string& name);
std::string GetEncodingName (int encoding);
wxString GetEncodingDescription (int encoding);
int GetEncodingFromIndex (int index);
int GetEncodingIndex (int encoding);
int GetEncodingsCount (bool forMenu=false);
wxMBConv& GetEncodingCodec (int index);
int GetLineEndingsCount (bool forMenu = false);
int GetLineEndingFromName (const std::string& name);
std::string GetLineEndingName (int lineEnding);
int GetEncoding (const Properties& properties);
int GetLineEnding (const Properties& properties);
int GetIndentType (const Properties& properties);
void SetEncoding (Properties& properties, int encoding);
void SetLineEnding (Properties& properties, int lineEnding);
void SetIndentType (Properties& properties, int indentType);
bool LoadFile (const wxString& filename, wxString& text, Properties& properties);
bool SaveFile (const wxString& filename, wxString& text, const Properties& properties);

int LuaSimpleEval (const wxString& eval, wxString* result = nullptr);

wxIMPLEMENT_DYNAMIC_CLASS(TextDocument, wxDocument);

int TextDocument::GetEncoding () const {
return ::GetEncoding(properties);
}

void TextDocument::SetEncoding (int encoding) {
::SetEncoding(properties, encoding);
}

int TextDocument::GetLineEnding () const {
return ::GetLineEnding(properties);
}

void TextDocument::SetLineEnding (int lineEnding) {
::SetLineEnding(properties, lineEnding);
}

int TextDocument::GetIndentType () const {
return ::GetIndentType(properties);
}

void TextDocument::SetIndentType (int indentType) {
::SetIndentType(properties, indentType);
}

wxTextCtrlIface* TextDocument::GetEditor () const {
auto wxview = GetFirstView();
auto view = wxview? static_cast<TextView*>(wxview) :nullptr;
return view? view->GetEditor() :nullptr;
}

void TextDocument::Modify (bool modified) {
AbstractDocument::Modify(modified);
auto editor = GetEditor();
if (editor && editor->IsModified()!=modified) editor->SetModified(modified);
}

void TextDocument::PushUndoState (wxCommand* cmd, bool tryJoin) {
auto proc = GetCommandProcessor();
auto cur = dynamic_cast<TextCommand*>( proc->GetCurrentCommand() );
if (tryJoin && cur && cur->Join(cmd)) delete cmd;
else proc->Store(cmd);
}

bool TextDocument::DoOpenDocument (const wxString& filename) {
auto editor = GetEditor();
if (!editor) return false;
if (!wxFileExists(filename)) return true;
wxString text;
if (!LoadFile(filename, text, properties)) return false;
long start, end;
editor->GetSelection(&start, &end);
editor->SetValue(text);
editor->SetSelection(start, end);
Modify(false);
return true;
}

bool TextDocument::DoSaveDocument (const wxString& filename) {
auto editor = GetEditor();
if (!editor) return false;
wxString text = editor->GetValue();
if (!SaveFile(filename, text, properties)) return false;
Modify(false);
return true;
}

bool TextDocument::OnNewDocument () {
if (!AbstractDocument::OnNewDocument()) return false;
auto& app = wxGetApp();
Properties& config = app.GetConfig();
if (GetEncoding()==wxFONTENCODING_DEFAULT) SetEncoding(GetEncodingFromName(config.get("default_charset", "utf-8")));
if (GetLineEnding()<0) SetLineEnding(GetLineEndingFromName(config.get("default_line_ending", "crlf")));
if (GetIndentType()==0) {
bool tab = config.get("default_indent_style", "space")=="tab";
int size = (tab? -1 : 1) * config.get("default_indent_size", 4);
SetIndentType(size);
}
UpdateAllViews();
return true;
}

void TextDocument::ChooseEncodingDialog () {
int encoding = GetEncoding();
int selection = GetEncodingIndex(encoding);
wxArrayString options;
for (int i=0, n=GetEncodingsCount(); i<n; i++) {
int enc = GetEncodingFromIndex(i);
wxString desc = GetEncodingDescription(enc);
options.push_back(desc);
}
SingleChoiceDialog scd(GetFirstView()->GetFrame(), MSG("EncodingChoice"), MSG("ChooseEncoding")+":", options, selection);
if (scd.ShowModal()==wxID_OK) {
auto sel = scd.GetSelection();
auto enc = GetEncodingFromIndex(sel);
GetEncodingCodec(sel);
SetEncoding(enc);

auto menubar = static_cast<AbstractView*>(GetFirstView()) ->GetMenuBar();
if (!menubar->FindItem(IDM_ENCODING +sel)) {
auto encodingMenu = menubar->FindItem(IDM_ENCODING) ->GetMenu();
encodingMenu->InsertRadioItem(encodingMenu->GetMenuItemCount() -1, IDM_ENCODING +sel, GetEncodingDescription(enc));
}
menubar->Check(IDM_ENCODING +sel, true);
}}

void TextDocument::ChooseLineEndingDialog () {
int lineEnding = GetLineEnding();
wxArrayString options;
for (int i=0, n=GetLineEndingsCount(); i<n; i++) {
wxString desc = MSG("le-" + GetLineEndingName(i));
options.push_back(desc);
}
SingleChoiceDialog scd(GetFirstView()->GetFrame(), MSG("LineEndingChoice"), MSG("ChooseLineEnding")+":", options, lineEnding);
if (scd.ShowModal()==wxID_OK) {
auto sel = scd.GetSelection();
SetLineEnding(sel);

auto menubar = static_cast<AbstractView*>(GetFirstView()) ->GetMenuBar();
if (!menubar->FindItem(IDM_LINE_ENDING +sel)) {
auto leMenu = menubar->FindItem(IDM_LINE_ENDING) ->GetMenu();
leMenu->InsertRadioItem(leMenu->GetMenuItemCount() -1, IDM_LINE_ENDING  +sel, MSG("le-" + GetLineEndingName(sel)));
}
menubar->Check(IDM_LINE_ENDING  +sel, true);
}}

bool qjMoveTo (TextEditor* editor, const wxString& cmd) {
wxRegEx rMoveXY("^\\[?(\\d+)(?:[,:](\\d+))?\\]?$");
if (rMoveXY.Matches(cmd)) {
auto sLine = rMoveXY.GetMatch(cmd, 1);
auto sColumn = rMoveXY.GetMatch(cmd, 2);
int y = std::stoi(U(sLine)), x = sColumn.empty()? 1 : std::stoi(U(sColumn));
int pos = editor ->XYToPosition(x -1, y -1);
editor->SetInsertionPoint(pos);
return true;
}
return false;
}

bool qjSelect (TextEditor* editor, const wxString& cmd) {
wxRegEx rSelXY("^(\\d+)(?:[,:](\\d+))?-(\\d+)(?:[,:](\\d+))?$");
if (rSelXY.Matches(cmd)) {
auto sStartLine = rSelXY.GetMatch(cmd, 1);
auto sStartColumn = rSelXY.GetMatch(cmd, 2);
auto sEndLine = rSelXY.GetMatch(cmd, 3);
auto sEndColumn = rSelXY.GetMatch(cmd, 4);
int startY = std::stoi(U(sStartLine)), startX = sStartColumn.empty()? 1 : std::stoi(U(sStartColumn));
int endY = std::stoi(U(sEndLine)), endX = sEndColumn.empty()? -1 : std::stoi(U(sEndColumn));
if (endX<0) endX = editor->GetLineLength(endY -1);
int start = editor->XYToPosition(startX -1, startY -1);
int end = editor->XYToPosition(endX -1, endY -1);
editor->SetSelection(start, end);
return true;
}
return false;
}

bool qjRelativeMove (TextEditor* editor, const wxString& cmd) {
wxRegEx rRelMove("^[-+]\\d+$");
if (rRelMove.Matches(cmd)) {
int nLines = editor->GetNumberOfLines(), curLine = editor->GetLineOfPosition(editor->GetInsertionPoint());
auto sVal = rRelMove.GetMatch(cmd, 0);
int n = std::stoi(U(sVal));
int line = std::max(0, std::min(curLine +n, nLines));
int column = editor->GetLineLength(line);
int pos = editor->XYToPosition(column, line);
editor->SetInsertionPoint(pos);
return true;
}
return false;
}

bool qjAbsoluteMove (TextEditor* editor, const wxString& cmd) {
wxRegEx rAbsMove("^\\^\\^(\\d+)$");
if (rAbsMove.Matches(cmd)) {
auto sVal = rAbsMove.GetMatch(cmd, 1);
size_t x, y, pos = std::stoi(U(sVal));
positionToXY(editor->GetValue(), pos, x, y);
pos = editor->XYToPosition(x, y);
editor->SetInsertionPoint(pos);
return true;
}
return false;
}

bool qjFind (TextView* view, const wxString& cmd) {
if (cmd.empty() || cmd[0]!='/') return false;
auto p1 = cmd.find('/', 1);
if (p1==std::string::npos) return false;
wxString find = cmd.substr(1, p1 -1);
int flags = FRI_REGEX;
if (cmd.find('i', p1) != std::string::npos) flags |= FRI_ICASE;
if (cmd.find('s', p1) == std::string::npos) flags |= FRI_RE_DOTALL;
BellIfFalse(view->FindReplace(FindReplaceInfo(flags, find)));
return true;
}

bool qjMoveToMarker(TextView* view, const wxString& cmd) {
if (cmd.empty() || cmd[0]!='#') return false;
auto editor = view->GetEditor();
if (!editor) return false;
editor->MoveToMarker(cmd.substr(1));
return true;
}

bool qjEvalLua (TextEditor* editor, const wxString& cmd) {
if (cmd.empty() || cmd[0]!='=') return false;
wxString re;
LuaSimpleEval(cmd.substr(1), &re);
editor->WriteText(re);
return true;
}

bool qjExecuteCommand (TextDocument* doc, const wxString& cmd) {
if (cmd.empty() || cmd[0]!='$') return false;
bool inSameDoc = cmd.size()>=2 && cmd[1]=='>';
auto s = cmd.substr(inSameDoc? 2 : 1);
//if (!inSameDoc) doc = static_cast<TextDocument*>( wxGetApp() .CreateNewEmptyDocument(s) );
//doc->ExecuteCommand(s);
return true;
}

bool TextDocument::DoQuickJump (const wxString& cmd) {
auto view = static_cast<TextView*>(GetFirstView());
auto editor = view? view->GetEditor() : nullptr;
if (!editor) return false;
return 
qjMoveTo(editor, cmd) 
|| qjSelect(editor, cmd)
|| qjRelativeMove(editor, cmd)
|| qjFind(view, cmd)
|| qjMoveToMarker(view, cmd)
|| qjEvalLua(editor, cmd)
|| qjExecuteCommand(this, cmd)
|| qjAbsoluteMove(editor, cmd);
}

bool TextDocument::OnMenuAction (wxCommandEvent& e) {
if (AbstractDocument::OnMenuAction(e)) return true;
size_t id = e.GetId();
if (id>=IDM_ENCODING && id<IDM_ENCODING_OTHER) SetEncoding(GetEncodingFromIndex(id-IDM_ENCODING));
else if (id>=IDM_LINE_ENDING && id<IDM_LINE_ENDING_OTHER) SetLineEnding(id-IDM_LINE_ENDING);
else if (id>=IDM_INDENT && id<=IDM_INDENT+8) SetIndentType(id - IDM_INDENT);
else switch(id){
case wxID_REVERT_TO_SAVED: Revert(); break;
case IDM_ENCODING_OTHER: ChooseEncodingDialog(); break;
case IDM_LINE_ENDING_OTHER: ChooseLineEndingDialog(); break;
default: return false;
}
return true;
}

void TextDocument::AddSpecificTools (wxToolBar* toolbar) {
}

void TextDocument::AddSpecificMenus (wxMenuBar* menubar) {
wxMenu* format = new wxMenu();

wxMenu* encoding = new wxMenu();
for (int i=0, n=GetEncodingsCount(true); i<n; i++) encoding->AppendRadioItem(IDM_ENCODING +i, GetEncodingDescription(GetEncodingFromIndex(i)));
encoding->Append(IDM_ENCODING_OTHER, MSG("OtherEncoding") + "...");

wxMenu* lineEnding = new wxMenu();
for (int i=0, n=GetLineEndingsCount(true); i<n; i++)  lineEnding->AppendRadioItem(IDM_LINE_ENDING +i, MSG("le-" + GetLineEndingName(i)));
lineEnding->Append(IDM_LINE_ENDING_OTHER, MSG("OtherLineEnding") + "...");

wxMenu* indentation = new wxMenu();
indentation->AppendRadioItem(IDM_INDENT, MSG("IndentTabs"));
indentation->AppendRadioItem(IDM_INDENT +1, U(std::to_string(1)) + " " + MSG("IndentSpace"));
for (int i=2; i<=8; i++) indentation->AppendRadioItem(IDM_INDENT +i, U(std::to_string(i)) + " " + MSG("IndentSpaces"));

format->AppendSubMenu(encoding, MSG("Encoding"));
format->AppendSubMenu(lineEnding, MSG("LineEnding"));
format->AppendSubMenu(indentation, MSG("Indentation"));
format->AppendCheckItem(IDM_SHOW_PANE, MSG("LateralPane"));
format->AppendCheckItem(IDM_READONLY, MSG("ReadOnly") + "\tCtrl+Shift+Y");
format->AppendCheckItem(IDM_LINEWRAP, MSG("LineWrap"));
format->SetClientObject(new StringClientData("format"));

menubar->Insert(2, format, MSG("FormatMenu"));
}

