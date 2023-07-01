#include "ReadOnlyTextDialog.hpp"
#include <wx/utils.h>
#include <wx/tglbtn.h>
using namespace std;

ReadOnlyTextDialog::ReadOnlyTextDialog (wxWindow* parent, const wxString& hint, const wxString& title, const wxString& message, int buttons):
wxDialog(parent, wxID_ANY, 
title,
wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE)
{
auto msgLabel = new wxStaticText(this, wxID_ANY, hint);
msgText = new wxTextCtrl(this, wxID_ANY, message, wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2 | wxTE_AUTO_URL | wxTE_PROCESS_ENTER | wxHSCROLL | wxTE_DONTWRAP);

auto dlgSizer = new wxBoxSizer(wxVERTICAL);
auto upperSizer = new wxBoxSizer(wxHORIZONTAL);
auto btnSizer = new wxStdDialogButtonSizer();

wxButton *okBtn=nullptr, *cancelBtn=nullptr, *yesBtn=nullptr, *noBtn=nullptr;
if (buttons & wxOK)  btnSizer->AddButton(okBtn = new wxButton(this, wxID_OK));
if (buttons & wxYES)  btnSizer->AddButton(yesBtn = new wxButton(this, wxID_YES));
if (buttons & wxNO)  btnSizer->AddButton(noBtn = new wxButton(this, wxID_NO));
if (buttons & wxCANCEL)  btnSizer->AddButton(cancelBtn = new wxButton(this, wxID_CANCEL));
detailBtn = new wxToggleButton(this, 1000, "Details");

detailBtn->SetValue(buttons&RTD_DETAILS_OPEN);

upperSizer->Add(msgLabel, 1, wxEXPAND);
upperSizer->Add(detailBtn, 0);
dlgSizer->Add(upperSizer, 0);
dlgSizer->Add(msgText, 1, wxEXPAND);
dlgSizer->Add(btnSizer, 0);

SetEscapeId(cancelBtn? wxID_CANCEL : wxID_OK);
msgText->Show(detailBtn->GetValue());
if (okBtn||yesBtn||cancelBtn) SetDefaultItem(okBtn? okBtn : (yesBtn? yesBtn : cancelBtn));
if (okBtn||yesBtn) btnSizer->SetAffirmativeButton(okBtn? okBtn : yesBtn);
if (cancelBtn) btnSizer->SetCancelButton(cancelBtn);
if (noBtn) btnSizer->SetNegativeButton(noBtn);
if (detailBtn->GetValue()) msgText->SetFocus();
else if (noBtn && (buttons&wxNO_DEFAULT)) noBtn->SetFocus();
else if (cancelBtn && (buttons&wxCANCEL_DEFAULT)) cancelBtn->SetFocus();
else if (okBtn) okBtn->SetFocus();
else if (yesBtn) yesBtn->SetFocus();

Bind(wxEVT_BUTTON, &ReadOnlyTextDialog::OnButtonClick, this);
detailBtn->Bind(wxEVT_TOGGLEBUTTON, [this](auto&e)mutable{
msgText->Show(detailBtn->GetValue());
Fit();
Layout();
});
msgText->Bind(wxEVT_TEXT_URL, &ReadOnlyTextDialog::URLClicked, this);
btnSizer->Realize();
SetSizerAndFit(dlgSizer);
}

void ReadOnlyTextDialog::URLClicked (wxTextUrlEvent& ue) {
auto& e = ue.GetMouseEvent();
auto uStart = ue.GetURLStart(), uEnd = ue.GetURLEnd();
if (e.LeftDown()) {
auto uurl = msgText->GetRange(uStart, uEnd);
wxLaunchDefaultBrowser(uurl, wxBROWSER_NEW_WINDOW);
}}

void ReadOnlyTextDialog::OnButtonClick (wxCommandEvent& e) {
EndModal(e.GetId());
}



