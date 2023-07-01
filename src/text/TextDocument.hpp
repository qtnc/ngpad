#ifndef _____TEXT_DOCUMENT_HPP
#define _____TEXT_DOCUMENT_HPP
#include "../app/AbstractDocument.hpp"
#include <wx/datetime.h>

class TextDocument: public AbstractDocument {
public:
wxTextCtrlIface* GetEditor () const;

int GetEncoding () const;
void SetEncoding (int encoding);
int GetLineEnding () const;
void SetLineEnding (int lineEnding);
int GetIndentType () const;
void SetIndentType (int indentType);

void PushUndoState (struct wxCommand* cmd, bool tryJoin);
void Modify (bool modified = true) override;

bool OnMenuAction (wxCommandEvent& e) override;
bool OnNewDocument () override;
bool DoOpenDocument (const wxString& filename) override;
bool DoSaveDocument (const wxString& filename) override;

bool DoQuickJump (const wxString& cmd) override;

void AddSpecificMenus (wxMenuBar* menubar) override;
void AddSpecificTools (wxToolBar* toolbar) override;

void ChooseEncodingDialog ();
void ChooseLineEndingDialog ();

friend class TextView;
friend class TextEditor;

wxDECLARE_DYNAMIC_CLASS(TextDocument);
};

#endif
