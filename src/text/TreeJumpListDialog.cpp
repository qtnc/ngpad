#include "TreeJumpListDialog.hpp"
#include "../common/println.hpp"
#include "../app/App.hpp"
#include <wx/filename.h>
#include <wx/dir.h>

size_t FindAllFiles (const wxString& dirname, wxArrayString* files, const wxString& glob, int flags);
wxTreeItemId MakeFileTreeItem(wxTreeCtrl* tree, wxTreeItemId root, const wxArrayString& dirs, size_t idx, int itemState = -1);

TreeJumpListDialog::TreeJumpListDialog (wxWindow* parent, const wxString& title, const wxString& message):
wxDialog(parent, wxID_ANY, title, wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxDIALOG_NO_PARENT)
{
auto sizer = new wxBoxSizer(wxVERTICAL);

auto lbl = new wxStaticText(this, wxID_ANY, message);
tree = new TreeJumpList(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT);

sizer->Add(lbl, 0, wxEXPAND);
sizer->Add(tree, 1, wxEXPAND);
tree->SetFocus();

SetSizerAndFit(sizer);
}

void AddFileTree (TreeJumpList* tree, const wxString& sRootDir, const wxString& glob) {
wxArrayString files;
FindAllFiles(sRootDir, &files, glob, wxDIR_FILES | wxDIR_DIRS);
wxFileName rootDir = wxFileName::DirName(sRootDir);

tree->UnselectAll();
tree->Freeze();
auto root = tree->IsEmpty()? tree->AddRoot(wxEmptyString) : tree->GetRootItem();
tree->DeleteChildren(root);

for (auto& file: files) {
wxFileName fn(file);
auto dirs = fn.GetDirs();
auto fileItem = MakeFileTreeItem(tree, root, dirs, rootDir.GetDirCount());
tree->AppendItem(fileItem, fn.GetFullName(), -1, -1, new StringTreeItemData(file));
}

tree->Thaw();
}
