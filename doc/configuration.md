---
title: Configuration directives
---

Key | Value type | Default value | Description
-----|-----|-----|-----
appearance | none, system, light, dark | none |General UI appearance (see below) 
default_charset  | string | utf-8 | Default encoding for new files
default_indent_size | integer | 4 | Default number of spaces of a level of indentation for new files
default_indent_style |  tab, space | tab | Default type of indentation for new files
default_line_ending | crlf, lf, cr, ls, ps, rs, nel, nul | crlf | Default  line ending for new files
exit_on_last_close | boolean | true | When the last document is closed, exit the application (true) or remain open (false)
notebook_style | top, bottom, left, right, fixed width, multiline | bottom multiline | Style and alignment of notebook tabs
session_mode | never, when_empty, always | never | Tells when files opened at last session should or shouldn't be reloaded (see below)
single_instance | boolean | true | Make sure a single instance is running (true) or allow multiple instances running at the same time (false)
toolbar_style | icon, text, text single line, horizontal, vertical, top, bottom, left, right | horizontal top icon text | Style and alignment of the toolbar
window_mode | sdi, mdi, notebook, auinotebook | auinotebook | Windowing mode (see below)
workspace_root_indicators | string | | comma-separated list of file names which, when found, indicates the possible root of a project workspace

## Appearance
- none: don't try to configure appearance mode in any way (default)
- system: configure appearance in light or dark mode, according to current system settings
- light: configure appearance in light mode
- dark: configure appearance in dark mode

## Session modes
- never: never reload files opened at last session (default)
- always: always reload files opened at last session
- when_empty: reload files opened at last session only when no file is specified on the command-line

## Window modes
- sdi: single document interface, a completely different and independant window for each file
- mdi: multiple document interface, a small window for each file, where all small windows stay inside a single main window
- notebook: each file in its own tab, in the same window
- auinotebook: each file in its own tab, in the same window, with advanced tabs interface (allows to move tabs, split, close button, etc.)
