#ifndef _____FIND_REPLACE_DIALOG
#define _____FIND_REPLACE_DIALOG
#include "../common/wxUtils.hpp"
#include "../common/pcre2cpp.hpp"
#include<vector>
#include<memory>

#define FRI_FIND 0
#define FRI_REGEX 1
#define FRI_ICASE 2
#define FRI_WHOLE_WORD 4
#define FRI_RE_DOTALL 8
#define FRI_REPLACE 16
#define FRI_MULTIPLE 32
#define FRI_RE_UNICODE 64
#define FRI_UPWARD 128

struct FindResultInfo {
size_t x, y, pos, len;
wxString filename, lineText, foundText, replacementText;
bool enabled;
FindResultInfo (size_t x0 = 0, size_t y0 = 0, size_t pos0 = 0, size_t len0 = 0, const wxString& file0 = wxEmptyString, const wxString& lineText0 = wxEmptyString, const wxString& foundText0 = wxEmptyString, const wxString& replacementText0 = wxEmptyString, bool enabled0 = true):
x(x0), y(y0), pos(pos0), len(len0), filename(file0), lineText(lineText0), foundText(foundText0), replacementText(replacementText0), enabled(enabled0)  {}
};

struct FindReplaceInfo {
wxString find, replace, glob, rootDir;
int flags;

FindReplaceInfo (): find(), replace(), glob(), rootDir(), flags(0) {}
FindReplaceInfo (int x, const wxString& f = wxEmptyString, const wxString& r = wxEmptyString, const wxString& g = wxEmptyString, const wxString& d = wxEmptyString): 
find(f), replace(r), glob(g), rootDir(d), flags(x) {}

WXPCRE CreateRegEx () const;
bool FindNext (const wxString& text, size_t& start, size_t& end) const;
bool FindPrevious (const wxString& text, size_t& start, size_t& end) const;
bool FindAll (std::vector<FindResultInfo>& results, struct TextView* view, int* nResults = nullptr, int* nFiles = nullptr);
bool FindAllMultiple (std::vector<FindResultInfo>& results, int* nResults = nullptr, int* nFiles = nullptr);
bool FindAllInEditor (std::vector<FindResultInfo>& results, struct TextEditor* editor, const wxString& filename, bool onlySelection, int* nResults = nullptr);
bool FindAllInFile (std::vector<FindResultInfo>& results, const wxString& filename, int* nResults = nullptr);
bool FindAllInText (std::vector<FindResultInfo>& results, const wxString& text, const wxString& filename, size_t start, size_t end, int* nResults = nullptr);
int ReplaceAllMultiple (const std::vector<FindResultInfo>& results);
int ReplaceAllInFile (const wxString& filename, const std::vector<size_t>& acceptedReplacements = {});
int ReplaceAllInEditor (struct TextEditor* editor, const std::vector<size_t>& acceptedReplacements = {});
int ReplaceAllInText (wxString& text, size_t delta, const std::vector<size_t>& acceptedReplacements = {});
};

class FindReplaceDialog: public wxDialog {
private:
wxComboBox *find, *replace;
wxTextCtrl *glob, *rootDir;
wxCheckBox *regex, *icase, *wholeWord, *regDotAll, *regUnicode;
wxRadioBox *direction;
wxButton* rootDirBrowse;
wxStaticText* resultLbl;
struct wxTreeCtrl* foundTree;
struct TextView* view;
std::unique_ptr<wxTimer> timer;
FindReplaceInfo info;
std::vector<FindResultInfo> results;
std::vector<FindReplaceInfo> history;

public:
FindReplaceDialog (wxWindow* parent, const FindReplaceInfo& value, TextView* view = nullptr);
FindReplaceInfo GetValue ();

protected:
void OnFindSelChange (wxCommandEvent& e);
void OnBrowse (wxCommandEvent& e);
void OnCancel (wxCommandEvent& e);
void OnOK (wxCommandEvent& e);
void UpdateFindResults ();
void StartUpdateTimer ();
void UpdateEnabledCheckboxes ();
void StartUpdateTimer (wxCommandEvent& e) { StartUpdateTimer(); }
void UpdateEnabledCheckboxesAndStartTimer (wxEvent& e) { UpdateEnabledCheckboxes(); StartUpdateTimer(); }
void OnTimerNotify (wxTimerEvent& e) { UpdateFindResults();  }
void OnFoundItemActivate (struct wxTreeEvent& e);
void OnFoundItemCheck (const struct wxTreeItemId& treeId, bool checked);
bool LoadHistory ();
bool SaveHistory ();
};

#endif
