#ifndef _____PCRE2CPP_HPP_____1
#define _____PCRE2CPP_HPP_____1
#define PCRE2_STATIC 1
#define PCRE2_CODE_UNIT_WIDTH 0
#include "pcre2.h"
#include<string>
#include<stdexcept>
#include<cstdio>


#define Z(C,N) \
inline pcre2_code_##N* pcre2_compile (const C* pattern, size_t patlen, uint32_t options, int* errcode, size_t* errof) { return pcre2_compile_##N( reinterpret_cast<PCRE2_SPTR##N>(pattern), patlen, options, errcode, errof, nullptr); } \
inline int pcre2_match (const pcre2_code_##N* code, const C* subject, size_t subjlen, size_t start, uint32_t options, pcre2_match_data_##N* data) { return pcre2_match_##N(code, reinterpret_cast<PCRE2_SPTR##N>(subject), subjlen, start, options, data, nullptr); } \
inline pcre2_match_data_##N* pcre2_match_data_create_from_pattern (pcre2_code_##N* code) {  return pcre2_match_data_create_from_pattern_##N(code, nullptr); } \
inline void pcre2_code_free (pcre2_code_##N* code) { pcre2_code_free_##N(code); } \
inline void pcre2_match_data_free (pcre2_match_data_##N* data) { pcre2_match_data_free_##N(data); } \
inline pcre2_code_##N* pcre2_code_copy (pcre2_code_##N* code) { return pcre2_code_copy_##N(code); } \
inline int pcre2_substring_length_bynumber (pcre2_match_data_##N* data, uint32_t number, uint32_t* length) { return pcre2_substring_length_bynumber_##N(data, number, length); } \
inline int pcre2_substring_length_byname (pcre2_match_data_##N* data, const C* name, uint32_t* length) { return pcre2_substring_length_byname_##N(data, reinterpret_cast<PCRE2_SPTR##N>(name), length); } \
inline int pcre2_substring_copy_bynumber (pcre2_match_data_##N* data, uint32_t number, C* buf, size_t* buflen) { return pcre2_substring_copy_bynumber_##N(data, number, reinterpret_cast<PCRE2_UCHAR##N*>(buf), buflen); } \
inline int pcre2_substring_copy_byname (pcre2_match_data_##N* data, const C* name, C* buf, size_t* buflen) { return pcre2_substring_copy_byname_##N(data, reinterpret_cast<PCRE2_SPTR##N>(name), reinterpret_cast<PCRE2_UCHAR##N*>(buf), buflen); } \
inline uint32_t pcre2_get_ovector_count (pcre2_match_data_##N* data) { return pcre2_get_ovector_count_##N(data); } \
inline size_t* pcre2_get_ovector_pointer (pcre2_match_data_##N* data) { return pcre2_get_ovector_pointer_##N(data); } \
inline int pcre2_substring_number_from_name (const pcre2_code_##N* code, const C* name) { return pcre2_substring_number_from_name_##N(code, reinterpret_cast<PCRE2_SPTR##N>(name)); } \
inline int pcre2_substitute (const pcre2_code_##N* code, const C* subject, size_t subjlen, size_t start, uint32_t options, pcre2_match_data_##N* data, pcre2_match_context_##N* ctx, const C* repl, size_t repllen, C* out, size_t* outlen) { return pcre2_substitute_##N(code, reinterpret_cast<PCRE2_SPTR##N>(subject), subjlen, start, options, data, ctx, reinterpret_cast<PCRE2_SPTR##N>(repl), repllen, reinterpret_cast<PCRE2_UCHAR##N*>(out), outlen); } \
inline int pcre2_get_error_message (int errcode, C* buf, size_t buflen) { return pcre2_get_error_message_##N(errcode, reinterpret_cast<PCRE2_UCHAR##N*>(buf), buflen); } \

Z(char, 8) Z(wchar_t, 16)
#undef Z

template <class char_type, class string_type> struct PCREStrToCStr {
char_type* operator() (const string_type& s) { return s.c_str(); }
};

template < class char_type, class string_type, class pcre2_code_type, class pcre2_match_data_type, class string_to_cstr  = PCREStrToCStr<char_type, string_type> > 
class PCRE_GENERIC {
protected:
pcre2_code_type* reg;
pcre2_match_data_type* data;
const char_type* subject;
size_t subjlen;
size_t startpos;
string_to_cstr c_str;

static inline void throwError (int errcode) {
char buf[256] = {0};
pcre2_get_error_message_8(errcode, reinterpret_cast<PCRE2_UCHAR8*>(buf), 255);
throw std::runtime_error(buf);
}

static inline int check (int errcode) {
if (errcode<0 && errcode!=PCRE2_ERROR_NOMATCH) throwError(errcode);
return errcode;
}

public:

PCRE_GENERIC  (): reg(nullptr), data(nullptr), subject(nullptr), subjlen(~0), startpos(~0), c_str() {}

PCRE_GENERIC (const string_type& pattern, uint32_t options = 0): 
reg(nullptr), subject(nullptr), subjlen(~0), startpos(~0), c_str()  {
int errcode=0; size_t errof=0;
reg = pcre2_compile(static_cast<const char_type*>(pattern.data()), pattern.size(), options, &errcode, &errof);
if (!reg) throwError(errcode);
data = pcre2_match_data_create_from_pattern(reg);
}

PCRE_GENERIC (PCRE_GENERIC&& p):
reg(p.reg), data(p.data), subject(p.subject), subjlen(p.subjlen), c_str(p.c_str)
{
p.reg = nullptr;
p.data = nullptr;
p.subject = nullptr;
p.subjlen = ~0;
}

PCRE_GENERIC (const PCRE_GENERIC& p):
reg(nullptr), data(nullptr), subject(p.subject), subjlen(p.subjlen), startpos(p.startpos), c_str(p.c_str)
{
reg = p.reg? pcre2_code_copy(p.reg) : nullptr;
data = reg? pcre2_match_data_create_from_pattern(reg) : nullptr;
}

PCRE_GENERIC& operator= (const PCRE_GENERIC& p) {
reg = p.reg? pcre2_code_copy(p.reg) : nullptr;
data = reg? pcre2_match_data_create_from_pattern(reg) : nullptr;
subject = p.subject;
subjlen = p.subjlen;
c_str = p.c_str;
startpos = p.startpos;
return *this;
}

PCRE_GENERIC& operator= (PCRE_GENERIC&& p) {
reg = p.reg;
data = p.data;
subject = p.subject;
subjlen = p.subjlen;
c_str = p.c_str;
startpos = p.startpos;
p.reg = nullptr;
p.data = nullptr;
return *this;
}

~PCRE_GENERIC () {
if (reg) pcre2_code_free(reg);
if (data) pcre2_match_data_free(data);
reg = nullptr;
data = nullptr;
}

inline bool valid () const { return reg && data; }
inline operator bool () const { return valid(); }


bool match (const string_type& source, size_t start = 0, uint32_t options = 0) {
subject = static_cast<const char_type*>(source.data());
subjlen = source.size();
startpos = start;
return check(pcre2_match(reg, subject, subjlen, start, options, data)) >= 0;
}

uint32_t groupCount () {
return pcre2_get_ovector_count(data);
}

size_t start (uint32_t number = 0) {
return pcre2_get_ovector_pointer(data)[2*number];
}

size_t end (uint32_t number = 0) {
return pcre2_get_ovector_pointer(data)[2*number+1];
}

void position (size_t* start, size_t* end = nullptr, uint32_t number = 0) {
auto v = pcre2_get_ovector_pointer(data);
if (start) *start = v[2*number];
if (end) *end = v[2*number+1];
}

size_t length (uint32_t number = 0) {
size_t length=0;
check(pcre2_substring_length_bynumber(data, number, &length));
return length;
}

string_type group (uint32_t number = 0) {
size_t len = length(number);
char_type buf[++len] = {0};
check(pcre2_substring_copy_bynumber(data, number, buf, &len));
return string_type(buf, len);
}

uint32_t groupIndex (const string_type& name) {
return static_cast<uint32_t>(check(pcre2_substring_number_from_name(reg, c_str(name))));
}

size_t start (const string_type& name) {
return start(groupIndex(name));
}

size_t end (const string_type& name) {
return end(groupIndex(name));
}

void position (size_t* start, size_t* end, const string_type& name) {
position(start, end, groupIndex(name));
}

size_t length (const string_type& name) {
size_t length=0;
check(pcre2_substring_length_byname(data, c_str(name), &length));
return length;
}

string_type group (const string_type& name) {
size_t len = length(name);
char_type buf[++len] = {0};
check(pcre2_substring_copy_byname(data, c_str(name), buf, &len));
return string_type(buf, len);
}

string_type getReplacement (const string_type& repl) {
size_t origStart, origEnd, origLen;
position(&origStart, &origEnd, 0);
origLen = origEnd - origStart;
size_t outLen = std::min(origLen * 2, 1024U);
char_type out[outLen] = { 0 };
string_type replacement;
int result = pcre2_substitute(reg, subject, subjlen, startpos, PCRE2_SUBSTITUTE_MATCHED | PCRE2_SUBSTITUTE_EXTENDED | PCRE2_SUBSTITUTE_REPLACEMENT_ONLY | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH, data, nullptr, repl.data(), repl.size(), out, &outLen);
if (result==PCRE2_ERROR_NOMEMORY) {
char_type* out2 = new char_type[++outLen];
check(pcre2_substitute(reg, subject, subjlen, startpos, PCRE2_SUBSTITUTE_MATCHED | PCRE2_SUBSTITUTE_EXTENDED | PCRE2_SUBSTITUTE_REPLACEMENT_ONLY | PCRE2_SUBSTITUTE_OVERFLOW_LENGTH, data, nullptr, repl.data(), repl.size(), out2, &outLen));
replacement = string_type(out2, outLen);
delete[] out2;
}
else {
check(result);
replacement = string_type(out, outLen);
}
return replacement;
}

template <class F>
string_type replace (const string_type& subject, size_t startpos, const F& repl) {
string_type result(subject.data(), startpos);
while(match(subject, startpos)) {
size_t nextpos = start();
result.append(subject.data() +startpos, nextpos-startpos);
result += repl(*this);
startpos = end();
}
result.append(subject.data() +startpos, subjlen-startpos);
return result;
}

string_type replace (const string_type& subject, const string_type& repl, size_t startpos = 0) {
return replace(subject, startpos, [&](auto&r){ return r.getReplacement(repl); });
}

};//PCRE_GENERIC

#ifdef _____WXW0_____INCL0
struct PCREStrToWXCStr {
const wchar_t* operator() (const wxString& s) { return s.wc_str(); }
};

typedef PCRE_GENERIC<wchar_t, wxString, pcre2_code_16, pcre2_match_data_16, PCREStrToWXCStr > WXPCRE;
#endif

typedef PCRE_GENERIC<char, std::string, pcre2_code_8, pcre2_match_data_8> PCRE;

#endif
