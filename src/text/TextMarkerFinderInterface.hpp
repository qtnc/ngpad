#ifndef _____TEXT_MARKER_FINDER_INTERFACE_HPP
#define _____TEXT_MARKER_FINDER_INTERFACE_HPP
#include "../common/stringUtils.hpp"
#include "../common/wxUtils.hpp"

struct TextMarker {
size_t start, end, line, level;
wxString name, displayName;

TextMarker (size_t s, size_t e, size_t li, size_t le, const wxString& n, const wxString& dn): start(s), end(e), line(li), level(le), name(n), displayName(dn) {}
TextMarker (): TextMarker(0, 0, 0, 0, wxEmptyString, wxEmptyString) {}
};

#endif
