#include "base.hpp"
#include <wx/filename.h>
#include "../../common/println.hpp"

LuaRegisterValueType(wxFileName);

static wxFileName FileNameConstruct (const wxString& volume, const wxString& path, const wxString& name, const wxString& ext, wxPathFormat format) {
println("Path construct: volume={}, path={}, name={}, ext={}", U(volume), U(path), U(name), U(ext));
if (!volume.empty() && path.empty() && name.empty() && ext.empty()) return wxFileName(volume);
else if (!volume.empty() && !path.empty() && name.empty() && ext.empty()) return wxFileName(volume, path);
else if (!volume.empty() && !path.empty() && !name.empty() && ext.empty()) return wxFileName(volume, path, name);
else if (!volume.empty() && !path.empty() && !name.empty() && !ext.empty()) return wxFileName(volume, path, name, ext);
else if (volume.empty() && !path.empty() && name.empty() && ext.empty()) return wxFileName(path);
else if (volume.empty() && path.empty() && !name.empty() && ext.empty()) return wxFileName(name);
else if (volume.empty() && !path.empty() && !name.empty() && ext.empty()) return wxFileName(path, name);
else if (volume.empty() && !path.empty() && !name.empty() && !ext.empty()) return wxFileName(path, name, ext);
else return wxFileName();
}

#define F(N) \
static int FN##N (lua_State* L) { \
bool re; \
if (lua_isstring(L, 1)) re = wxFileName::N( lua_get<wxString>(L, 1));\
else re = lua_get<wxFileName&>(L, 1) .N(); \
lua_pushboolean(L, re); \
return 1; \
}

F(Exists)
F(FileExists)
F(DirExists)
F(IsDirReadable)
F(IsDirWritable)
F(IsFileReadable)
F(IsFileWritable)
F(IsFileExecutable)
#undef F

static wxString FNGetPath (wxFileName& fn, std::optional<int> flags, wxPathFormat format) {
return fn.GetPath(flags.value_or(wxPATH_GET_VOLUME | wxPATH_GET_SEPARATOR), format);
}

static int FNGetSize (lua_State* L) {
wxULongLong llSize;
if (lua_isstring(L, 1)) llSize = wxFileName::GetSize( lua_get<wxString>(L, 1) );
else llSize = lua_get<wxFileName&>(L, 1) .GetSize();
lua_pushinteger(L, *reinterpret_cast<unsigned long long*>(&llSize));
return 1;
}

static int FNGetHumanReadableSize (lua_State* L) {
wxString nv = lua_get<wxString>(L, 2);
int precision = luaL_optinteger(L, 3, 1);
wxSizeConvention conv = lua_get<wxSizeConvention>(L, 4);
wxString re;
if (lua_isnumber(L, 1)) {
wxULongLong sz;
*reinterpret_cast<unsigned long long*>(&sz) = lua_tointeger(L, 1);
re = wxFileName::GetHumanReadableSize(sz, nv, precision, conv);
}
else re = lua_get<wxFileName&>(L, 1) .GetHumanReadableSize(nv, precision, conv);
return lua_push(L, re);
}

static int FNMkdir (lua_State* L) {
int perms = luaL_optinteger(L, 2, 0);
int flags = luaL_optinteger(L, 3, 0);
bool re;
if (lua_isstring(L, 1)) re = wxFileName::Mkdir( lua_get<wxString>(L, 1), perms, flags);
else re = lua_get<wxFileName&>(L, 1) .Mkdir(perms, flags);
lua_pushboolean(L, re);
return 1;
}

static int FNRmdir (lua_State* L) {
int flags = luaL_optinteger(L, 2, 0);
bool re;
if (lua_isstring(L, 1)) re = wxFileName::Rmdir( lua_get<wxString>(L, 1), flags);
else re = lua_get<wxFileName&>(L, 1) .Rmdir(flags);
lua_pushboolean(L, re);
return 1;
}

