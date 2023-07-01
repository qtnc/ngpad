#ifndef ______STRINGUTILS1
#define ______STRINGUTILS1
#include<string>
#include<vector>
#include<boost/algorithm/string.hpp>
#include<regex>

using boost::starts_with;
using boost::ends_with;
using boost::istarts_with;
using boost::iends_with;
using boost::contains;
using boost::icontains;
using boost::iequals;
using boost::replace_all;
using boost::replace_all_copy;
using boost::trim;
using boost::trim_copy;
using boost::to_lower;
using boost::to_lower_copy;
using boost::to_upper;
using boost::to_upper_copy;
using boost::join;

template<class T> std::string itos (T n, int base) {
static const char* digits = "0123456789abcdefghijklmnopqrstuvwxyz";
char buf[64] = {0}, *out = &buf[63];
do {
*(--out) = digits[n%base];
n/=base;
} while(n>0);
return out;
}

inline std::vector<std::string> split (const std::string& input, const std::string& delims, bool compress = false) {
std::vector<std::string> result;
boost::split(result, input, boost::is_any_of(delims), compress? boost::token_compress_on : boost::token_compress_off);
return result;
}

inline void replace_translate (std::string& str, const std::string& src, const std::string& dst) {
for (auto& c: str) {
auto i = src.find(c);
if (i<src.size() && i<dst.size()) c = dst[i];
}}

inline std::string dirname (const std::string& s) {
auto i = s.find_last_of("/\\");
return i==std::string::npos? "" : s.substr(0, i);
}

inline std::string basename (const std::string& s) {
auto i = s.find_last_of("/\\");
return i==std::string::npos? s : s.substr(i+1);
}

template<class T> inline size_t realBeginningOfLine (const T& str) {
auto i = str.find_first_not_of(" \t");
return i==std::string::npos? str.size() : i;
}

template<class T> inline bool isBlank (const T& str) {
return str.empty() || realBeginningOfLine(str) >= str.size();
}

template<class T> void unescape (T& str) {
replace_all(str, "\\\\", "\1");
replace_all(str, "\\t", "\t");
replace_all(str, "\\n", "\n");
replace_all(str, "\\r", "\r");
replace_all(str, "\1", "\\");
}

template <class T> void positionToXY (const T& str, size_t pos, size_t& x, size_t& y) {
y = std::count(str.begin(), str.begin() + pos, '\n');
auto l = str.rfind('\n', pos? pos -1 : 0);
x = l==std::string::npos? pos : pos-l -1;
}

template <class T> size_t xyToPosition (const T& str, size_t x, size_t y) {
if (y==0) return x;
for (size_t i=0, c=0, n=str.size(); i<n; i++) {
if (str[i]=='\n' && ++c==y) return i+x+1;
}
return std::string::npos;
}

template <class T> inline int indexOf (const std::vector<T>& v, const T& val) {
auto it = std::find(v.begin(), v.end(), val);
return it==v.end()? -1 : it-v.begin();
}

template <class I, class F> I find_last_if (const I& begin, const I& end, const I& invalid, const F& pred) {
I cur = end;
while (cur>begin && !pred(*--cur));
return pred(*cur)? cur : invalid;
}

template <class I, class F> I find_last_if_not (const I& begin, const I& end, const I& invalid, const F& pred) {
I cur = end;
while (cur>begin && pred(*--cur));
return pred(*cur)? invalid : cur;
}

#endif
