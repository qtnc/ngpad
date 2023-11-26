#include "FindReplaceDialog.hpp"
#include "TextEditor.hpp"
#include "../common/stringUtils.hpp"
#include "TextView.hpp"
#include "../app/App.hpp"
#include "../common/ObjectClientData.hpp"
#include <wx/regex.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/dirdlg.h>
#include <wx/treectrl.h>
#include <wx/stream.h>
#include <wx/stdstream.h>
#include <wx/wfstream.h>
#include "../common/LiveRegion.hpp"
#include "../common/pcre2cpp.hpp"
#include "../common/println.hpp"


void LoadEditorConfig (Properties& properties, const wxString& filename);
bool LoadFile (const wxString& filename, wxString& text, Properties& properties);
bool SaveFile (const wxString& filename, wxString& text, const Properties& properties);
wxRegEx GlobToRegEx (const wxString& glob);

struct FindResultInfoTreeItemData: wxTreeItemData {
FindResultInfo& info;
FindResultInfoTreeItemData (FindResultInfo& info0): info(info0) {}
};

class GlobTraverser: public wxDirTraverser {
private:
wxArrayString& files;
wxRegEx reg;
public:
GlobTraverser (wxArrayString& files0, const wxString& glob): files(files0), reg(GlobToRegEx(glob))  {}
wxDirTraverseResult OnOpenError (const wxString& dir) override { return wxDIR_IGNORE; }
wxDirTraverseResult OnDir (const wxString& dir) final override { return wxDIR_CONTINUE; }
wxDirTraverseResult OnFile (const wxString& file) final override { 
if (reg.Matches(file)) files.push_back(file);
return wxDIR_CONTINUE; 
}
};

size_t FindAllFiles (const wxString& dirname, wxArrayString* files, const wxString& glob, int flags) {
wxDir dir(dirname);
GlobTraverser traverser(*files, glob);
return dir.Traverse(traverser, wxEmptyString, flags);
}


static inline wxString GetDialogTitle (const FindReplaceInfo& info) {
if ((info.flags&FRI_REPLACE) && (info.flags&FRI_MULTIPLE)) return MSG("MultireplaceDlg");
else if ((info.flags&FRI_MULTIPLE) && !(info.flags&FRI_REPLACE)) return MSG("MultifindDlg");
else if ((info.flags&FRI_REPLACE) && !(info.flags&FRI_MULTIPLE)) return MSG("ReplaceDlg");
else return MSG("FindDlg");
}

