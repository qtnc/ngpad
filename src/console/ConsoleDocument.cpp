#include "ConsoleDocument.hpp"
#include "ConsoleView.hpp"
#include "ConsoleEditor.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"

wxIMPLEMENT_DYNAMIC_CLASS(ConsoleDocument, wxDocument);

wxTextCtrlIface* ConsoleDocument::GetEditor () const {
auto wxview = GetFirstView();
auto view = wxview? static_cast<ConsoleView*>(wxview) :nullptr;
return view? dynamic_cast<wxTextCtrlIface*>(view->GetEditor()) :nullptr;
}

void ConsoleDocument::Modify (bool modified) {
AbstractDocument::Modify(modified);
auto ta = dynamic_cast<wxTextAreaBase*>(GetEditor());
if (ta && ta->IsModified()!=modified) ta->SetModified(modified);
}

bool ConsoleDocument::DoOpenDocument (const wxString& filename) {
return true;
}

bool ConsoleDocument::DoSaveDocument (const wxString& filename) {
return true;
}

bool ConsoleDocument::OnNewDocument () {
if (!AbstractDocument::OnNewDocument()) return false;
UpdateAllViews();
return true;
}

void ConsoleDocument::ClearConsole () {
auto view = static_cast<ConsoleView*>(GetFirstView());
view->ClearConsole();
}

bool ConsoleDocument::ExecuteCommand (const wxString& cmd) {
auto view = static_cast<ConsoleView*>(GetFirstView());
return view->ExecuteCommand(cmd);
}

bool ConsoleDocument::DoQuickJump (const wxString& cmd) {
auto view = static_cast<ConsoleView*>(GetFirstView());
auto editor = view? view->GetEditor() : nullptr;
if (!editor) return false;
return false;
}

bool ConsoleDocument::OnMenuAction (wxCommandEvent& e) {
if (AbstractDocument::OnMenuAction(e)) return true;
size_t id = e.GetId();
switch(id){
case IDM_CLEAR_CONSOLE: ClearConsole(); break;
default: return false;
}
return true;
}

void ConsoleDocument::AddSpecificTools (wxToolBar* toolbar) {
}

void ConsoleDocument::AddSpecificMenus (wxMenuBar* menubar) {
wxMenu* edit = menubar->GetMenu(1);
edit->Append(IDM_CLEAR_CONSOLE, MSG("ClearConsole") + "\tCtrl+Shift+L");
}



