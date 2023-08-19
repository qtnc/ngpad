#ifndef _____TEXT_VIEW_HPP
#define _____TEXT_VIEW_HPP
#include "../app/AbstractView.hpp"
#include "FindReplaceDialog.hpp"
#include <wx/splitter.h>
#include <wx/listctrl.h>
#include <wx/treectrl.h>
#include<memory>

class TextView: public AbstractView {
private:
struct TextEditor* editor = nullptr;
wxSplitterWindow* splitter = nullptr;
wxTreeCtrl* tree = nullptr;
wxListView* list = nullptr;
FindReplaceInfo findReplace;
unsigned char statusBarDisplayModes[4] = { 0, 0, 0, 0 };
FindReplaceDialog* findReplaceDialog = nullptr;

public:
TextEditor* GetEditor () { return editor; }
void UpdateStatus () override;
void UpdateMenus ();

bool FindReplaceDefined ();
bool FindNext (const FindReplaceInfo& info, bool backward, bool selectUntil);
bool FindNext ();
bool FindPrevious ();
bool FindReplace (const FindReplaceInfo& info, const std::vector<FindResultInfo>& results = {});
bool DoReplace (const std::vector<FindResultInfo>& results);
bool OpenFindReplaceDialog (int flags);

bool OnCreate (wxDocument* doc, long flags) override;
void OnUpdate (wxView* sender, wxObject* param) override;
void OnShowLateralPane ();
void OnSwitchPane ();
void OnChangeLineWrap (bool);
bool OnMenuAction (wxCommandEvent& e) override;
void OnStatusBarClick (wxMouseEvent& e, int field) override;

wxDECLARE_DYNAMIC_CLASS(TextView);
};

#endif