FindReplaceDialog::FindReplaceDialog (wxWindow* parent, const FindReplaceInfo& info0, TextView* view0):
wxDialog(parent, wxID_ANY, GetDialogTitle(info0), wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxDIALOG_NO_PARENT),
info(info0), view(view0), timer(nullptr),
find(nullptr), replace(nullptr), glob(nullptr), rootDir(nullptr), rootDirBrowse(nullptr), foundTree(nullptr), icase(nullptr), wholeWord(nullptr), regex(nullptr), regDotAll(nullptr), regUnicode(nullptr), direction(nullptr)
{
timer = std::make_unique<wxTimer>(this, wxID_ANY);
auto topSizer = new wxBoxSizer(wxVERTICAL);

LoadHistory();
std::vector<wxString> finds, replaces;
for (int i=history.size() -1; i>=0; i--) {
auto& h = history[i];
finds.push_back(h.find);
if (!h.replace.empty()) replaces.push_back(h.replace);
}

auto grid = new wxFlexGridSizer(2);
grid->Add(new wxStaticText(this, wxID_ANY, MSG("Find") + ":"));
find = new wxComboBox(this, wxID_ANY, info.find, wxDefaultPosition, wxDefaultSize, finds.size(), &finds[0], 0);
grid->Add(find, 1, wxEXPAND);
if (info.flags&FRI_REPLACE) {
grid->Add(new wxStaticText(this, wxID_ANY, MSG("ReplaceWith") + ":"));
replace = new wxComboBox(this, wxID_ANY, info.replace, wxDefaultPosition, wxDefaultSize, replaces.size(), &replaces[0], 0);
grid->Add(replace, 1, wxEXPAND);
}
if (info.flags&FRI_MULTIPLE) {
grid->Add(new wxStaticText(this, wxID_ANY, MSG("SearchIn") +":"));
rootDir = new wxTextCtrl(this, wxID_ANY, info.rootDir, wxDefaultPosition, wxDefaultSize, wxHSCROLL);
rootDirBrowse = new wxButton(this, wxID_ANY, MSG("Browse") +"...");
auto sizer = new wxBoxSizer(wxHORIZONTAL);
sizer->Add(rootDir, 1, wxEXPAND);
sizer->Add(rootDirBrowse);
grid->Add(sizer, 1, wxEXPAND);
grid->Add(new wxStaticText(this, wxID_ANY, MSG("GlobPattern") +":"));
glob = new wxTextCtrl(this, wxID_ANY, info.glob, wxDefaultPosition, wxDefaultSize, wxHSCROLL);
grid->Add(glob, 1, wxEXPAND);
rootDirBrowse->Bind(wxEVT_BUTTON, &FindReplaceDialog::OnBrowse, this);
}
icase = new wxCheckBox(this, wxID_ANY, MSG("IgnoreCase"));
wholeWord = new wxCheckBox(this, wxID_ANY, MSG("WholeWord"));
regex = new wxCheckBox(this, wxID_ANY, MSG("RegularExpression"));
regDotAll = new wxCheckBox(this, wxID_ANY, MSG("RegexDotAll"));
regUnicode = new wxCheckBox(this, wxID_ANY, MSG("RegexUCP"));
icase->SetValue(info.flags&FRI_ICASE);
wholeWord->SetValue(info.flags&FRI_WHOLE_WORD);
regex->SetValue(info.flags&FRI_REGEX);
regDotAll->SetValue(info.flags&FRI_RE_DOTALL);
regUnicode->SetValue(info.flags&FRI_RE_UNICODE);
grid->Add(icase);
grid->Add(wholeWord);
grid->Add(regex);
grid->Add(new wxStaticText(this, wxID_ANY, wxEmptyString));
grid->Add(regDotAll);
grid->Add(regUnicode);
topSizer->Add(grid, 1, wxEXPAND);

if ((info.flags & (FRI_MULTIPLE | FRI_REPLACE))==0) {
wxString labels[] = { MSG("DirectionUpward"), MSG("DirectionDownward") };
direction = new wxRadioBox(this, wxID_ANY, MSG("Direction"), wxDefaultPosition, wxDefaultSize, 2, labels, 1, wxRA_SPECIFY_ROWS);
direction->SetSelection( (info.flags&FRI_UPWARD)? 0 : 1);
topSizer->Add(direction, 0, wxEXPAND);
}

resultLbl = new wxStaticText(this, wxID_ANY, wxEmptyString);
foundTree = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_SINGLE | wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT);
if (info.flags&FRI_REPLACE) MakeCheckTreeCtrl(foundTree, [this](auto&i,int s){ this->OnFoundItemCheck(i,s); });
foundTree->Bind(wxEVT_TREE_ITEM_ACTIVATED, &FindReplaceDialog::OnFoundItemActivate, this);

topSizer->Add(resultLbl, 0, wxEXPAND);
topSizer->Add(foundTree, 1, wxEXPAND);

auto bupane = CreateButtonSizer(wxOK | wxCANCEL);
topSizer->Add(bupane, 0, wxEXPAND);

