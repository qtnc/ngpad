#include "SingleChoiceDialog.hpp"
#include "stringUtils.hpp"

SingleChoiceDialog::SingleChoiceDialog (wxWindow* parent, const wxString& title, const wxString& message, const wxArrayString& opts, int selection, bool sorted):
wxDialog(parent, wxID_ANY, title), 
list(nullptr), tfFilter(nullptr), options(opts), timer(nullptr)
{
auto sizer = new wxBoxSizer(wxVERTICAL);
new wxStaticText(this, wxID_ANY, MSG("QuickFilter"), wxPoint(0, 0), wxSize(1, 1));
tfFilter = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxHSCROLL);
sizer->Add(new wxStaticText(this, wxID_ANY, message), 0, wxEXPAND);
sizer->Add(tfFilter, 0, wxEXPAND);
list = new wxListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE | wxLB_HSCROLL | wxLB_NEEDED_SB | (sorted? wxLB_SORT : 0));
FillList();
SetSelection(selection);
sizer->Add(list, 1, wxEXPAND);
auto bupane = CreateButtonSizer(wxOK | wxCANCEL);
sizer->Add(bupane, 0, wxEXPAND);

timer = std::make_unique<wxTimer>(this, wxID_ANY);
Bind(wxEVT_TIMER, [&](auto&e){ FillList(); });
tfFilter->Bind(wxEVT_TEXT, [&](auto&e){ timer->StartOnce(500); });

MakeLetterNavigable(list);
list->SetFocus();

SetSizerAndFit(sizer);
}

void SingleChoiceDialog::FillList () {
int sel = GetSelection();
std::string filter = U(tfFilter->GetValue());
list->Freeze();
list->Clear();
for (size_t i=0, n=options.size(); i<n; i++) {
if (!filter.empty() && !icontains(U(options[i]), filter)) continue;
list->Append(options[i], reinterpret_cast<void*>(i));
}
list->Thaw();
SetSelection(sel);
}

bool SingleChoiceDialog::SetSelection (int sel) {
for (size_t i=0, n=list->GetCount(); i<n; i++) {
int x = reinterpret_cast<intptr_t>(list->GetClientData(i));
if (x==sel) {
list->SetSelection(i);
return true;
}}
return false;
}

int SingleChoiceDialog::GetSelection () {
int i = list->GetSelection();
if (i>=0) i = reinterpret_cast<intptr_t>( list->GetClientData(i) );
return i;
}



