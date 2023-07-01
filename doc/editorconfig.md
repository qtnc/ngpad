Title: .editorconfig support

## Standard directives

Key | Value type | Default value | Description
-----|-----|-----|-----
charset | string | defined in configuration | Character encoding of the file
indent_style: | tab, space | defined in configuration | Type of indentation
indent_size | integer | defined in configuration  | Number of spaces of a level of indentation when indenting with spaces
insert_final_newline | boolean | false | Whether or not to automatically insert a final blank line at the end of the file
line_Ending | crlf, lf, cr | defined in configuration  | Type of line ending to use in the file
trim_trailing_whitespace | boolean | false | Whether to automatically remove blank characters at the end of each line

## Non-standard directives

Key | Value type | Default value | Description
-----|-----|-----|-----
block_type | indentation, brace, xml | indentation | Way to delimit blocks of code, for use with tree-based navigation with Alt+arrow keys
camel_case_words | boolean | true | Whether to handle camel case words when moving the cursor by word with Ctrl+Left/Right
editor | raw, rich1, rich2 | rich2 | Type of text editor to use
include | string | | Another configuration file to include in place. Path is always relative to the current file. Can be repeated multiple times.
line_wrap | boolean | false | Whether to automatically wrap long lines (true), or use horizontal scrollbar (false)
marker_type | none, regex, markdown | none | Type of marker, used for F2/Shift+F2 navigation and in the tree jump list
plugin | string | | Name of a plugin to load, whether a DLL without extension or a name that will be passed to lua require() function. Can be repeated multiple times.
spell_check | boolean | false | Activate spell check; editor must be rich2
spell_check_language | string | | Language of the spell checker
unindent_with_backspace | boolean | false | Effect when pressing backspace at the beginning of the line: unindent one level (true) or erase to previous line (false)
word_separators | string | native | Characters to use as word separators, affect the behavior of Ctrl+Left/Right; the native behavior of the underlying control is taken unmodified if set to "native" or empty string.


## Editor type
- raw: very fast in loading big files, but don't allow any formatting
- rich1: old version of rich text field, slower but allow formatting
- rich2: rich text field, slower but allows formatting
- **TODO** stc: scintilla text editor, fast and good formatting, but no advanced keyboard navigation and not accessible to screen readers