Bind(wxEVT_TIMER, &FindReplaceDialog::OnTimerNotify, this);
Bind(wxEVT_BUTTON, &FindReplaceDialog::OnCancel, this, wxID_CANCEL);
Bind(wxEVT_BUTTON, &FindReplaceDialog::OnOK, this, wxID_OK);
regex->Bind(wxEVT_CHECKBOX, &FindReplaceDialog::UpdateEnabledCheckboxesAndStartTimer, this);
icase->Bind(wxEVT_CHECKBOX, &FindReplaceDialog::UpdateEnabledCheckboxesAndStartTimer, this);
wholeWord->Bind(wxEVT_CHECKBOX, &FindReplaceDialog::UpdateEnabledCheckboxesAndStartTimer, this);
regDotAll->Bind(wxEVT_CHECKBOX, &FindReplaceDialog::UpdateEnabledCheckboxesAndStartTimer, this);
regUnicode->Bind(wxEVT_CHECKBOX, &FindReplaceDialog::UpdateEnabledCheckboxesAndStartTimer, this);
if (rootDir) rootDir->Bind(wxEVT_TEXT, &FindReplaceDialog::StartUpdateTimer, this);
if (glob) glob->Bind(wxEVT_TEXT, &FindReplaceDialog::StartUpdateTimer, this);
find->Bind(wxEVT_TEXT, &FindReplaceDialog::StartUpdateTimer, this);
find->Bind(wxEVT_COMBOBOX, &FindReplaceDialog::OnFindSelChange, this);
find->SetFocus();
UpdateEnabledCheckboxes();
SetLiveRegion(resultLbl);
SetSizerAndFit(topSizer);
}

void FindReplaceDialog::UpdateEnabledCheckboxes () {
wholeWord->Enable(!regex->IsChecked());
regDotAll->Enable(regex->IsChecked());
regUnicode->Enable(regex->IsChecked());
}

void FindReplaceDialog::StartUpdateTimer () {
if (timer) timer->StartOnce(1000);
}

void FindReplaceDialog::OnCancel (wxCommandEvent& e) {
Destroy();
}

void FindReplaceDialog::OnOK (wxCommandEvent& e) {
FindReplaceInfo info = GetValue();
history.emplace_back(info);
if (history.size()>100) history.erase(history.begin());
SaveHistory();

if (view && !(info.flags&FRI_MULTIPLE)) {
view->Activate();
BellIfFalse(view->FindReplace(info, results));
}
else if (info.flags&(FRI_MULTIPLE|FRI_REPLACE)) {
info.ReplaceAllMultiple(results);
}

Destroy();
}

