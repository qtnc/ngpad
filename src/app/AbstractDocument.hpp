#ifndef _____ABSTRACT_DOCUMENT_HPP
#define _____ABSTRACT_DOCUMENT_HPP
#include "../common/wxUtils.hpp"
#include "wx/docview.h"
#include "../common/Properties.hpp"

wxDECLARE_EVENT(wxEVT_DOC_CREATING, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_DOC_CREATED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_DOC_LOADING, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_DOC_LOADED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_DOC_SAVING, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_DOC_SAVED, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_DOC_CLOSING, wxCommandEvent);
wxDECLARE_EVENT(wxEVT_DOC_CLOSED, wxCommandEvent);

class AbstractDocument: public wxDocument {
protected:
Properties properties;
wxDateTime lastModifiedTime;
volatile bool closing = false;

public:
Properties& GetProperties () { return properties; }

bool OnCreate (const wxString& filename, long flags) override;
bool OnNewDocument () override;
bool OnSaveDocument (const wxString& filename) override;
bool OnOpenDocument (const wxString& filename) override;
bool OnCloseDocument () override;
void Modify (bool modified) override;
bool IsClosing () { return closing; }
virtual bool Reload () ;
virtual void CheckConcurrentModification ();
virtual wxString GetWorkspaceRoot ();
virtual wxTextCtrlIface* GetEditor () const = 0;

virtual bool DoQuickJump (const wxString& cmd) { return false; }

virtual void AddSpecificMenus (wxMenuBar* menubar) {}
virtual void AddSpecificTools (wxToolBar* toolbar) {}
virtual bool OnMenuAction (wxCommandEvent& e) { return false; }

virtual bool SendEvent (wxEventType type);
};

#endif
