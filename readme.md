# NGPad: new generation notepad
NGPad is a text editor especially made to be accessible to screen readers.
It's quite lightweight, configurable and scriptable.
It is made in C++ and is using [wxWidgets](http://wxwidgets.org/)  for its GUI interface.

I hope this time will be the successful one, after three ore or less failed or abandoned attempts.
The only thing which isn't cool this time is the name... I wasn't inspired. The name is probably going to change in the future.

# Features
Main distinctive features of NGPad includes :

* Edit multiple files in a tabbed interface, in MDI mode, or old school one window per file, as you wish
* Read and write files in most popular character encodings as well as 80 less popular ones
* Search and replace using PCRE2 regular expressions, in all files in your project at once
* Use the powerful jump to feature and extended navigation shortcuts to quickly move between and within files in your development projects
* Configure navigation and behavior of the editor based on file type, greatly enhancing the [editorconfig](https://editorconfig.org/) concept
* Customize the editor and add new features on your own with the integrated lua scripting

# Download (for users)
You can download the latest version from this link, version 2023.7.8:
http://vrac.quentinc.net/NGPad.zip

# Documentation (for users)
You can find documentation in the doc directory. Currently these documents are available:

- [Configuration](doc/configuration.md)
- [.editorconfig support](doc/editorconfig.md)
- [Editor shortcuts](doc/editorShortcuts.md)
- [Quick jump feature](doc/quickJump.md)
- [Lua scripting reference](doc/scripting-reference.md)

Other external useful documentations:

- [Lua 5.4 reference manual](https://www.lua.org/manual/5.4/manual.html)

# Dependencies (for developers and contributors)
The following dependencies are needed in order to compile NGPad:

- [WXWidgets](http://wxwidgets.org) 3.2+
- [Lua](http://lua.org) 5.4+
- [PCRE2](https://github.com/PCRE2Project/pcre2) with both 8 and 16 bit code units

# Story
Since a long time, I have been frustrated by default windows' notepad because of its lack of functionalities for developers.
There are of course dedicated text editors especially made for developers, but I wheither find them too heavy, or not as accessible as I would like with a screen reader.

For example, notepad2 and notepad++ are very lightweight, but only partially accessible. Screen readers don't always behave as they should, or don't always read what they need to when necessary.
Sometimes a given version works quite well, but the next one breaks all the accessibility, such as with Notepad++ when switching from version 7 to version 8.

IN the opposite side, complete integrated developement editors like [eclipse](http://eclipse.org/) are known to be not so badly accessible, but they are most of the time too heavy, not so easy to use, take time to start up, and aren't that suited to have a quick look at small files and other notes.
They also do a lot of things under the hoo that aren't always desired and these things aren't easy to configure.

There effectively already exist a text editor made for screen reader users, it is called [EdSharp](http://empowermentzone.com/EdSharp.htm).
However, I find its interface not as easy as it is said; menus are especially full of rarely used features, are quite randomly mixed up, and it lacks an obvious possibility of customizing the whole thing in an easy way. And, of course, I don't program in C#.

NGPad is actually the fourth attempt to create a lightweight and accessible text editor:

- [6pad](http://github.com/qtnc/6pad) (2012-2013) was the first attempt
- [6pad++](http://github.com/qtnc/6pad2) (2015-2016) was the second one
- [Jane](http://github.com/qtnc/jane) (2019) was the third one

The only thing which isn't cool this time is the name... I wasn't inspired.

6pad and 6pad++ worked, well, but C and then C++ were still difficult to master, and I used the Win32 API for GUI, what was also source of strange bugs that have never been solved.
For the third attempt, I decided to start again from the beginning, doing everything in Python and by using the wxPython library for GUI.
However, nobody has really switched to Jane. The few users who previously used 6pad or 6pad++ kept it in spite of it no longer being maintained. I must also admit that Python isn't my cup of tea, so I'm back in the game with C++.


# Contributing
If you have great ideas, found a bug or want to improve documentation, feel free to post issues and send pull requests. You can of course also write me an e-mail directly.
Don't hesitate to also share extensions or scripts you made.

Have fun !

# License
NGPad is distributed as a free software with the [GNU GPL version 3 license](license.txt).
For more information, read license.txt.