void FindReplaceDialog::OnBrowse (wxCommandEvent& e) {
wxDirDialog dd(this, wxDirSelectorPromptStr, rootDir->GetValue(), wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
if (dd.ShowModal()==wxID_OK) rootDir->SetValue(dd.GetPath());
}

void FindReplaceDialog::OnFindSelChange (wxCommandEvent& e) {
int i = find->GetSelection();
if (i<0 || i>=(int)history.size()) return;
auto& h = history[history.size() -i -1];
if (replace) replace->SetValue(h.replace);
regex->SetValue(h.flags&FRI_REGEX);
icase->SetValue(h.flags&FRI_ICASE);
wholeWord->SetValue(h.flags&FRI_WHOLE_WORD);
regDotAll->SetValue(h.flags&FRI_RE_DOTALL);
regUnicode->SetValue(h.flags&FRI_RE_UNICODE);
if (direction) direction->SetSelection( (h.flags&FRI_UPWARD)? 0 : 1);
UpdateEnabledCheckboxes();
StartUpdateTimer();
}

FindReplaceInfo FindReplaceDialog::GetValue () {
info.find = find->GetValue();
if (replace) info.replace = replace->GetValue();
if (glob) info.glob = glob->GetValue() ;
if (rootDir) info.rootDir = rootDir->GetValue() ;
info.flags =
(regex->IsChecked()? FRI_REGEX : 0)
| (icase->IsChecked()? FRI_ICASE : 0)
| (wholeWord->IsChecked()? FRI_WHOLE_WORD : 0)
| (regDotAll->IsChecked()? FRI_RE_DOTALL : 0)
| (regUnicode->IsChecked()? FRI_RE_UNICODE : 0)
| (direction && direction->GetSelection()==0? FRI_UPWARD : 0)
| (glob && rootDir? FRI_MULTIPLE : 0)
| (replace? FRI_REPLACE : 0);
return info;
}

void FindReplaceDialog::OnFoundItemActivate (wxTreeEvent& e) {
auto item = e.GetItem();
auto data = foundTree->GetItemData(item);
if (auto it = dynamic_cast<FindResultInfoTreeItemData*>(data)) {
auto& info = it->info;
wxGetApp().DoQuickJump( U(format("{}:{}:{}", U(info.filename), info.y+1, info.x+1)) );
}
else if (auto it = dynamic_cast<StringTreeItemData*>(data)) {
wxGetApp() .DoQuickJump(it->GetValue());
}
e.Skip();
}

void FindReplaceDialog::OnFoundItemCheck (const wxTreeItemId& item, bool checked) {
auto data = foundTree->GetItemData(item);
if (auto it = dynamic_cast<FindResultInfoTreeItemData*>(data)) {
it->info.enabled = checked;
}
}

wxTreeItemId MakeFileTreeItem(wxTreeCtrl* tree, wxTreeItemId root, const wxArrayString& dirs, size_t idx, int itemState) {
if (idx>=dirs.size()) return root;
const wxString& dir = dirs[idx];
wxTreeItemIdValue cookie;
wxTreeItemId item = tree->GetFirstChild(root, cookie);
while (item.IsOk() ) {
if (tree->GetItemText(item) == dir) break;
item = tree->GetNextChild(root, cookie);
}
if (!item.IsOk()) {
item = tree->AppendItem(root, dir);
if (itemState>=0 && item.IsOk()) tree->SetItemState(item, itemState);
}
return MakeFileTreeItem(tree, item, dirs, idx+1, itemState);
}

void FindReplaceDialog::UpdateFindResults () {
int nResults=0, nFiles=0;
auto info = GetValue();

results.clear();
try {
info.FindAll(results, view, &nResults, &nFiles);
} catch (std::exception& e) {
resultLbl->SetLabel(U(e.what()));
LiveRegionUpdated(resultLbl);
wxBell();
}

wxFileName fnRootDir = wxFileName::DirName(info.rootDir);
wxTreeItemId root = foundTree->IsEmpty()? foundTree->AddRoot(wxEmptyString) : foundTree->GetRootItem();
foundTree->UnselectAll();
foundTree->Freeze();
foundTree->DeleteChildren(root);
for (auto& result: results) {
wxTreeItemId fileItem = root;
if (!result.filename.empty()) {
wxFileName fn(result.filename);
auto dirs = fn.GetDirs();
dirs.push_back(fn.GetFullName());
fileItem = MakeFileTreeItem(foundTree, root, dirs, fnRootDir.GetDirCount(), info.flags&FRI_REPLACE? 1 : -1);
}
auto text = (info.flags & (FRI_REPLACE | FRI_REGEX))?
U(format("{}:{}: {} \xE2\x87\x92 {}. \t{}", result.y+1, result.x+1, U(result.foundText), U(result.replacementText), U(result.lineText))):
U(format("{}:{}: {}", result.y+1, result.x+1, U(result.lineText)));
auto entryItem = foundTree->AppendItem(fileItem, text, -1, -1, new FindResultInfoTreeItemData(result));
if (info.flags&FRI_REPLACE) foundTree->SetItemState(entryItem, 1);
}
foundTree->Thaw();

resultLbl->SetLabel(U(
format(GetTranslation(nFiles? "SRMultiResult" : "SRSimpleResult"), nResults, nFiles)
+ ":"));
LiveRegionUpdated(resultLbl);
}


WXPCRE FindReplaceInfo::CreateRegEx () const {
wxString find = this->find;
if (!(flags&FRI_REGEX)) find = "\\Q" + find + "\\E";
if (flags&FRI_WHOLE_WORD) find = "\\b" + find + "\\b";
return WXPCRE(
find, 
((flags&FRI_RE_DOTALL)? PCRE2_DOTALL : 0)
| ((flags&FRI_RE_UNICODE)? 0 : PCRE2_UCP) 
| ((flags&FRI_ICASE)? 0 : PCRE2_CASELESS) 
| PCRE2_UTF | PCRE2_MULTILINE | PCRE2_ALT_BSUX
);
}


bool FindReplaceInfo::FindNext (const wxString& text, size_t& start, size_t& end) const {
size_t initialStart = std::max(start,end);
auto reg = CreateRegEx();
if (reg.match(text, initialStart)) {
reg.position(&start, &end, 0);
return true;
}
return false;
 }

bool FindReplaceInfo::FindPrevious (const wxString& text, size_t& start, size_t& end) const {
size_t initialStart = std::min(start,end);
auto reg = CreateRegEx();

size_t pos = 0, i, j;
start = std::string::npos;
while (reg.match(text, pos)) {
reg.position(&i, &j, 0);
if (i<initialStart) { start=i; end=j; }
else break;
pos = j;
}
return start!=std::string::npos;
}

bool FindReplaceInfo::FindAll (std::vector<FindResultInfo>& results, TextView* view, int* nResults, int* nFiles) {
if (find.empty()) return false;
else if (flags&FRI_MULTIPLE) return FindAllMultiple(results, nResults, nFiles);
else if (view && (flags&FRI_REPLACE)) return FindAllInEditor(results, view->GetEditor(), wxEmptyString, true, nResults);
else if (view) return FindAllInEditor(results, view->GetEditor(), wxEmptyString, false, nResults);
else return false;
}

static TextEditor* FindEditor (const wxString& file) {
auto& app = wxGetApp();
auto doc = app.FindDocument(file);
auto wxview = doc? doc->GetFirstView() : nullptr;
auto view = wxview? dynamic_cast<TextView*>(wxview) : nullptr;
auto editor = view? view->GetEditor() : nullptr;
return editor;
}

bool FindReplaceInfo::FindAllMultiple (std::vector<FindResultInfo>& results, int* nResults, int* nFiles) {
wxArrayString files;
FindAllFiles(rootDir, &files, glob, wxDIR_FILES | wxDIR_DIRS);
for (auto& file: files) {
auto editor = FindEditor(file);
if (editor) FindAllInEditor(results, editor, file, flags&FRI_REPLACE);
else FindAllInFile(results, file);
}
if (nResults) *nResults = results.size();
if (nFiles) *nFiles = files.size();
return true;
}

int FindReplaceInfo::ReplaceAllMultiple (const std::vector<FindResultInfo>& results) {
std::unordered_map<wxString, std::vector<size_t>> infos;
int count, totalCount = 0;

for (auto& result: results) {
if (result.enabled) infos[result.filename].push_back(result.pos);
}
for (auto& info: infos) {
auto editor = FindEditor(info.first);
if (editor) count = ReplaceAllInEditor(editor, info.second);
else count = ReplaceAllInFile(info.first, info.second);
if (count>0) totalCount += count;
}
return totalCount;
}

bool FindReplaceInfo::FindAllInEditor (std::vector<FindResultInfo>& results, TextEditor* editor, const wxString& filename, bool onlySelection, int* nResults) {
if (!editor) return false;
long start=0, end=0, startX=0, startY=0, endX=0, endY=0;
if (onlySelection) {
editor->GetSelection(&start, &end);
if (start<end) std::swap(start, end);
editor->PositionToXY(start, &startX, &startY);
editor->PositionToXY(end, &endX, &endY);
}
if (start==end) { 
start = startX = startY = 0;
end = editor->GetLastPosition();
editor->PositionToXY(end, &endX, &endY);
}
wxString value = editor->GetValue();
start = xyToPosition(value, startX, startY);
end = xyToPosition(value, endX, endY);
return FindAllInText(results, value, filename, start, end, nResults);
}

bool FindReplaceInfo::FindAllInFile (std::vector<FindResultInfo>& results, const wxString& filename, int* nResults) {
wxString text;
Properties properties;
LoadEditorConfig(properties, filename);
LoadFile(filename, text, properties);
return FindAllInText(results, text, filename, 0, text.size(), nResults);
}

bool FindReplaceInfo::FindAllInText (std::vector<FindResultInfo>& results, const wxString& text, const wxString& filename, size_t start, size_t end, int* nResults) {
size_t pos = std::min(start,end), max = std::max(start, end);
auto reg = CreateRegEx();
while (reg.match(text, pos)) {
size_t i, j;
reg.position(&i, &j, 0);
if (i>=end) break;
size_t x=0, y=0;
positionToXY(text, i, x, y);
size_t endLine = text.find('\n', i), startLine = text.rfind('\n', i);
if (endLine==std::string::npos) endLine = text.size();
if (startLine==std::string::npos) startLine = 0;
wxString replacement = (flags&FRI_REPLACE)? reg.getReplacement(replace) : wxString(wxEmptyString);
results.emplace_back(x, y, i, j-i, filename, text.substr(startLine, endLine-startLine), reg.group(0), replacement);
pos = std::max(j, i+1);
}
if (nResults) *nResults = results.size();
return true;
}

int FindReplaceInfo::ReplaceAllInFile (const wxString& filename, const std::vector<size_t>& acceptedReplacements) {
wxString text;
Properties properties;
LoadEditorConfig(properties, filename);
LoadFile(filename, text, properties);
int count = ReplaceAllInText(text, 0, acceptedReplacements);
if (count>0) SaveFile(filename, text, properties);
return count;
}

int FindReplaceInfo::ReplaceAllInEditor (TextEditor* editor, const std::vector<size_t>& acceptedReplacements) {
if (!editor) return -1;

auto [start, end] = editor->GetSelection();
int count = -2;

if (start==end) {
wxString text = editor->GetValue();
count = ReplaceAllInText(text, 0, acceptedReplacements);
editor->ReplaceAndPushUndoState(0, editor->GetLastPosition(), text);
editor->SetSelection(start, end);
}
else {
long min = std::min(start, end), max = std::max(start, end);
wxString text = editor->GetRange(min, max);
count = ReplaceAllInText(text, min, acceptedReplacements);
editor->ReplaceAndPushUndoState(min, max, text);
if (end==max) end = start + text.size();
else start = end + text.size();
editor->SetSelection(start, end);
}
return count;
}

int FindReplaceInfo::ReplaceAllInText (wxString& text, size_t delta, const std::vector<size_t>& acceptedReplacements) {
auto reg = CreateRegEx();
int count = 0;
text = reg.replace(text, 0, [&](auto&r){
if (acceptedReplacements.empty() || acceptedReplacements.end()!=std::find(acceptedReplacements.begin(), acceptedReplacements.end(), delta+r.start())) {
count++;
return r.getReplacement(replace);
}
else {
return r.group(0);
}
});
return count;
}

bool FindReplaceDialog::LoadHistory () {
wxLogNull logNull;
wxString path = wxGetApp() .FindAppFile(FIND_REPLACE_HISTORY_FILENAME);
if (path.empty()) return false;
wxFileInputStream fIn(path);
wxBufferedInputStream bIn(fIn);
wxStdInputStream in(bIn);
Properties data;
data.load(in);
history.clear();
for (int i=1; data.contains("find" + std::to_string(i)); i++) {
std::string I = std::to_string(i);
history.emplace_back();
auto& info = history.back();
info.find = U(data.get("find" + I, ""));
info.replace = U(data.get("replace" +I, ""));
info.glob = U(data.get("glob" +I, ""));
info.rootDir = U(data.get("rootDir" +I, ""));
info.flags = data.get("flags" +I, 0);
}
return true;
}

bool FindReplaceDialog::SaveHistory () {
wxLogNull logNull;
Properties data;
int i = 0;
for (auto& info: history) {
std::string I = std::to_string(++i);
#define D(N) if (!info.N.empty()) data.put(#N +I, U(info.N));
D(find) D(replace) D(glob) D(rootDir)
#undef D
if (info.flags) data.put("flags" +I, info.flags);
}
wxString path = wxGetApp() .GetAppDir() + "/" FIND_REPLACE_HISTORY_FILENAME;
wxFileOutputStream fOut(path);
if (!fOut.IsOk()) return false;
wxBufferedOutputStream bOut(fOut);
wxStdOutputStream out(bOut);
data.save(out);
return true;
}
