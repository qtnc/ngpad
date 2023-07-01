#include "TextMarkerFinder.hpp"
#include <wx/sstream.h>
#include <wx/xml/xml.h>
#include <wx/log.h>

class XMLTextMarkerFinder: public TextMarkerFinder {
public:
bool Reset (const wxString& text) override;
virtual ~XMLTextMarkerFinder () {}
};

static void addNode (std::vector<TextMarker>& markers, wxXmlNode* node, int level) {
if (!node) return;
if (node->GetType()==wxXML_ELEMENT_NODE && !node->GetName().empty()) {
const wxString& name = node->GetName();
int line = node->GetLineNumber();
markers.emplace_back(0, 0, line, level, name, name);
}
addNode(markers, node->GetChildren(), level+1);
addNode(markers, node->GetNext(), level);
}

bool XMLTextMarkerFinder::Reset (const wxString& text) {
wxLogNull logNull;
wxXmlDocument doc;
wxStringInputStream in(text);
if (!doc.Load(in) || !doc.IsOk()) return true;
markers.clear();
addNode(markers, doc.GetRoot(), 0);
return true;
}

void registerXMLTextMarkerFinder () {
TextMarkerFinder::Register("xml", [](auto&p){ return new XMLTextMarkerFinder(); });
}
