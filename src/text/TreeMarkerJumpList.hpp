#ifndef ____TREE_MARKER_JUMP_LIST_HPP____
#define ____TREE_MARKER_JUMP_LIST_HPP____
#include "TreeJumpList.hpp"
#include "TextEditor.hpp"
#include <wx/timer.h>
#include<memory>

class TreeMarkerJumpList: public TreeJumpList {
private:
TextEditor* editor;
std::unique_ptr<wxTimer> timer;
unsigned long long lastUpdate;

public:
TreeMarkerJumpList (wxWindow* parent, TextEditor* editor);

protected:
void OnItemActivate (wxTreeEvent& e) override;
void OnTimerNotify (wxTimerEvent& e);
void OnFocus (wxFocusEvent& e);
void Update (bool force = false);
};

#endif
