#ifndef _____CONSOLE_EDITOR_HPP
#define _____CONSOLE_EDITOR_HPP
#include "../common/wxUtils.hpp"
#include "wx/docview.h"
#include<memory>

class ConsoleEditor: public wxTextCtrl {
private:
struct ConsoleView* view;
size_t historyIndex = 0;

public:
ConsoleEditor (ConsoleView* view, wxWindow* parent, int id, const wxString& text, const wxPoint& pos, const wxSize& size, long flags);

struct ConsoleDocument* GetDocument ();

bool IsEditableAtInsertionPoint (bool bs = false);

void OnHistoryNext ();
void OnHistoryPrevious ();
void RecallHistory ();

void OnChar  (wxKeyEvent& e);
void OnCharHook (wxKeyEvent& e);
void OnCut (wxClipboardTextEvent& e);
void OnPaste (wxClipboardTextEvent& e);
void OnEnter ();
void OnCtrlEnter ();
bool OnBackspace ();
bool OnDelete ();

friend class ConsoleView;
friend class ConsoleDocument;
};

#endif
