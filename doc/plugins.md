---
title: Writing a Lua plugin
---

A plugin is a Lua script that is loaded automatically, either at startup or when a file matching some editorconfig section is opened. It can use the objects described in the [scripting reference](scripting-reference.html), for example to register commands or react to documents.

## Declaring a plugin

Plugins are declared with the `plugin` key, in `config.ini` or in an `.editorconfig` file:

```ini
plugin=myplugin
```

* In `config.ini`, the plugin is loaded for the whole application.
* In an `.editorconfig` file, put the key in the section matching the files concerned, for example `[*.txt]`. The plugin is loaded when a matching document is opened.

The key can be repeated, or several names can be separated with commas or semicolons:

```ini
plugin=first
plugin=second
plugin=third; fourth
```

## The value is not a file path

The value of `plugin` is **not** the path of a `.lua` file. It is a module name which is passed as is to the Lua function `require()`. Writing

```ini
plugin=myplugin
```

is equivalent to running `require("myplugin")` in Lua.

This means that:

* you must not write the `.lua` extension, nor a drive letter or a directory path such as `C:\scripts\myplugin.lua` or `./myplugin.lua`;
* the script is searched by Lua in the places listed in `package.path`, exactly as for any other call to `require()`;
* sub-directories are written with dots, as in Lua: `plugin=mylib.myplugin` loads `mylib/myplugin.lua`;
* a module is only executed once, even if several documents ask for it, since `require()` remembers modules that are already loaded.

## Where to put the script

With the default Lua search path, a module `myplugin` is found in one of these files:

* `myplugin.lua` in the current directory;
* `lua\myplugin.lua` or `myplugin.lua` next to the application's executable;
* `myplugin\init.lua` in one of the same directories.

If you prefer another location, extend `package.path` from a script that is already loaded, then require your module.

## Writing the script

The file is a normal Lua script, executed once when it is required. Anything it does at the top level happens at load time. By convention a module can return a value, which `require()` caches, but the application ignores it.

```lua
-- myplugin.lua
print("My plugin is loaded")
```

If the script cannot be found or contains an error, loading fails and the error message is reported.

## Troubleshooting

* *module 'xxx' not found*: the name is not resolved by `require()`. Check that you didn't write a path or the `.lua` extension, and that the file lies in one of the directories listed in the error message.
* The plugin doesn't seem to run for a document: check that the `plugin` key is in the right `.editorconfig` section, and that the section matches the file name.
