#include "TreeJumpList.hpp"
#include "../app/App.hpp"

TreeJumpList::TreeJumpList (wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style):
wxTreeCtrl(parent, id, pos, size, style)
{
Bind(wxEVT_TREE_ITEM_ACTIVATED, &TreeJumpList::OnItemActivate, this);
}

void TreeJumpList::OnItemActivate (wxTreeEvent& e) {
auto item = e.GetItem();
auto data = GetItemData(item);
if (!data) return;
const wxString& cmd = static_cast<StringTreeItemData*>(data) ->GetValue();
wxGetApp() .DoQuickJump(cmd);
}
