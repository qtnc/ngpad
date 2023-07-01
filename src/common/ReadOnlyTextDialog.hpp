#ifndef _____READONLYTEXTDIALOG1___
#define _____READONLYTEXTDIALOG1___
#include "wxUtils.hpp"

#define RTD_DETAILS_OPEN 0x100000


struct ReadOnlyTextDialog: wxDialog {
struct wxTextCtrl* msgText;
struct wxToggleButton* detailBtn;

ReadOnlyTextDialog  (wxWindow* parent, const wxString& hint, const wxString& title, const wxString& message, int buttons);
void OnButtonClick (wxCommandEvent& e);
void URLClicked (wxTextUrlEvent& ue);
};

#endif
