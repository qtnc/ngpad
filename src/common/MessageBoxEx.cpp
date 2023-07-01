#include "MessageBoxEx.hpp"
#include <wx/utils.h>
using namespace std;

int MessageBoxEx::Open (wxWindow* parent, const wxString& title, const wxString& message, const std::vector<wxString>& buttons, int initialButton, int cancelButton) {
MessageBoxEx mb(parent, title, message, buttons, initialButton, cancelButton);
int re = mb.ShowModal();
if (re>=1000) return re -1000;
else return -1;
}

MessageBoxEx::MessageBoxEx (wxWindow* parent, const wxString& title, const wxString& message, const std::vector<wxString>& buttonNames, int initialButton, int cancelButton):
wxDialog(parent, wxID_ANY, 
title,
wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
int btnId = 1000;
vector<wxButton*> buttons;
auto msgLabel = new wxStaticText(this, wxID_ANY, message);
auto dlgSizer = new wxBoxSizer(wxVERTICAL);
auto btnSizer = new wxBoxSizer(wxHORIZONTAL);
for (const wxString& s: buttonNames) {
auto btn = new wxButton(this, btnId++, s);
btnSizer->Add(btn, 0);
buttons.push_back(btn);
}

dlgSizer->Add(msgLabel, 1, wxEXPAND);
dlgSizer->Add(btnSizer, 0);
if (cancelButton>=0) SetEscapeId(1000 + cancelButton);
if (initialButton>=0) {
SetDefaultItem(buttons[initialButton]);
buttons[initialButton]->SetFocus();
}

Bind(wxEVT_BUTTON, &MessageBoxEx::OnButtonClick, this);
SetSizerAndFit(dlgSizer);
}


void MessageBoxEx::OnButtonClick (wxCommandEvent& e) {
EndModal(e.GetId());
}



