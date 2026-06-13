#include "TreeJumpList.hpp"
#include "../app/App.hpp"

TreeJumpList::TreeJumpList (wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style):
wxTreeCtrl(parent, id, pos, size, style)
{
Bind(wxEVT_TREE_ITEM_ACTIVATED, &TreeJumpList::OnItemActivate, this);
}

bool TreeJumpList::OnItemActivate (wxTreeItemId item) {
auto data = GetItemData(item);
if (!data) return false;
const wxString& cmd = static_cast<StringTreeItemData*>(data) ->GetValue();
wxGetApp() .DoQuickJump(cmd);
return true;
}
