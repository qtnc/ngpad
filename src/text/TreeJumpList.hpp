#ifndef ____TREE_JUMP_LIST_HPP____
#define ____TREE_JUMP_LIST_HPP____
#include <wx/treectrl.h>
#include "../common/ObjectClientData.hpp"

class TreeJumpList: public wxTreeCtrl {
public:
TreeJumpList (wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTR_HIDE_ROOT | wxTR_HAS_BUTTONS | wxTR_SINGLE);

protected:
virtual void OnItemActivate (wxTreeEvent& e);
};

#endif
