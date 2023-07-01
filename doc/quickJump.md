Title: Quick jump

## General syntax
`file:command?parameters`
The three parts are optional, but the colon is required in most cases when omiting the file, in order to avoid ambiguity.

Quick jump is invoked in several cases:

- Through its dedicated dialog box, Ctrl+J
- When evaluating each command-line argument passed to the process
- When pressing Ctrl+Enter in the text area

Note that it doesn't make sense to omit the file for a command-line argument.

## File part

The file part simply specify which document to open before processing the command.
It can be a full path name, but it can also be a partial name and/or a name with jokers `*` and `?`.
In case of ambiguity, a dialog box asking which file to open will appear.

## Command part


Syntax | Example | Effect
-----|-----|-----
`n` | 123 | go to line n
`l,c` | 12,34 | Go to line l, column c
`l:c` | 12:34 | Go to line l, column c
`[l,c]` | [12,34] | Go to line l, column c, alternative syntax
`[l:c]` | [12:34] | Go to line l, column c, alternative syntax
`s-e`| 5-8 | Select from beginning of line s to end of line e
`sl,sc-el,ec` | 5,1-8,36 | Select from line sl, column sc to line el, column ec
`+n` | +4 | Move n lines down
`-n` | -4 | Move n lines up
`/regex/options` | /sometext/i | Find text and move to the first occurence
**TODO** `s/regex/replacement/options` | s/oldtext/newtext/i | Replace text
`#name` | `#NextMethod` | Move to next marker with name
`$command` | `$dir *.txt` | Execute command and display result in a new file
`$>command` | `$>dir *.txt` | Execute command and insert result in current file
`=expression` | `=6*7` | Execute code and insert result of expression in current file

Currently unused symbols: `%, &, |, ^, >, <, ~, @`

## Parameters part

The last part of the quick jump command is just a list of key/value pairs, which will be put in the properties of the targeted document.
Key/value pairs have to be separated by `&` or `;`.

For example, `?charset=utf-8` will change the encoding of the document to UTF-8.

Note that menu items and other UI elements may not be updated immediately, and note also that some properties can't be changed after the document is loaded, for example the text editor to use.
