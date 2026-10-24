#include "AgentDocument.hpp"
#include "AgentView.hpp"
#include "AgentEditor.hpp"
#include "../app/App.hpp"
#include "../common/println.hpp"

wxIMPLEMENT_DYNAMIC_CLASS(AgentDocument, wxDocument);

wxTextCtrlIface* AgentDocument::GetEditor () const {
auto wxview = GetFirstView();
auto view = wxview? static_cast<AgentView*>(wxview) :nullptr;
return view? dynamic_cast<wxTextCtrlIface*>(view->GetEditor()) :nullptr;
}

void AgentDocument::Modify (bool modified) {
AbstractDocument::Modify(modified);
auto ta = dynamic_cast<wxTextAreaBase*>(GetEditor());
if (ta && ta->IsModified()!=modified) ta->SetModified(modified);
}

bool AgentDocument::DoOpenDocument (const wxString& filename) {
return true;
}

bool AgentDocument::DoSaveDocument (const wxString& filename) {
return true;
}

bool AgentDocument::OnNewDocument () {
if (!AbstractDocument::OnNewDocument()) return false;
UpdateAllViews();
return true;
}

/*void AgentDocument::ClearAgent () {
auto view = static_cast<AgentView*>(GetFirstView());
view->ClearAgent();
}*/

/*bool AgentDocument::ExecuteCommand (const wxString& cmd) {
auto view = static_cast<AgentView*>(GetFirstView());
return view->ExecuteCommand(cmd);
}*/

bool AgentDocument::DoQuickJump (const wxString& cmd) {
auto view = static_cast<AgentView*>(GetFirstView());
auto editor = view? view->GetEditor() : nullptr;
if (!editor) return false;
return false;
}

bool AgentDocument::OnMenuAction (wxCommandEvent& e) {
if (AbstractDocument::OnMenuAction(e)) return true;
size_t id = e.GetId();
switch(id){
//case IDM_CLEAR_Agent: ClearAgent(); break;
default: return false;
}
return true;
}

void AgentDocument::AddSpecificTools (wxToolBar* toolbar) {
}

void AgentDocument::AddSpecificMenus (wxMenuBar* menubar) {
wxMenu* edit = menubar->GetMenu(1);
//edit->Append(IDM_CLEAR_Agent, MSG("ClearAgent") + "\tCtrl+Shift+L");
}



