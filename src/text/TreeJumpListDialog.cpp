#include "TreeJumpListDialog.hpp"
#include "../common/println.hpp"
#include "../app/App.hpp"
#include "../common/stringUtils.hpp"
#include <wx/filename.h>
#include <wx/dir.h>

size_t FindAllFiles (const wxString& dirname, wxArrayString* files, const wxString& glob, int flags);
wxTreeItemId MakeFileTreeItem(wxTreeCtrl* tree, wxTreeItemId root, const wxArrayString& dirs, size_t idx, int itemState = -1);

TreeJumpListDialog::TreeJumpListDialog (wxWindow* parent, const wxString& title, const wxString& rootDir1):
wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxDIALOG_NO_PARENT),
tree(nullptr), rootDir(rootDir1)
{
timer = std::make_unique<wxTimer>(this, wxID_ANY);
auto lblFilter = new wxStaticText(this, wxID_ANY, MSG("TJLDFileFilter"), wxPoint(-1, -1), wxSize(1, 1));
filter = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHSCROLL | wxTE_PROCESS_ENTER);
auto lblTree = new wxStaticText(this, wxID_ANY, title, wxPoint(-1, -1), wxSize(1, 1));
tree = new TreeJumpList(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT);
auto okBtn = new wxButton(this, wxID_OK);
auto cancelBtn = new wxButton(this, wxID_CANCEL);

auto sizer = new wxBoxSizer(wxVERTICAL);
auto btnSizer = new wxStdDialogButtonSizer();
btnSizer->AddButton(okBtn);
btnSizer->AddButton(cancelBtn);
sizer->Add(filter, 0, wxEXPAND);
sizer->Add(tree, 1, wxEXPAND);
sizer->Add(btnSizer, 0, wxEXPAND);

Bind(wxEVT_TIMER, &TreeJumpListDialog::OnTimerNotify, this);
filter->Bind(wxEVT_TEXT, &TreeJumpListDialog::OnFilterTextInput, this);
filter->Bind(wxEVT_TEXT_ENTER, &TreeJumpListDialog::OnActivate, this);
Bind(wxEVT_BUTTON, &TreeJumpListDialog::OnCancel, this, wxID_CANCEL);
Bind(wxEVT_BUTTON, &TreeJumpListDialog::OnActivate, this, wxID_OK);

UpdateFileTree();
filter->SetFocus();
SetSizerAndFit(sizer);
}

void TreeJumpListDialog::OnFilterTextInput (wxCommandEvent& e) { 
if (timer) timer->StartOnce(1000); 
e.Skip();
}

void TreeJumpListDialog::SetRootDir (const wxString& rd) {
if (rootDir==rd) return;
rootDir = rd;
UpdateFileTree();
}

void TreeJumpListDialog::OnCancel () {
Hide();
}

void TreeJumpListDialog::OnActivate () {
tree->ActivateCurrentItem();
}

void TreeJumpListDialog::UpdateFileTree () {
std::string glob = U(filter->GetValue());
for (int i=glob.size() -1; i>0; i--) {
char c1 = glob[i];
if (c1>='A' && c1<='Z') glob.insert(glob.begin()+i, '*');
}
glob += '*';
AddFileTree(tree, rootDir, U(glob));
}

static bool IsFileExcluded (const wxString& file) {
static std::vector<std::string> excludes;
if (excludes.empty()) {
auto& config = wxGetApp().GetConfig();
excludes = split(config.get("workspace_file_list_exclude_exts", ""), ",; ", true);
for (auto& ex: excludes) ex.insert(ex.begin(), '.');
}
std::string s = U(file);
for (auto& ex: excludes) {
if (iends_with(s, ex)) return true;
}
return false;
}

void AddFileTree (TreeJumpList* tree, const wxString& sRootDir, const wxString& glob) {
wxArrayString files;
FindAllFiles(sRootDir, &files, glob, wxDIR_FILES | wxDIR_DIRS);
wxFileName rootDir = wxFileName::DirName(sRootDir);
tree->UnselectAll();
tree->Freeze();
auto root = tree->IsEmpty()? tree->AddRoot(wxEmptyString) : tree->GetRootItem();
tree->DeleteChildren(root);

wxString lastDir;
wxTreeItemId lastDirItem = root, firstFileItem = root;
bool firstFileItemSet=false, hideExcluded = wxGetApp().GetConfig() .get("workspace_file_list_hide_excluded", false);

for (auto& file: files) {
bool excluded = IsFileExcluded(file);
if (excluded && hideExcluded) continue;
wxFileName fn(file);
auto dir = fn.GetPath();
if (dir!=lastDir) {
lastDirItem = MakeFileTreeItem(tree, root, fn.GetDirs(), rootDir.GetDirCount());
lastDir = dir;
}
auto fileItem = tree->AppendItem(lastDirItem, fn.GetFullName(), -1, -1, new StringTreeItemData(file));
if (!firstFileItemSet && !excluded) { firstFileItem = fileItem; firstFileItemSet=true; }
}

tree->Thaw();
if (firstFileItemSet) tree->SelectItem(firstFileItem);
}
