#include<optional>
#include "base.hpp"
#include "../../app/App.hpp"
#include "../../app/AbstractDocument.hpp"
#include "../../common/SingleChoiceDialog.hpp"
#include "../../common/ReadOnlyTextDialog.hpp"
#include "../../common/MessageBoxEx.hpp"
#include "../../common/WebViewDialog.hpp"
#include <wx/numdlg.h>
#include <wx/creddlg.h>
#include<map>

wxWindow* GetCurDocWindow () {
auto& app = wxGetApp();
auto doc = app.GetCurrentDocument();
return doc? doc->GetDocumentWindow() : nullptr;
}

static std::optional<wxArrayString> dlgOpenFile (const wxString& title, const wxString& dir, const wxString& file, const wxString& wild, bool multiple) {
auto parent = GetCurDocWindow();
wxFileDialog fd(parent, title, dir, file, wild, wxFD_DEFAULT_STYLE | wxFD_OPEN | wxFD_FILE_MUST_EXIST | (multiple? wxFD_MULTIPLE : 0) );
if (fd.ShowModal()==wxID_OK) {
wxArrayString ar;
fd.GetPaths(ar);
return ar;
}
else return  std::optional<wxArrayString>();
}

static std::optional<wxString> dlgSaveFile (const wxString& title, const wxString& dir, const wxString& file, const wxString& wild) {
auto parent = GetCurDocWindow();
wxFileDialog fd(parent, title, dir, file, wild, wxFD_DEFAULT_STYLE | wxFD_SAVE);
std::optional<wxString> re;
if (fd.ShowModal()==wxID_OK) re = fd.GetPath();
return re;
}

static std::optional<wxArrayString> dlgOpenDir (const wxString& title, const wxString& dir, bool multiple) {
auto parent = GetCurDocWindow();
wxDirDialog dd(parent, title, dir, wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST  | (multiple? wxDD_MULTIPLE : 0));
if (dd.ShowModal()==wxID_OK) {
wxArrayString ar;
dd.GetPaths(ar);
return ar;
}
else return std::optional<wxArrayString>();
}

static std::optional<wxString>	 dlgSaveDir (const wxString& title, const wxString& dir) {
auto parent = GetCurDocWindow();
wxDirDialog dd(parent, title, dir, wxDD_DEFAULT_STYLE);
std::optional<wxString> re;
if (dd.ShowModal()==wxID_OK) re = dd.GetPath();
return re;
}

static std::optional<std::pair<int,wxString>> dlgChooseOne (const wxString& message, const wxString& title, const wxArrayString& options, int selection, bool sorted) {
auto parent = GetCurDocWindow();
SingleChoiceDialog scd(parent, title, message, options, selection -1, sorted);
std::optional<std::pair<int,wxString>> re;
if (scd.ShowModal()==wxID_OK) re = { scd.GetSelection()+1, scd.GetStringSelection() };
return re;
}

static std::optional<wxString> dlgTextEntry (const wxString& message, const wxString& title, const wxString& value) {
std::optional<wxString> re;
auto parent = GetCurDocWindow();
wxTextEntryDialog ted(parent, message, title, value, wxOK | wxCANCEL);
if (ted.ShowModal()==wxID_OK) re = ted.GetValue();
return re;
}

static std::optional<wxString> dlgPasswordEntry (const wxString& message, const wxString& title, const wxString& value) {
std::optional<wxString> re;
auto parent = GetCurDocWindow();
wxTextEntryDialog ted(parent, message, title, value, wxOK | wxCANCEL | wxTE_PASSWORD);
if (ted.ShowModal()==wxID_OK) re = ted.GetValue();
return re;
}

static std::optional<long> dlgNumberEntry (const wxString& message, const wxString& title, const wxString& prompt, long value, long min, long max) {
std::optional<long> re;
auto parent = GetCurDocWindow();
wxNumberEntryDialog ned(parent, message, prompt, title, value, min, max);
if (ned.ShowModal()==wxID_OK) re = ned.GetValue();
return re;
}

