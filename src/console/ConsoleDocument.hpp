#ifndef _____CONSOLE_DOCUMENT_HPP
#define _____CONSOLE_DOCUMENT_HPP
#include "../app/AbstractDocument.hpp"

class ConsoleDocument: public AbstractDocument {
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

void ClearConsole ();
bool ExecuteCommand (const wxString& command);

friend class ConsoleView;
friend class ConsoleEditor;

wxDECLARE_DYNAMIC_CLASS(ConsoleDocument);
};

#endif
