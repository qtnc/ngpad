#ifndef _____SINGLE_CHOICE_DIALOG_HPP
#define _____SINGLE_CHOICE_DIALOG_HPP
#include "wxUtils.hpp"
#include <wx/timer.h>
#include<memory>

class SingleChoiceDialog: public wxDialog {
private:
wxListBox* list;
wxTextCtrl* tfFilter;
std::unique_ptr<wxTimer> timer;
const wxArrayString& options;

public:
SingleChoiceDialog (wxWindow* parent, const wxString& title, const wxString& message, const wxArrayString& options, int selection = 0, bool sorted = false);
bool SetSelection (int i);
int GetSelection ();
wxString GetStringSelection () { return list->GetStringSelection(); }

private:
void FillList ();
};

#endif
