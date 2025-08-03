EXENAME=NGPad
DEFINES=$(options) HAVE_W32API_H __WXMSW__ _UNICODE WXUSINGDLL NOPCH

ifeq ($(OS),Windows_NT)
EXT_EXE=.exe
EXT_DLL=.dll
else
EXT_EXE=
EXT_DLL=.so
endif

ifeq ($(mode),release)
NAME_SUFFIX=
DEFINES += RELEASE
CXXOPTFLAGS=-s -O3
else
NAME_SUFFIX=d
DEFINES += DEBUG
CXXOPTFLAGS=-g
endif

EXECUTABLE=$(EXENAME)$(NAME_SUFFIX)$(EXT_EXE)
OBJDIR=obj$(NAME_SUFFIX)/

CXX=g++
WINDRES=windres
WINDRESFLAGS=-c 65001 $(addprefix -D,$(DEFINES)) -I"$(CPATH)"
CXXFLAGS=-std=gnu++17 -Wextra $(addprefix -D,$(DEFINES)) -mthreads
LDFLAGS=-L. -lpcre2-8 -lpcre2-16 -lwxbase33u$(NAME_SUFFIX) -lwxmsw33u$(NAME_SUFFIX)_core -lwxmsw33u$(NAME_SUFFIX)_aui -lwxbase33u$(NAME_SUFFIX)_xml -lwxbase33u$(NAME_SUFFIX)_net -lwxmsw33u$(NAME_SUFFIX)_webview -lwxmsw33u$(NAME_SUFFIX)_stc -lfmt -lole32 -loleaut32 -loleacc -llua -ltidy -mthreads -mwindows -Wl,--allow-multiple-definition

SRCS=$(wildcard src/common/*.cpp) $(wildcard src/app/*.cpp) $(wildcard src/console/*.cpp) $(wildcard src/text/*.cpp) $(wildcard src/lua/binding/*.cpp) $(wildcard src/lua/app/*.cpp)
RCSRCS=$(wildcard src/app/*.rc)
MDSRCS=$(wildcard doc/*.md)
OBJS=$(addprefix $(OBJDIR),$(SRCS:.cpp=.o))
RCOBJS=$(addprefix $(OBJDIR)rsrc/,$(RCSRCS:.rc=.o))
MDOBJS=$(MDSRCS:.md=.html)
PERCENT=%

all: $(EXECUTABLE)

.PHONY: $(EXECUTABLE)

clean:
	rm -r $(OBJDIR)

doc: $(MDOBJS) doc/scripting-reference.md

$(EXECUTABLE): $(RCOBJS) $(OBJS)
	$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -o $@ $^ $(LDFLAGS)

$(OBJDIR)%.o: %.cpp $(wildcard %.hpp)
	mkdir.exe -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(CXXOPTFLAGS) -c -o $@ $<

$(OBJDIR)rsrc/%.o: %.rc
	mkdir.exe -p $(dir $@)
	$(WINDRES) $(WINDRESFLAGS) -o $@ $<

doc/scripting-reference.md: gendoc.lua doc/scripting-reference.mdg $(SRCS)
	lua gendoc.lua $^ $@

doc/%.html: doc/%.md
	pandoc -t html5 -f gfm --standalone -o $@ $<

