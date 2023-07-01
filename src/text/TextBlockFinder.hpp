#ifndef _____TEXT_BLOCK_FINDER_HPP
#define _____TEXT_BLOCK_FINDER_HPP
#include "../common/stringUtils.hpp"
#include "../common/wxUtils.hpp"
#include<functional>
#include<unordered_map>

class TextBlockFinder {
public:
virtual void Reset (int direction = 0) = 0;
virtual int GetBlockLevel (const wxString& text) = 0;
virtual bool IsLikeBlankLine (const wxString& text) = 0;
virtual bool OnEnter (const wxString& previousLine, wxString& newLine, int& curIndent, int& newIndent) = 0;
virtual ~TextBlockFinder () {}

typedef std::function< TextBlockFinder*(struct Properties&) > Factory;
static TextBlockFinder* Create (const std::string& name, struct Properties& props);
static void Register (const std::string& name, const Factory& factory);
protected: static inline std::unordered_map<std::string, Factory> factories;
};

#endif
