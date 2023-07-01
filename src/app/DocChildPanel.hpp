#ifndef _____DOC_CHILD_PANEL_HPP
#define _____DOC_CHILD_PANEL_HPP
#include "../common/wxUtils.hpp"

class DocChildPanel: public wxPanel {
private:
struct wxView* view;

public:
DocChildPanel (wxView* view0, wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize, long style = wxTAB_TRAVERSAL, const wxString& name = wxPanelNameStr):
wxPanel(parent, id, pos, size, style, name), view(view0) {}
wxView* GetView () { return view; }
};

#endif
