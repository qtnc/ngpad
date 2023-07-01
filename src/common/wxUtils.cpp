#include "wxUtils.hpp"
#include <wx/clipbrd.h>
#include <wx/treectrl.h>
#include <wx/regex.h>
#include<cstdio>
using namespace std;

void SetClipboardText (const wxString& s) {
auto& cb = *wxClipboard::Get();
if (cb.Open()) {
finally __f([&](){ cb.Close(); });
cb.SetData(new wxTextDataObject(s));
cb.Flush();
}}

wxString GetClipboardText () {
auto& cb = *wxClipboard::Get();
if (cb.Open()) {
finally __f([&](){ cb.Close(); });
if (cb.IsSupported(wxDF_TEXT)) {
wxTextDataObject tdo;
cb.GetData(tdo);
return tdo.GetText();
}}
return "";
}


unsigned long long mtime () {
auto ll = wxGetLocalTimeMillis();
return *reinterpret_cast<unsigned long long*>(&ll);
}

struct LetterNavigable {
wxItemContainerImmutable* ctl;
wxString input;
unsigned long long time;

void OnChar (wxKeyEvent& e) {
auto ch = e.GetUnicodeKey();
if (ch==WXK_NONE || ch<=32) { e.Skip(); return; }
auto curtime = mtime();
if (curtime-time>400) input.clear();
input += ch;
time = curtime;
int count = ctl->GetCount(), pos = ctl->GetSelection(), start = pos, newPos = -1;
if (input.size()<=1) start++;
wxRegEx reg("\\b" + wxRegEx::QuoteMeta(input), wxRE_ICASE);
if (count) for (int i=start%count, j=0; j<count; j++, i=(i+1)%count) {
wxString text = ctl->GetString(i);
if (reg.Matches(text)) { newPos=i; break; }
}
if (newPos<0) wxBell();
else if (pos!=newPos) ctl->SetSelection(newPos);
}
LetterNavigable (wxItemContainerImmutable* ctl0): ctl(ctl0) {
auto ctlx = dynamic_cast<wxControl*>(ctl);
ctlx->Bind(wxEVT_CHAR, &LetterNavigable::OnChar, this);
ctlx->Bind(wxEVT_DESTROY, [&](auto&e)mutable{ delete this; });
}
};

void MakeLetterNavigable (wxItemContainerImmutable* ctl) {
new LetterNavigable(ctl);
}

void CheckTreeItemUpdate (wxTreeCtrl* tree, const wxTreeItemId& item) {
if (!item.IsOk()) return;
wxTreeItemIdValue cookie;
int state = 3;
for (wxTreeItemId child = tree->GetFirstChild(item, cookie); child.IsOk(); child = tree->GetNextChild(item, cookie)) {
if (state==2) break;
auto childState = tree->GetItemState(child);
if (state==3) state = childState;
else if (state!=childState) state = 2;
}
tree->SetItemState(item, state);
CheckTreeItemUpdate(tree, tree->GetItemParent(item));
}

void CheckTreeItem (wxTreeCtrl* tree, const wxTreeItemId& item, int state, const std::function<void(const wxTreeItemId&,int)>& onItemCheck) {
wxTreeItemIdValue cookie;
for (wxTreeItemId child = tree->GetFirstChild(item, cookie); child.IsOk(); child = tree->GetNextChild(item, cookie)) {
auto childState = tree->GetItemState(child);
if (childState!=state) CheckTreeItem(tree, child, state, onItemCheck);
}
tree->SetItemState(item, state);
CheckTreeItemUpdate(tree, tree->GetItemParent(item));
if (onItemCheck) onItemCheck(item, state);
}

void CheckTreeItemClicked (wxTreeCtrl* tree, const wxTreeItemId& item, const std::function<void(const wxTreeItemId&,int)>& onItemCheck) {
if (!item.IsOk()) return;
auto state = tree->GetItemState(item);
CheckTreeItem(tree, item, state==0? 1 : 0, onItemCheck);
}

void MakeCheckTreeCtrl (wxTreeCtrl* tree, const std::function<void(const wxTreeItemId&,int)>& onItemCheck) {
tree->Bind(wxEVT_TREE_STATE_IMAGE_CLICK, [=](auto&e){
CheckTreeItemClicked(tree, e.GetItem(), onItemCheck);
});
tree->Bind(wxEVT_CHAR_HOOK, [=](auto&e){
int k = e.GetKeyCode(), mods = e.GetModifiers();
if (k==WXK_SPACE && mods==0) {
wxTreeEvent e2(wxEVT_TREE_STATE_IMAGE_CLICK, tree, tree->GetSelection());
wxPostEvent(tree, e2);
}
else e.Skip();
});
}



