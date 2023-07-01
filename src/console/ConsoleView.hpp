#ifndef _____CONSOLE_VIEW_HPP
#define _____CONSOLE_VIEW_HPP
#include "../app/AbstractView.hpp"
#include "LiveTextAppender.hpp"
#include<memory>

class ConsoleView: public AbstractView {
private:
struct ConsoleEditor* editor = nullptr;
std::unique_ptr<LiveTextAppender> liveAppender1 = nullptr, liveAppender2 = nullptr;
std::unique_ptr<LiveTextSubmitter> liveSubmitter = nullptr;
std::vector<wxString> history;

public:
struct ConsoleEditor* GetEditor () { return editor; }

void AddToHistory (const wxString& cmd);

bool ExecuteCommand (const wxString& cmd);
int GetExecutingCommandInsertionPoint () { return liveAppender1? liveAppender1->GetInsertionPoint() : -1; }
void ExecutingCommandSubmit () { if (liveSubmitter) liveSubmitter->Submit(); }
void MakeLuaConsole ();
void ClearConsole ();

bool OnCreate (wxDocument* doc, long flags) override;
void OnUpdate (wxView* sender, wxObject* param) override;
bool OnMenuAction (wxCommandEvent& e) override;

friend class ConsoleDocument;
friend class ConsoleEditor;

wxDECLARE_DYNAMIC_CLASS(ConsoleView);
};

#endif
