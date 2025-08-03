#ifndef _____MEMSTRMWB00_____
#define _____MEMSTRMWB00_____
#include <wx/mstream.h>

struct MemoryInputStreamWithBuffer: wxMemoryInputStream {
std::string* buf;
virtual ~MemoryInputStreamWithBuffer () { delete buf; }
MemoryInputStreamWithBuffer (std::string* b): wxMemoryInputStream(b->data(), b->size()), buf(b) {}
bool Eof () const override { return TellI()>=static_cast<wxFileOffset>(buf->size()); }
};

#endif
