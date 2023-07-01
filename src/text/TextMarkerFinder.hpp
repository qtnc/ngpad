#ifndef _____TEXT_MARKER_FINDER_HPP
#include "TextMarkerFinderInterface.hpp"
#define _____TEXT_MARKER_FINDER_HPP
#include "../common/stringUtils.hpp"
#include "../common/wxUtils.hpp"
#include <wx/regex.h>
#include<unordered_map>
#include<functional>

class TextMarkerFinder {
protected:
std::vector<TextMarker> markers;

public:
virtual bool Reset (const wxString& text) = 0;
virtual TextMarker* FindNextMarker (size_t pos, const wxString& pattern = wxEmptyString);
virtual TextMarker* FindPreviousMarker  (size_t pos, const wxString& pattern = wxEmptyString);
virtual TextMarker* FindMarker (size_t pos, const wxString& pattern = wxEmptyString);
virtual bool NameMatches (const wxString& name, const wxString& pattern);
virtual const std::vector<TextMarker>& GetMarkers () { return markers; }
virtual ~TextMarkerFinder () {}

typedef std::function< TextMarkerFinder* (struct Properties&) > Factory;
static TextMarkerFinder* Create (const std::string& name, struct Properties& props);
static void Register (const std::string& name, const Factory& factory);
protected: static inline std::unordered_map<std::string, Factory> factories;
};

#endif