static std::optional<std::pair<wxString,wxString>> dlgCredentialEntry (const wxString& message, const wxString& title, const wxString& user, const wxString& password) {
auto parent = GetCurDocWindow();
std::optional<std::pair<wxString,wxString>> re;
wxWebCredentials cred(user, wxSecretValue(password));
wxCredentialEntryDialog ced(parent, message, title, cred);
if (ced.ShowModal()==wxID_OK) {
cred = ced.GetCredentials();
re = { cred.GetUser(), cred.GetPassword().GetAsString() };
}
return re;
}

static int messageBox (const wxString& message, const wxString& title, int style) {
return wxMessageBox(message, title, style, GetCurDocWindow());
}

static int messageBoxEx (const wxString& message, const wxString& title, const wxArrayString& arBtns, int initialButton, int cancelButton) {
std::vector<wxString> buttons;
std::copy(arBtns.begin(), arBtns.end(), std::back_inserter(buttons));
return 1 + MessageBoxEx::Open(GetCurDocWindow(), title, message, buttons, initialButton -1, cancelButton -1);
}

static void dlgWebView (const wxString& url) {
WebViewDialog::Open(GetCurDocWindow(), url);
}

static void openBrowser (const wxString& url) {
wxLaunchDefaultBrowser(url, wxBROWSER_NEW_WINDOW);
}

static int dlgReadOnlyText (const wxString& message, const wxString& title, const wxString& hint, int buttons) {
ReadOnlyTextDialog rtd(GetCurDocWindow(), hint, title, message, buttons);
return rtd.ShowModal();
}

static void alert (const wxString& message, const wxString& title) {
messageBox(message, title, wxOK | wxICON_ASTERISK);
}

static void warning (const wxString& message, const wxString& title) {
messageBox(message, title, wxOK | wxICON_WARNING);
}

static void error (const wxString& message, const wxString& title) {
messageBox(message, title, wxOK | wxICON_ERROR);
}

static bool confirm (const wxString& message, const wxString& title) {
return wxYES == messageBox(message, title, wxYES_NO | wxICON_EXCLAMATION);
}

