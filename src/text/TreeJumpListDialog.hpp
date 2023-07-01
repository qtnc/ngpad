#ifndef _____TREE_JUMP_LIST_DIALOG_HPP_____
#define _____TREE_JUMP_LIST_DIALOG_HPP_____
#include "TreeJumpList.hpp"
#include<vector>
#include<memory>

class TreeJumpListDialog: public wxDialog {
private:
TreeJumpList* tree;

public:
TreeJumpListDialog (wxWindow* parent, const wxString& title, const wxString& message);
inline TreeJumpList* GetTreeJumpList () { return tree; }

protected:
//void OnItemActivate (wxTreeEvent& e);
//void OnItemCheck (wxTreeEvent& e);
};

void AddFileTree (TreeJumpList* tree, const wxString& sRootDir, const wxString& glob);

#endif