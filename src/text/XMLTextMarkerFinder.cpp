#include "TextMarkerFinder.hpp"
#include <wx/sstream.h>
#include <wx/xml/xml.h>
#include <wx/log.h>
#include<tidy/tidy.h>
#include<tidy/tidybuffio.h>

class XMLTextMarkerFinder: public TextMarkerFinder {
public:
bool Reset (const wxString& text) override;
virtual bool LoadDoc (wxXmlDocument& doc, const wxString& text);
virtual ~XMLTextMarkerFinder () {}
};

class HTMLTextMarkerFinder: public XMLTextMarkerFinder {
bool LoadDoc (wxXmlDocument& doc, const wxString& text) override;
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

bool XMLTextMarkerFinder::LoadDoc (wxXmlDocument& doc, const wxString& text) {
wxStringInputStream in(text);
return doc.Load(in) && doc.IsOk();
}

bool XMLTextMarkerFinder::Reset (const wxString& text) {
wxLogNull logNull;
wxXmlDocument doc;
if (!LoadDoc(doc, text) || !doc.IsOk()) return true;
markers.clear();
addNode(markers, doc.GetRoot(), 0);
return true;
}

bool HTMLTextMarkerFinder::LoadDoc (wxXmlDocument& xmldoc, const wxString& text) {
TidyBuffer out = {0, 0, 0, 0, 0};
TidyBuffer err = {0, 0, 0, 0, 0};
TidyDoc doc = tidyCreate();
finally ___f([=](){ tidyRelease(doc); });
tidyOptSetBool(doc, TidyXhtmlOut, yes);
tidySetErrorBuffer(doc, &err);
std::string html = U(text);
tidyParseString(doc, html.c_str());
tidyCleanAndRepair(doc);
tidyRunDiagnostics(doc);
tidySaveBuffer(doc, &out);
wxStringInputStream in(U(out.bp));
return xmldoc.Load(in);
}

void registerXMLTextMarkerFinder () {
TextMarkerFinder::Register("xml", [](auto&p){ return new XMLTextMarkerFinder(); });
TextMarkerFinder::Register("html", [](auto&p){ return new HTMLTextMarkerFinder(); });
}
