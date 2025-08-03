Title: .editorconfig support

## Standard directives

Key | Value type | Default value | Description
-----|-----|-----|-----
charset | string | defined in configuration | Character encoding of the file
indent_style: | tab, space | defined in configuration | Type of indentation
indent_size | integer | defined in configuration  | Number of spaces of a level of indentation when indenting with spaces
insert_final_newline | boolean | false | Whether or not to automatically insert a final blank line at the end of the file
line_Ending | crlf, lf, cr, ls, ps, rs, nel, nul | defined in configuration  | Type of line ending to use in the file
trim_trailing_whitespace | boolean | false | Whether to automatically remove blank characters at the end of each line

## Non-standard directives

Key | Value type | Default value | Description
-----|-----|-----|-----
block_type | indentation, brace, xml, regex | indentation | Way to delimit blocks of code, for use with tree-based navigation with Alt+arrow keys
camel_case_words | boolean | true | Whether to handle camel case words when moving the cursor by word with Ctrl+Left/Right
editor | raw, rich1, rich2, stc  | rich2 | Type of text editor to use (see below)
include | string | | Another configuration file to include in place. Path is always relative to the current file. Can be repeated multiple times.
line_wrap | boolean | false | Whether to automatically wrap long lines (true), or use horizontal scrollbar (false)
marker_type | none, regex, markdown, xml, html | none | Type of marker, used for F2/Shift+F2 navigation and in the tree jump list
plugin | string | | Name of a plugin to load, whether a DLL without extension or a name that will be passed to lua require() function. Can be repeated multiple times.
spell_check | boolean | false | Activate spell check; editor must be rich2
spell_check_language | string | | Language of the spell checker
unindent_with_backspace | boolean | false | Effect when pressing backspace at the beginning of the line: unindent one level (true) or erase to previous line (false)
word_separators | string | native | Characters to use as word separators, affect the behavior of Ctrl+Left/Right; the native behavior of the underlying control is taken unmodified if set to "native" or empty string.


## Editor type
- raw: very fast in loading big files, but don't allow any formatting
- rich1: old version of rich text field, slower but allow basic formatting
- rich2: rich text field, slower but allows basic formatting
- stc: scintilla text editor, fast and good formatting, but no advanced keyboard navigation and not 100% accessible to screen readers

## Block types
Indicates how to detect blocks of code. This is most notably used when navigating with Alt+arrow keys.

- brace: a block is delimited with braces `{` `}`
- indentation: a block is delimited by a number of lines having the same indentation level
- regex: blocks are detected with regular expressions (see below)
- xml: a block is given by an XML tag

### Regex blocks
Use the following directives to configure block detection with regular expressions.
Block delimiters are always matched line by line.

Key | Value type | Default value | Description
-----|-----|-----|-----
block_blank_regex | regex | | Regex denoting the equivalent of a blank line when matched
block_close_regex | regex | | Regex matching a block closing
block_open_regex | regex | | Regex matching a block opening


## Marker types
Indicate how to detect markers, which are displayed in the tree jump list, and used when navigating with F2 and Shift+F2 to go to the next or previous marker.

- none: don't use markers; the tree jump list will remain empty and F2/Shift+F2 will not work
- html: use HTML elements as markers
- markdown: use markdown-style headings as markers
- regex: use regular expressions to find out markers, see below
- xml: use XML elements as markers

### Regex markers
Use the following directives to configure marker detection with regular expressions

Key | Value type | Default value | Description
-----|-----|-----|-----
marker_branch_regex | regex | | Regex to find out the name of the marker, for blocks that can be nested (branch nodes)
marker_branch_display_name_index | integer | 1 | 1-based capturing group index of the branch regex to be used as the name displayed in the tree jump list
marker_branch_name_index | integer | 1 | 1-based capturing group index of the branch regex to be used in find commands such as in quick jump
marker_exclude_regex | regex | | When a marker is matched by branch or leaf regex, skip/ignore it if it is also matched by this exclusion regex
marker_leaf_regex | regex | | Regex to find out the name of the marker, for blocks that cannot be nested (leaf nodes)
marker_leaf_display_name_index | integer | 1 | 1-based capturing group index of the leaf regex to be used as the name displayed in the tree jump list
marker_leaf_name_index | integer | 1 | 1-based capturing group index of the leaf regex to be used in find commands such as in quick jump
marker_split_regex  | regex | | Regex used to split potential markers
