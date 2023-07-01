#include "AbstractView.hpp"
#include "AbstractDocument.hpp"
#include "App.hpp"
#include <wx/cmdproc.h>
#include "../common/println.hpp"

int AbstractView::GetPageIndex () {
auto window = GetFrame();
auto notebook = wxGetApp() .GetNotebook();
if (!notebook || !window) return wxNOT_FOUND;
if (auto auinb = dynamic_cast<wxAuiNotebook*>(notebook)) return auinb->GetPageIndex(window);
else return notebook->FindPage(window);
}

void AbstractView::UpdatePageLabel () {
auto doc = GetDocument();
auto notebook = wxGetApp() .GetNotebook();
if (notebook) {
int i = GetPageIndex();
if (i>=0) notebook->SetPageText(i, doc->GetUserReadableName());
}
}

void AbstractView::Activate (bool activate) {
wxView::Activate(activate);
if (activate) {
auto notebook = wxGetApp() .GetNotebook();
int pageIndex = GetPageIndex();
if (notebook && pageIndex>=0)  {
notebook->ChangeSelection(pageIndex);
GetFrame()->SetFocus();
wxGetApp() .UpdateTitle();
}
OnUpdate(nullptr, nullptr);
}}

bool AbstractView::OnCreate (wxDocument* wxdoc, long flags) {
if (!wxView::OnCreate(wxdoc, flags)) return false;
auto doc = static_cast<AbstractDocument*>(wxdoc);
auto& app = wxGetApp();
auto mainWindow = app.GetMainWindow();
auto childWindow = app.CreateChildWindow(this);
auto childFrame = wxDynamicCast(childWindow, wxFrame);
auto parentFrame = childFrame? childFrame : mainWindow;

menubar = app.CreateMenuBar();
doc->AddSpecificMenus(menubar);
parentFrame->SetMenuBar(menubar);
doc->GetCommandProcessor()->SetEditMenu(menubar->GetMenu(1));
doc->GetCommandProcessor()->Initialize();

toolbar = app.CreateToolBar(parentFrame);
doc->AddSpecificTools(toolbar);
toolbar->Realize();
parentFrame->SetToolBar(toolbar);
toolbar->Realize();

childWindow->Bind(wxEVT_CHILD_FOCUS, &AbstractView::OnFocus, this);

SetFrame(childWindow);
UpdateAcceleratorTable();
return true;
}

void AbstractView::OnFocus (wxChildFocusEvent& e) {
auto& app = wxGetApp();
auto doc = static_cast<AbstractDocument*>(GetDocument());
if (app.IsValidView(this) && app.IsValidDocument(doc) && !IsClosing() && !app.IsClosing() && !doc->IsClosing()) {
doc->CheckConcurrentModification();
}
e.Skip();
}

void AbstractView::OnChangeFilename () {
wxView::OnChangeFilename();
UpdatePageLabel();
}

bool AbstractView::OnClose (bool deleteWindow) {
if (closing) return true;
closing = wxView::OnClose(deleteWindow);
if (!closing) return false;

auto& app = wxGetApp();
auto mainWindow = app.GetMainWindow();
auto notebook = app .GetNotebook();

if (deleteWindow) {
switch(app.GetWindowMode()) {
case MODE_NB:
case MODE_AUINB:
int pageIndex = notebook->GetSelection();
wxWindow* page = notebook->GetPage(pageIndex);
if (page==GetFrame()) notebook->DeletePage(pageIndex);
break;
}}
return true;
}

void AbstractView::OnActivateView (bool activated, wxView* activatedView, wxView* deactivatedView) {
if (activated && activatedView==deactivatedView) return;
auto& app = wxGetApp();
auto window = GetFrame();
auto frame = wxDynamicCast(window, wxFrame);
auto doc = static_cast<AbstractDocument*>(GetDocument());

if (activated) {
if (!frame && menubar) app.GetMainWindow()->SetMenuBar(menubar);
if (!frame && toolbar) {
toolbar->Realize();
app.GetMainWindow()->SetToolBar(toolbar);
toolbar->Realize();
}
OnUpdate(nullptr, nullptr);
}
}

wxStatusBar* AbstractView::GetStatusBar () {
auto window = GetFrame();
auto frame = GetParentWindow<wxFrame>(window);
return frame? frame->GetStatusBar() : nullptr;
}

bool AbstractView::AddAccelerator (wxAcceleratorEntry& entry) {
if (entry.GetCommand()<=0) {
entry.Set(entry.GetFlags(), entry.GetKeyCode(), 8000 + accelerators.size());
}
if (entry.IsOk()) {
accelerators.push_back(entry);
UpdateAcceleratorTable();
}
return entry.IsOk();
}

void AbstractView::UpdateAcceleratorTable () {
auto& app = wxGetApp();
auto frame = GetFrame();
auto& appAccelerators = app.GetAccelerators();
std::vector<wxAcceleratorEntry> allEntries;
allEntries.insert(allEntries.end(), appAccelerators.begin(), appAccelerators.end());
allEntries.insert(allEntries.end(), accelerators.begin(), accelerators.end());
frame->SetAcceleratorTable(wxAcceleratorTable(allEntries.size(), &allEntries[0]));
}