export int luaopen_Dialogs (lua_State* L) {
lua_newtable(L);
lua_pushvalue(L, -1);
//T Standard dialog boxes and other utilities related to dialog boxes
lua_setglobal(L, "Dialogs");

//F Open a dialog box where the user can choose one or more files to open
//P title: string: '': title of the dialog box
//P directory: string: '': directory initially open when the dialog opens
//P filename: string: '': Name of the file initially selected when the dialog box opens
//P filter: string: '': List of filters in a form like '*.txt|Text files'
//P multiple: boolean: false: whether the user can choose multiple files (true) or only a single one (false, default)
//R table: If the user selected one or more files to open, a table of filenames is returned, otherwise nil if the user cancelled the dialog box.
lua_pushfield(L, "openFile", &dlgOpenFile, {"title", "directory", "filename", "filter", "multiple"});

//F Open a dialog box where the user can choose a file to save to.
//P title: string: '': title of the dialog box
//P directory: string: '': directory initially open when the dialog opens
//P filename: string: '': Name of the file initially selected when the dialog box opens
//P filter: string: '': List of filters in a form like '*.txt|Text files'
//R string: if the user selected a file, it is returned, otherwise nil if the user cancelled the dialog box.
lua_pushfield(L, "saveFile", &dlgSaveFile, {"title", "directory", "filename", "filter"});

//F Open a dialog box where the user can choose one or more directories to open
//P title: string: '': title of the dialog box
//P directory: string: '': directory initially open when the dialog opens
//P multiple: boolean: false: whether the user can choose multiple directories (true) or only a single one (false, default)
//R table: If the user selected one or more directories to open, a table of directories is returned, otherwise nil if the user cancelled the dialog box.
lua_pushfield(L, "openDirectory", &dlgOpenDir, {"title", "directory", "multiple"});

//F Open a dialog box where the user can choose a directory to save into.
//P title: string: '': title of the dialog box
//P directory: string: '': directory initially open when the dialog opens
//R string: If the user selected a directory, it is returned, otherwise nil if the user cancelled the dialog box.
lua_pushfield(L, "saveDirectory", &dlgSaveDir, {"title", "directory"});

//F Prompts the user to choose a single option among a list
//P message: string: '': accompagning message displayed above the list of options
//P title: string: '': title of the dialog box
//P options: table: nil: a table of options the user can choose from
//P selection: integer: 0: 1-based index of the option initially selected when the dialog box opens, or 0 to not select anything by default.
//P sorted: boolean: false: true to automatically sort items in the list
//R integer: the 1-based index of the option selected, or nil if the user cancelled the dialog box
//R string: the string value of the selected option, or nil if the user cancelled the dialog box
lua_pushfield(L, "chooseOne", &dlgChooseOne, {"message", "title", "options", "selection", "sorted"});

//F prompts the user to enter some single line text
//P message: string: '': accompagning message displayed above the input text field
//P title: string: '': title of the dialog box
//P value: string: '': the initial text value present in the text field when the dialog opens
//R string: if the user clicked OK, the entered text is returned, otherwise nil if the user cancelled the dialog box.
lua_pushfield(L, "prompt", &dlgTextEntry, {"message", "title", "value"});

//F prompts the user to enter a password
//P message: string: '': accompagning message displayed above the input text field
//P title: string: '': title of the dialog box
//P value: string: '': the initial text value present in the text field when the dialog opens
//R string: if the user clicked OK, the entered text is returned, otherwise nil if the user cancelled the dialog box.
lua_pushfield(L, "promptPassword", &dlgPasswordEntry, {"message", "title", "value"});

//F prompts the user to enter an integer number
//P message: string: '': accompagning message displayed above the input text field
//P title: string: '': title of the dialog box
//P prompt: string: '': accompagning message displayed on the left of the input text field
//P value: integer: 0: the initial value present in the input field when the dialog opens
//P min: integer: 0: minimum value the user is allowed to enter
//P max: integer: 100: maximum value the user is allowed to enter
//R integer: if the user clicked OK, the entered value is returned, otherwise nil if the user cancelled the dialog box.
lua_pushfield(L, "promptNumber", &dlgNumberEntry, {"message", "title", "prompt", "value", "min", "max"});

//F prompts the user to enter some credentials, i.e. username and password
//P message: string: '': accompagning message displayed above the input text field
//P title: string: '': title of the dialog box
//P username: string: '': the initial username when the dialog opens
//P password: string: '': initial password when the dialog opens
//R string: entered username, or nil if the user cancelled the dialog box
//R string: entered password, or nil if the user cancelled the dialog box
lua_pushfield(L, "promptCredentials", &dlgCredentialEntry, {"message", "title", "username", "password"});

//F Display a simple dialog box with an OK button, suitable for an informational message
//P message: string: '': message displayed in the dialog box
//P title: string: '': title of the dialog box
lua_pushfield(L, "alert", &alert, {"message", "title"});
//F Display a simple dialog box with an OK button, suitable for a warning message
//P message: string: '': message displayed in the dialog box
//P title: string: '': title of the dialog box
lua_pushfield(L, "warning", &warning, {"message", "title"});
//F Display a simple dialog box with an OK button, suitable for an error message
//P message: string: '': message displayed in the dialog box
//P title: string: '': title of the dialog box
lua_pushfield(L, "error", &error, {"message", "title"});

//F Display a simple dialog box with Yes/No buttons
//P message: string: '': message displayed in the dialog box
//P title: string: '': title of the dialog box
//R boolean: true if the user chose Yes, false if the user chose No or closed the dialog box
lua_pushfield(L, "confirm", &confirm, {"message", "title"});

//F Display a simple dialog box with a combination of standard buttons
//P message: string: '': message displayed in the dialog box
//P title: string: '': title of the dialog box
//P style: integer: nil: a combination of values telling which buttons and which icon to show.
//R integer: the ID_XXX value corresponding to the button clicked by the user.
lua_pushfield(L, "messageBox", &messageBox, {"message", "title", "style"});

//F Display a simple dialog box with a message and custom buttons
//P message: string: '': message displayed in the dialog box
//P title: string: '': title of the dialog box
//P buttons: table: nil: list of buttons to display in the dialog box
//P defaultButton: integer: 1: 1-based index of the default button initially selected when the dialog box opens
//P cancelButton: integer: 0: 1-based index of a button that will be triggered when escape is pressed. By default, escape doesn't allow to close implicitly the dialog box.
//R integer: the 1-based index of the button selected by the user
lua_pushfield(L, "messageBoxEx", &messageBoxEx, {"message", "title", "buttons", "defaultButton", "cancelButton"});


//F Open a dialog box presenting a web page
//P url: string: nil: URL of the web page to show. Can also be some HTML code directly.
lua_pushfield(L, "webViewBox", &dlgWebView, {"url"});

//F Open the default browser of the system and navigate to the given URL
//P url: string: nil: URL to open
lua_pushfield(L, "browse", &openBrowser, {"url"});

//F Show a dialog box with a short text, and a longer text that can be shown when pressing on  a detail toggle button
//P message: string: nil: a long message that will initially be hidden
//P title: string: nil: title of the dialog box
//P hint: string: nil: short text which will always be visible
//P style: integer: nil: a value indicating which standard buttons and icons to show
//R integer: the ID of the button clicked by the user
lua_pushfield(L, "messageDetailBox", &dlgReadOnlyText, {"message", "title", "hint", "style"});

#define C(N) lua_pushfield(L, #N, wx##N);

//K integer: flag telling that the OK button must be shown
C(OK)
//K integer: flag telling that the Cancel button must be shown
C(CANCEL)
//K integer: flag telling that the Yes button must be shown
C(YES)
//K integer: flag telling that the No button must be shown
C(NO)
//K integer: flag telling that the Yes and No buttons must be shown
C(YES_NO)
//K integer: flag telling that the Apply button must be shown
C(APPLY)
//K integer: flag telling that the Close button must be shown
C(CLOSE)
//K integer: flag telling that the Help button must be shown
C(HELP)

//K integer: identifier of the OK button
C(ID_OK) 
//K integer: identifier of the Cancel button
C(ID_CANCEL) 
//K integer: identifier of the Yes button
C(ID_YES) 
//K integer: identifier of the No button
C(ID_NO)
//K integer: identifier of the Apply button
C(ID_APPLY) 
//K integer: identifier of the Close button
C(ID_CLOSE) 
//K integer: identifier of the Help button
C(ID_HELP)

//K integer: Flag indicating that the OK button should be selected by default
C(OK_DEFAULT)
//K integer: Flag indicating that the Yes button should be selected by default
C(YES_DEFAULT)
//K integer: Flag indicating that the No button should be selected by default
C(NO_DEFAULT) 
//K integer: Flag indicating that the Cancel button should be selected by default
C(CANCEL_DEFAULT)

//K integer: Flag to display no particular icon
C(ICON_NONE)
//K integer: Flag to display an error icon
C(ICON_ERROR) 
//K integer: Flag to display a warning icon
C(ICON_WARNING) 
//K integer: Flag to display an exclamative icon
C(ICON_EXCLAMATION)
//K integer: Flag to display an informative icon
C(ICON_INFORMATION) 
//K integer: Flag to display an interrogative icon
C(ICON_QUESTION) 
//K integer: Flag to display an icon of a shield
C(ICON_AUTH_NEEDED)

//K integer: flag indicating that the longer text of messageDetailBox must be initially visible (it's hidden by default)
lua_pushfield(L, "DETAILS_OPEN", RTD_DETAILS_OPEN);

return 1;
}

