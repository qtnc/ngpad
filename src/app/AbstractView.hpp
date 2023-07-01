#ifndef _____ABSTRACT_VIEW_HPP
#define _____ABSTRACT_VIEW_HPP
#include "../common/wxUtils.hpp"
#include "wx/docview.h"

class AbstractView: public wxView {
protected:
wxMenuBar* menubar = nullptr;
wxToolBar* toolbar = nullptr;
volatile bool closing = false;
std::vector<wxAcceleratorEntry> accelerators;

public:
bool OnCreate (wxDocument* doc, long flags) override;
void Activate (bool activate = true) override;
void OnActivateView (bool activated, wxView* activatedView, wxView* deactivatedView) override;
void OnFocus  (wxChildFocusEvent& e);
void OnChangeFilename () override;
bool OnClose (bool deleteWindow = false) override;
void OnDraw (wxDC* dc) override {}
virtual bool OnMenuAction (wxCommandEvent& e) { return false; }
virtual void OnStatusBarClick (wxMouseEvent& e, int field) { e.Skip(); }
int GetPageIndex ();
void UpdatePageLabel ();

bool IsClosing () { return closing; }
wxMenuBar* GetMenuBar () { return menubar; }
wxToolBar* GetToolBar () { return toolbar; }
wxStatusBar* GetStatusBar ();
virtual void UpdateStatus () {}
bool AddAccelerator (wxAcceleratorEntry& entry);
void UpdateAcceleratorTable ();
};

#endif
