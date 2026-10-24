#ifndef _____AGENT_DOCUMENT_HPP
#define _____AGENT_DOCUMENT_HPP
#include "../app/AbstractDocument.hpp"

class AgentDocument: public AbstractDocument {
public:
wxTextCtrlIface* GetEditor () const;

void Modify (bool modified = true) override;

bool OnMenuAction (wxCommandEvent& e) override;
bool OnNewDocument () override;
bool DoOpenDocument (const wxString& filename) override;
bool DoSaveDocument (const wxString& filename) override;

bool DoQuickJump (const wxString& cmd) override;

void AddSpecificMenus (wxMenuBar* menubar) override;
void AddSpecificTools (wxToolBar* toolbar) override;

friend class AgentView;
friend class AgentEditor;

wxDECLARE_DYNAMIC_CLASS(AgentDocument);
};

#endif