static int FNSetCwd (lua_State* L) {
bool re;
if (lua_isstring(L, 1)) re = wxFileName::SetCwd( lua_get<wxString>(L, 1));
else re = lua_get<wxFileName&>(L, 1) .SetCwd();
lua_pushboolean(L, re);
return 1;
}

static std::tuple<wxString, wxString, wxString, wxString, bool> FNSplitPath (const wxString& fullpath, wxPathFormat format) {
wxString volume, path, name, ext;
bool hasExt;
wxFileName::SplitPath(fullpath, &volume, &path, &name, &ext, &hasExt, format);
return std::make_tuple(volume, path, name, ext, hasExt);
}

export int luaopen_Path (lua_State* L) {
lua_pushglobaltable(L);
//T File class allowing file/directory/path manipulations
Binding::LuaClass<wxFileName>(L, "Path")
//C Path constructor. You may specify from 1 to 4 parameters. With 1 parameter you specify the full path. With 2 parameters, you specify the path and the name. With 3 parameters you specify path, name and extension. With 4 parameters you specify volume, path, name and extension. You can additionally specify path options by passing an integer as the last parameter.
//P volume: string: nil: volume
//P path: string: nil: directory path
//P name: string: nil: file name
//P extension: string: nil: file extension
//P format: integer: nil: path otptions and format
.constructor(&FileNameConstruct, {"volume", "path", "name", "extension", "format"})
.destructor()
//M Append a directory component at the end of the path
//P path: string: nil: path component to append
.method("appendDir", &wxFileName::AppendDir)
//M insert a directory component inside the path
//P position: integer: nil: 0-based position where to insert the directory component
//P path: string: nil: path component to insert
.method("insertDir", &wxFileName::InsertDir)
//M Remove a path component
//P position: integer: nil: position of the path component to remove
.method("removeDir", &wxFileName::RemoveDir)
//M Remove the last path component of the path
.method("removeLastDir", &wxFileName::RemoveLastDir)
//M Prepend a path component
//P path: string: nil: path component to prepend
.method("prependDir", &wxFileName::PrependDir)
//A string: path without file name
.property("path", &FNGetPath, &wxFileName::SetPath)
//G string: full path including file name
.getter("fullPath", &wxFileName::GetFullPath)
//G string: full absolute path including file name
.getter("absolutePath", &wxFileName::GetAbsolutePath)
//A string: file name with extension
.property("fullName", &wxFileName::GetFullName, &wxFileName::SetFullName)
//A string: file name without extension
.property("name", &wxFileName::GetName, &wxFileName::SetName)
//A string: file extension
.property("extension", &wxFileName::GetExt, &wxFileName::SetExt)
//G string: long form of full path
.getter("longPath", &wxFileName::GetLongPath)
//G string: short form of full path
.getter("shortPath", &wxFileName::GetShortPath)
//A string: volume
.property("volume", &wxFileName::GetVolume, &wxFileName::SetVolume)
//F Check if a directory exists
//P file: File|string: nil: path to test
//R boolean: true if a directory exists under the path given
.method("dirExists", &FNDirExists)
//F Check if a file exists
//P file: File|string: nil: path to test
//R boolean: true if a file exists under the path given
.method("fileExists", &FNFileExists)
//F Check if a file or directory exists
//P file: File|string: path to test
//R boolean: true if a file or directory exists under the path given
.method("exists", &FNExists)
//F get the current working directory (CWD)
//R string: current working directory (CWD)
.method("getCwd", &wxFileName::GetCwd)
//M get the number of path segments of this File
//R integer: number of path components of this File
.method("getDirCount", &wxFileName::GetDirCount)
//M get all path segments of this File in a table
//R table: table of path segments
.method("getDirs", &wxFileName::GetDirs)
//F get the user's home directory
//R string: user's home directory
.method("getHomeDir", &wxFileName::GetHomeDir)
//M get the size of this File
//R integer: size in bytes of this File
.getter("size", &FNGetSize)
//M checks if this File has a name
//R boolean: true if this file has a name
.method("hasName", &wxFileName::HasName)
//M checks if this File has an extension
//R boolean: true if this file has an extension
.method("hasExt", &wxFileName::HasExt)
//M checks if this File has a volume part
//R boolean: true if this file has a vollume part
.method("hasVolume", &wxFileName::HasVolume)
//M Checks if this File points to an absolute path
//R boolean: true if the path to this File is absolute
.method("isAbsolute", &wxFileName::IsAbsolute)
//M Checks if the file name is case sensitive
//R boolean: true if the file name is case sensitive
.method("isCaseSensitive", &wxFileName::IsCaseSensitive)
//M Checks if this File is in fact a directory
//R boolean: true if this File is in fact a directory
.method("isDir", &wxFileName::IsDir)
//M checks if the directory is readable / listable
//R boolean: true if the directory is readable / listable
.method("isDirReadable", &FNIsDirReadable)
//M checks if the directory is writable / allows to create or delete files inside it
//R boolean: true if the directory is writable / allows to create or delete files inside it
.method("isDirWritable", &FNIsDirWritable)
//M checks if the file is readable
//R boolean: true if the file is readable
.method("isFileReadable", &FNIsFileReadable)
//M checks if the file is writable
//R boolean: true if the file is writable
.method("isFileWritable", &FNIsFileWritable)
//M checks if the file is executable
//R boolean: true if the file is executable
.method("isFileExecutable", &FNIsFileExecutable)
//M Checks if this File points to a relative path
//R boolean: true if the path to this File is relative
.method("isRelative", &wxFileName::IsRelative)
//M Transform a relative path to an absolute path
//P path: string: nil: path to use as a base to make this path absolute
.method("makeAbsolute", &wxFileName::MakeAbsolute)
//M Transform an absolute path to a relative path
//P path: string: nil: path to use as a base to make this path relative to
.method("makeRelativeTo", &wxFileName::MakeRelativeTo)
//M normalize the path
//P options: integer: nil: normalization options
//P path: string: nil: path to use as a base
.method("normalize", (bool(wxFileName::*)(int, const wxString&, wxPathFormat)) &wxFileName::Normalize)
//F create directories
//P path: File|string: nil: path of directories to create
//P permissions: integer: 0: permissions of the new directories
//P flags: integer: nil: additional options
//R boolean: true if the operation is successful
.method("mkdir", &FNMkdir)
//F delete a directory
//P path: File|string: nil: directory to delete
//R boolean: true if the operation is successful
.method("rmdir", &FNRmdir)
//M Change the current working directory (CWD)
//P path: File|string: nil: new current working directory to set
.method("setCwd", &FNSetCwd)
//F Remove the extension of a file name
//P path: string: nil: path from which to remove extension
//R string: path with extension removed
.method("stripExtension", &wxFileName::StripExtension)
.method("touch", &wxFileName::Touch)
//F split a path into volume, path, name, extension
//P path: string: nil: path to split
//R string: volume
//R string: path
//R string: name
//R string: extension
//R boolean: true if the path had an extension
.method("splitPath", &FNSplitPath)
//F get a file size in a human readable form
//P file: File|integer: nil: File object or file size in bytes
//R string: file size in an human readable form
.method("getHumanReadableSize", &FNGetHumanReadableSize)
.pop();
#define C(N) lua_pushfield(L, #N, wx##N);
C(PATH_GET_VOLUME) C(PATH_GET_SEPARATOR) C(PATH_NO_SEPARATOR)
C(PATH_NORM_ENV_VARS) C(PATH_NORM_DOTS) 
C(PATH_NORM_TILDE) C(PATH_NORM_CASE)
C(PATH_NORM_ABSOLUTE) C(PATH_NORM_LONG) 
C(PATH_NORM_SHORTCUT) 
//C(PATH_NORM_ALL)
C(PATH_MKDIR_FULL)
C(PATH_RMDIR_RECURSIVE) C(PATH_RMDIR_FULL)
C(SIZE_CONV_TRADITIONAL) C(SIZE_CONV_IEC) C(SIZE_CONV_SI)
#undef C
lua_pop(L, 1);
lua_getglobal(L, "Path");
return 1;
}
