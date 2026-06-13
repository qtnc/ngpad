#ifndef _____TREE_JUMP_LIST_DIALOG_HPP_____
#define _____TREE_JUMP_LIST_DIALOG_HPP_____
#include "TreeJumpList.hpp"
#include<vector>
#include<memory>

class TreeJumpListDialog: public wxDialog {
private:
TreeJumpList* tree;
wxTextCtrl* filter;
wxString rootDir;
std::unique_ptr<wxTimer> timer;

public:
TreeJumpListDialog (wxWindow* parent, const wxString& title, const wxString& rootDir);
void SetRootDir (const wxString& rd);

protected:
void UpdateFileTree ();
void OnTimerNotify (wxTimerEvent& e) { UpdateFileTree();  }
void OnFilterTextInput (wxCommandEvent& e);
void OnActivate ();
void OnActivate (wxCommandEvent& e) { OnActivate(); }
void OnCancel ();
void OnCancel (wxCommandEvent& e) { OnCancel(); }
};

void AddFileTree (TreeJumpList* tree, const wxString& sRootDir, const wxString& glob);

#endif