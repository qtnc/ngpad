#ifndef _____MESSAGE_BOX_EX_1
#define _____MESSAGE_BOX_EX_1
#include "wxUtils.hpp"


struct MessageBoxEx: wxDialog {

MessageBoxEx (wxWindow* parent, const wxString& title, const wxString& message, const std::vector<wxString>& buttons, int initialButton = 0, int cancelButton = -1);
void OnButtonClick (wxCommandEvent& e);

static int Open (wxWindow* parent, const wxString& title, const wxString& message, const std::vector<wxString>& buttons, int initialButton = 0, int cancelButton = -1);
};

#endif
