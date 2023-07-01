#include "TreeMarkerJumpList.hpp"
#include "../common/println.hpp"


TreeMarkerJumpList::TreeMarkerJumpList (wxWindow* parent, TextEditor* editor0):
TreeJumpList(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_SINGLE | wxTR_HAS_BUTTONS | wxTR_HIDE_ROOT),
editor(editor0), timer(nullptr), lastUpdate(0)
{
timer = std::make_unique<wxTimer>(this, wxID_ANY);
Bind(wxEVT_TIMER, &TreeMarkerJumpList::OnTimerNotify, this);
Bind(wxEVT_SET_FOCUS, &TreeMarkerJumpList::OnFocus, this);
timer->Start(1000);
Update(true);
}

void TreeMarkerJumpList::OnFocus (wxFocusEvent& e) {
Update(true);
e.Skip();
}

void TreeMarkerJumpList::OnTimerNotify (wxTimerEvent& e) {
Update(false);
}

void TreeMarkerJumpList::Update (bool force) {
auto lastKeyPress = editor->GetLastKeyPress();
auto now = mtime();
if (!force && (lastUpdate>=lastKeyPress || now-lastKeyPress<=1000)) return;
lastUpdate = now;
auto& df = editor->GetMarkerFinder();
if (!&df) return;
long start=0, end=0, line=0, lastMarkerStart=0, lastMarkerLine=0;
editor->GetSelection(&start, &end);
line = editor->GetLineOfPosition(start);

df.Reset(editor->GetValue());
wxTreeItemId lastItem, itemToSelect, root = IsEmpty()? AddRoot(wxEmptyString) : GetRootItem();

UnselectAll();
Freeze();
DeleteChildren(root);
std::vector<wxTreeItemId> items;
items.push_back(root);

for (auto& marker: df.GetMarkers()) {
if (marker.start>0 && end>=lastMarkerStart && static_cast<size_t>(end)<marker.start) itemToSelect = lastItem;
else if (marker.line>0 && line>=lastMarkerLine && static_cast<size_t>(line)<marker.line) itemToSelect = lastItem;
wxString cmd;
if (marker.line>0) cmd = format("{}", marker.line);
else cmd = U(format("^^{}", marker.start));
size_t index = std::min(marker.level, items.size() -1);
lastMarkerStart = marker.start;
lastMarkerLine = marker.line;
while(items.size()<=index+1) items.emplace_back();
items[index+1] = lastItem = AppendItem(items[index], marker.displayName, -1, -1, new StringTreeItemData(cmd));
}
if (lastMarkerStart>0 && end>=lastMarkerStart) itemToSelect = lastItem;
else if (lastMarkerLine>0 && line>=lastMarkerLine) itemToSelect = lastItem;
Thaw();
if (itemToSelect.IsOk()) SelectItem(itemToSelect);
}

void TreeMarkerJumpList::OnItemActivate (wxTreeEvent& e) {
editor->GetControl()->SetFocus();
TreeJumpList::OnItemActivate(e);
}
