Title: Scripting reference

## App
Type representing the application. A single instance of this type exists, under the global variable `app`.

### app:execute (command)
Execute a command and put the result in a new tab/window. This is equivalent to Tools>Run... or F10

**Parameters:**
* command: string: command to execute

**Returns:**
* boolean: true if the execution succeeded. 

### App.getClipboardText ()
Get the current text of the clipboard

**Returns:**
* string: text of the clipboard

### app:getConfig (key)
Get a configuration key

**Parameters:**
* key: string: the configuration key to get

**Returns:**
* string: the configuration value, or an empty string if nothing is defined for that key

### app:getTranslation (key)
Get a translation key

**Parameters:**
* key: string: the translation key to get

**Returns:**
* string: the translation corresponding to that key, or an empty string if it was not found

### app:openDocument (filename)
Open the specified filename, or reactivate (put its tab/window in front) if it's already open

**Parameters:**
* filename: string: file to open or reactivate

**Returns:**
* Document: the document opened or reactivated

### app:openNewDocument ()
Open a new tab/window with an empty document

### app:quickJump (command)
Execute a quick jump command. This is equivalent to Ctrl+J

**Parameters:**
* command: string: quick jump command to execute

**Returns:**
* boolean: true if the quick jump command succeeded

### App.sayText (text)
send some text to the live region, to be spoken by screen readers

**Parameters:**
* text: string: text to be spoken by screen readers

### App.setClipboardText (text)
Set the text of the clipboard

**Parameters:**
* text: string: text to put into the clipboard

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
applicationDirectory | string | true | false | directory where the executable of the application is found
currentDocument | Document | true | false | The currently active document
documents | table | true | false | Table of all documents currently opened in the application
locale | string | true | false | current language of the application
userDataDirectory | string | true | false | directory where user settings are stored
userLocalDataDirectory | string | true | false | directory where local user settings are stored

## CommandEvent
Event for all command-based simple events

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
checked | boolean | true | false | tells if the element or item was just checked or unchecked
extraLong | integer | true | true | second additional information of type integer. The kind of information depends on the event type.
int | integer | true | true | additional information of type integer. The kind of information depends on the event type.
selection | integer | true | true | the 0-based index of the element or item just selected
string | string | true | true | additional information of type string. The kind of information depends on the event type.

### Constants
Name | Type | Description
-----|-----|-----|-----|-----
CONTEXT_MENU | integer | event type identifier for calling the context menu (right click, Shift+F10 or application key)
MENU | integer | type identifier for menu item selection event
TEXT_ENTER | integer | event type identifier for pressing enter in the edition area
TOOL | integer | event identifier for tool selection event
TOOL_ENTER | integer | event type identifier for activating a tool event
TOOL_RCLICKED | integer | event type identifier for right clicking on a tool event

## Dialogs
Standard dialog boxes and other utilities related to dialog boxes

### Dialogs.alert (message, title)
Display a simple dialog box with an OK button, suitable for an informational message

**Parameters:**
* message: string: message displayed in the dialog box
* title: string: title of the dialog box

### Dialogs.browse (url)
Open the default browser of the system and navigate to the given URL

**Parameters:**
* url: string: URL to open

### Dialogs.chooseOne (message, title, options, selection=0, sorted=false)
Prompts the user to choose a single option among a list

**Parameters:**
* message: string: accompagning message displayed above the list of options
* title: string: title of the dialog box
* options: table: a table of options the user can choose from
* selection: integer: 1-based index of the option initially selected when the dialog box opens, or 0 to not select anything by default.
* sorted: boolean: true to automatically sort items in the list

**Returns:**
* integer: the 1-based index of the option selected, or nil if the user cancelled the dialog box
* string: the string value of the selected option, or nil if the user cancelled the dialog box

### Dialogs.confirm (message, title)
Display a simple dialog box with Yes/No buttons

**Parameters:**
* message: string: message displayed in the dialog box
* title: string: title of the dialog box

**Returns:**
* boolean: true if the user chose Yes, false if the user chose No or closed the dialog box

### Dialogs.error (message, title)
Display a simple dialog box with an OK button, suitable for an error message

**Parameters:**
* message: string: message displayed in the dialog box
* title: string: title of the dialog box

### Dialogs.messageBox (message, title, style)
Display a simple dialog box with a combination of standard buttons

**Parameters:**
* message: string: message displayed in the dialog box
* title: string: title of the dialog box
* style: integer: a combination of values telling which buttons and which icon to show.

**Returns:**
* integer: the ID_XXX value corresponding to the button clicked by the user.

### Dialogs.messageBoxEx (message, title, buttons, defaultButton=1, cancelButton=0)
Display a simple dialog box with a message and custom buttons

**Parameters:**
* message: string: message displayed in the dialog box
* title: string: title of the dialog box
* buttons: table: list of buttons to display in the dialog box
* defaultButton: integer: 1-based index of the default button initially selected when the dialog box opens
* cancelButton: integer: 1-based index of a button that will be triggered when escape is pressed. By default, escape doesn't allow to close implicitly the dialog box.

**Returns:**
* integer: the 1-based index of the button selected by the user

### Dialogs.messageDetailBox (message, title, hint, style)
Show a dialog box with a short text, and a longer text that can be shown when pressing on  a detail toggle button

**Parameters:**
* message: string: a long message that will initially be hidden
* title: string: title of the dialog box
* hint: string: short text which will always be visible
* style: integer: a value indicating which standard buttons and icons to show

**Returns:**
* integer: the ID of the button clicked by the user

### Dialogs.openDirectory (title, directory, multiple=false)
Open a dialog box where the user can choose one or more directories to open

**Parameters:**
* title: string: title of the dialog box
* directory: string: directory initially open when the dialog opens
* multiple: boolean: whether the user can choose multiple directories (true) or only a single one (false, default)

**Returns:**
* table: If the user selected one or more directories to open, a table of directories is returned, otherwise nil if the user cancelled the dialog box.

### Dialogs.openFile (title, directory, filename, filter, multiple=false)
Open a dialog box where the user can choose one or more files to open

**Parameters:**
* title: string: title of the dialog box
* directory: string: directory initially open when the dialog opens
* filename: string: Name of the file initially selected when the dialog box opens
* filter: string: List of filters in a form like '*.txt|Text files'
* multiple: boolean: whether the user can choose multiple files (true) or only a single one (false, default)

**Returns:**
* table: If the user selected one or more files to open, a table of filenames is returned, otherwise nil if the user cancelled the dialog box.

### Dialogs.prompt (message, title, value)
prompts the user to enter some single line text

**Parameters:**
* message: string: accompagning message displayed above the input text field
* title: string: title of the dialog box
* value: string: the initial text value present in the text field when the dialog opens

**Returns:**
* string: if the user clicked OK, the entered text is returned, otherwise nil if the user cancelled the dialog box.

### Dialogs.promptCredentials (message, title, username, password)
prompts the user to enter some credentials, i.e. username and password

**Parameters:**
* message: string: accompagning message displayed above the input text field
* title: string: title of the dialog box
* username: string: the initial username when the dialog opens
* password: string: initial password when the dialog opens

**Returns:**
* string: entered username, or nil if the user cancelled the dialog box
* string: entered password, or nil if the user cancelled the dialog box

### Dialogs.promptNumber (message, title, prompt, value=0, min=0, max=100)
prompts the user to enter an integer number

**Parameters:**
* message: string: accompagning message displayed above the input text field
* title: string: title of the dialog box
* prompt: string: accompagning message displayed on the left of the input text field
* value: integer: the initial value present in the input field when the dialog opens
* min: integer: minimum value the user is allowed to enter
* max: integer: maximum value the user is allowed to enter

**Returns:**
* integer: if the user clicked OK, the entered value is returned, otherwise nil if the user cancelled the dialog box.

### Dialogs.promptPassword (message, title, value)
prompts the user to enter a password

**Parameters:**
* message: string: accompagning message displayed above the input text field
* title: string: title of the dialog box
* value: string: the initial text value present in the text field when the dialog opens

**Returns:**
* string: if the user clicked OK, the entered text is returned, otherwise nil if the user cancelled the dialog box.

### Dialogs.saveDirectory (title, directory)
Open a dialog box where the user can choose a directory to save into.

**Parameters:**
* title: string: title of the dialog box
* directory: string: directory initially open when the dialog opens

**Returns:**
* string: If the user selected a directory, it is returned, otherwise nil if the user cancelled the dialog box.

### Dialogs.saveFile (title, directory, filename, filter)
Open a dialog box where the user can choose a file to save to.

**Parameters:**
* title: string: title of the dialog box
* directory: string: directory initially open when the dialog opens
* filename: string: Name of the file initially selected when the dialog box opens
* filter: string: List of filters in a form like '*.txt|Text files'

**Returns:**
* string: if the user selected a file, it is returned, otherwise nil if the user cancelled the dialog box.

### Dialogs.warning (message, title)
Display a simple dialog box with an OK button, suitable for a warning message

**Parameters:**
* message: string: message displayed in the dialog box
* title: string: title of the dialog box

### Dialogs.webViewBox (url)
Open a dialog box presenting a web page

**Parameters:**
* url: string: URL of the web page to show. Can also be some HTML code directly.

### Constants
Name | Type | Description
-----|-----|-----|-----|-----
APPLY | integer | flag telling that the Apply button must be shown
CANCEL | integer | flag telling that the Cancel button must be shown
CANCEL_DEFAULT | integer | Flag indicating that the Cancel button should be selected by default
CLOSE | integer | flag telling that the Close button must be shown
HELP | integer | flag telling that the Help button must be shown
ICON_AUTH_NEEDED | integer | Flag to display an icon of a shield
ICON_ERROR | integer | Flag to display an error icon
ICON_EXCLAMATION | integer | Flag to display an exclamative icon
ICON_INFORMATION | integer | Flag to display an informative icon
ICON_NONE | integer | Flag to display no particular icon
ICON_QUESTION | integer | Flag to display an interrogative icon
ICON_WARNING | integer | Flag to display a warning icon
ID_APPLY | integer | identifier of the Apply button
ID_CANCEL | integer | identifier of the Cancel button
ID_CLOSE | integer | identifier of the Close button
ID_HELP | integer | identifier of the Help button
ID_NO | integer | identifier of the No button
ID_OK | integer | identifier of the OK button
ID_YES | integer | identifier of the Yes button
NO | integer | flag telling that the No button must be shown
NO_DEFAULT | integer | Flag indicating that the No button should be selected by default
OK | integer | flag telling that the OK button must be shown
OK_DEFAULT | integer | Flag indicating that the OK button should be selected by default
YES | integer | flag telling that the Yes button must be shown
YES_DEFAULT | integer | Flag indicating that the Yes button should be selected by default
YES_NO | integer | flag telling that the Yes and No buttons must be shown

## Document
Type representing a document currently opened in the application

### document:activate ()
Activate the document and make it visible

### document:bindAccelerator (accelerator, action)
Bind an accelerator to an action

**Parameters:**
* accelerator: string: accelerator to bind
* action: function: action to execute when the accelerator is triggered

**Returns:**
* handler: an object that can be passed to unbind() in order to cancel the event

### document:close ()
Close the document

### document:getProperty (key)
Get a property of this document

**Parameters:**
* key: string: name of the property to get

**Returns:**
* any: value of the property or nil if not found

### document:reload ()
Reload the document from disk and update the display.

### document:revert ()
Revert the document to the state of its last save.

### document:save ()
Save the document

### document:saveAs ()
Save the document under another name. This opens the "Save as" dialog box.

### document:setProperty (key, value)
Sets a property for this document

**Parameters:**
* key: string: name of the property to set
* value: any: value to set

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
alreadySaved | boolean | true | false | tells if the document hasn't been modified since the last save
editor | TextEditor | true | false | Text edition zone of the document
filename | string | true | true | File name from and into which the document is loaded and saved on disk
menus | MenuBar | true | false | Menu bar of the document
modified | boolean | true | true | tells if the document has been modified since its last save.
properties | table | true | true | all properties of the document
saved | boolean | true | true | tells if the document has already been saved
status | string | true | true | content of the status bar
title | string | true | true | Title of the window or tab for this document
tools | ToolBar | true | false | Toolbar of the document
type | string | true | false | type of document

## Event
Parent class of all events

### event:skip (skip=true)
Set the skip state of the event

**Parameters:**
* skip: boolean: skip state

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
document | Document document in which the event took place | true | false | 
id | integer | true | false | ID of the element or control where the event took place
skipped | boolean | true | true | skipped state of the event
timestamp | integer | true | true | timestamp of the event
type | integer | true | true | type of the event

## EventHandler
Base class of all elements and controls that can react to events

### eventHandler:bind (type, fromId, toId, handler)
Bind an event

**Parameters:**
* type: integer: type of event
* fromId: integer: minimum ID for which the event must be processed. This parameter can be totally omited.
* toId: integer: maximum ID for which the event must be processed. This parameter can be totally omited.
* handler: function: event function to call when the event occurrs. The function must always be the last parameter.

**Returns:**
* handler: an object that can be passed to unbind() in order to cancel the event

### eventHandler:unbind (handler)
unbind an event previously bount with bind()

**Parameters:**
* handler: handler: event handler to unbind

### Constants
Name | Type | Description
-----|-----|-----|-----|-----
DOC_CLOSED | integer | type identifier for document closed event, triggered when a document has been closed
DOC_CLOSING | integer | type identifier for document closing event, triggered when a document is about to be closed
DOC_CREATED | integer | type identifier for document created event, triggered when a document has been created
DOC_CREATING | integer | type identifier for document creating event, triggered when a document is about to be created
DOC_LOADED | integer | type identifier for document loaded event, triggered when a document has been loaded from file
DOC_LOADING | integer | type identifier for document loading event, triggered when a document is about to be loaded from a file
DOC_SAVED | integer | type identifier for document saved event, triggered when a document has been saved to file
DOC_SAVING | integer | type identifier for document saving event, triggered when a document is about to be saved to file

## InputStream
Input stream class

### InputStream (file)
Create a new input stream

**Parameters:**
* file: string|Path: file name or path object referencing the file to open

### inputStream:canRead ()
Checks if the stream can be read without blocking

**Returns:**
* boolean: true if a read operation can be done without blocking

### inputStream:close ()
Closes the stream

### InputStream.from (content)
Create an input stream from an input string

**Parameters:**
* content: string: content of the input stream

**Returns:**
* InputStream: created input stream

### inputStream:read (count, timeout=0)
Read contents from the stream

**Parameters:**
* count: integer: maximum number of bytes to read from the stream
* timeout: integer: the maximum amount of time (in miliseconds) to wait for contents, 0 or negative means infinite

**Returns:**
* string: content read from the stream. It may be less than the maximum requested, including 0. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.

### inputStream:readAll (timeout=0)
Read all the contents til the end of the stream

**Parameters:**
* timeout: integer: the maximum amount of time (in miliseconds) to wait for contents, 0 or negative means infinite

**Returns:**
* string: content read from the stream. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.

### inputStream:readLine (timeout=0)
Read a line of text from the stream, until \n or \r\n, without including it in the returned string. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.

**Parameters:**
* timeout: integer: the maximum amount of time (in miliseconds) to wait for contents, 0 or negative means infinite

**Returns:**
* string: content read from the stream. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.

### inputStream:readTo (out)
Write all content read from this input stream to an output stream

**Parameters:**
* out: OutputStream: Output stream to write to

### inputStream:seek (position, mode='current')
Seek in the stream

**Parameters:**
* position: integer: position to seek to
* mode: string: one of 'begin', 'current' or 'end' to seek respectively from the beginning of the stream, its current position, or from the end.

**Returns:**
* integer: the new position of the stream, or -1 if unknown

### inputStream:tell ()
Tell the current position of the stream

**Returns:**
* integer: the current position of the stream, or -1 if unknown

### inputStream:unread (data)
Put back some content in the stream. That content will be read again.

**Parameters:**
* data: string: data to put back in the stream

## KeyEvent
Event class for all keyboard-related events

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
altDown | boolean | true | false | is Alt key down?
cmdDown | boolean | true | false | is command key down?
controlDown | boolean | true | false | is Ctrl key down?
ctrlDown | boolean | true | false | is Ctrl key down?
keyCode | integer | true | false | key code of the key, see VK_XXX constants
metaDown | boolean | true | false | is meta key down?
modifiers | integer | true | false | modifiers of the key, see MOD_XXX constants
rawControlDown | boolean | true | false | is Ctrl key down?
shiftDown | boolean | true | false | is Shift key down?
unicodeKey | string | true | false | unicode character corresponding to the key

### Constants
Name | Type | Description
-----|-----|-----|-----|-----
CHAR | integer | event type identifier for character key press event
CHAR_HOOK | integer | event type identifier for character key press event
KEY_DOWN | integer | event type identifier for key down event
KEY_UP | integer | event type identifier for key up event

## Menu
Type representing a menu, whether a menu in the menubar, or a popup menu

### menu:add (label, subMenu, helpString, position=0, id=0, checkable=false, radio=false, checked=false)
Add a new menu item to this menu

**Parameters:**
* label: string: text label of the item
* subMenu: Menu: a submenu to attach to this item
* helpString: string: the help string displayed on the status bar when the item is highlighted
* position: integer: position of the item inside the menu (1-based)
* id: integer: unique identifier of the item (0 = automatically allocate any ID)
* checkable: boolean: tells if the menu item is checkable, i.e. has a checkbox that can be checked
* radio: boolean: Tells if this menu item is a radio button item
* checked: boolean: Tells if this menu item is checked

**Returns:**
* MenuItem: the item created and added in this menu

### menu:addSeparator (position=0)
add a separator to this menu

**Parameters:**
* position: integer: position of the item in the menu (1-based)

**Returns:**
* MenuItem: item created and added in this menu

### menu:check (id, checked=true)
Check a menu item by its ID

**Parameters:**
* id: integer: ID of the item to check or uncheck
* checked: boolean: state checked or unchecked to apply

### Menu.chooseOne (options)
Show a popup/context menu with simple text options to choose from

**Parameters:**
* options: table: a table of options to choose from

**Returns:**
* integer: the 1-based index of the options selected by the user, 0 or -1 if the menu has been closed without choosing any option

### menu:enable (id, enabled=true)
Enable a menu item by its ID

**Parameters:**
* id: integer: ID of the item to enable or disable
* enabled: boolean: state enabled or disabled to apply

### menu:getMenuItemCount ()
Return the number of items present in this menu

**Returns:**
* integer: the number of items present in this menu

### menu:isChecked (id)
Tells if a given menu item is checked

**Parameters:**
* id: integer: Id of the menu item to check

**Returns:**
* boolean: true if the given menu item exists and is checked

### menu:isEnabled (id)
Tells if a menu item given by its ID is enabled

**Parameters:**
* id: integer: ID of the menu item to check

**Returns:**
* boolean: true if the given item exists and is enabled

### menu:remove (itemOrId)
Remove an item from this menu

**Parameters:**
* itemOrId: integer | MenuItem: the item to remove from this menu, whether a MenuItem object, or an integer denothing the 1-based position or the unique identifier of the item

### menu:show ()
Displays this menu as a popup/context menu

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
name | string | true | true | internal name of this menu
parent | Menu | true | false | parent menu of this menu

## MenuBar
Type representing the menu bar of a document window or tab

### menuBar:add (label, menu, position=0)
Add a new menu in this menu bar

**Parameters:**
* label: string: the text label of the menu, displayed in the menu bar
* menu: Menu: the menu to add
* position: integer: the 1-based position where to add the new menu

### menuBar:check (id, checked=true)
Check or uncheck a menu item by its ID

**Parameters:**
* id: integer: menu item ID
* checked: boolean: state checked or unchecked to apply

### menuBar:enable (id, enable=true)
Enable or disable a menu item by its ID

**Parameters:**
* id: integer: menu item ID
* enable: boolean: enabled or disabled state to apply

### menuBar:getMenu (indexOrName)
fetch a menu from this menu bar

**Parameters:**
* indexOrName: integer | string: 1-based index or name of the menu to fetch

**Returns:**
* Menu: the requested menu 

### menuBar:getMenuCount ()
Return the number of menus present in this menu bar

**Returns:**
* integer: the number of menus in this menu bar

### menuBar:isChecked (id)
Checks the checked state of a menu item by its ID

**Parameters:**
* id: integer: ID of the menu item

**Returns:**
* boolean: checked state

### menuBar:isEnabled (id)
Checks if a menu item is enabled or disabled by its ID

**Parameters:**
* id: integer: menu item ID

**Returns:**
* boolean: enabled or disabled state

### menuBar:remove (index)
Remove a menu from this menu bar

**Parameters:**
* index: integer: 1-based index of the menu to remove

## MenuItem
Type representing a single menu item

### menuItem:bind (handler)
Bind an event handler to this item

**Parameters:**
* handler: function: the event handler to bind to this item

**Returns:**
* handler: an object allowing to unbind the event handler later on

### menuItem:check (checked=true)
Checks or unchecks the checkbox of this item

**Parameters:**
* checked: boolean: The state checked or unchecked to apply

### menuItem:enable (enabled=true)
Enable or disable this item

**Parameters:**
* enabled: boolean: the enabled or disabled state to apply

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
checkable | boolean | true | false | Tells if this item is checkable, i.e. if it has a checkbox
checked | boolean | true | true | Tells if this item is currently checked
enabled | boolean | true | true | Tells if this item is currently enabled
helpString | string | true | true | the help string associated with this item. The help string is displayed in the status bar while the item is highlighted.
id | integer | true | false | unique identifier of this item
label | string | true | true | the text label of this item
labelText | string | true | false | the text label of this item, without including accelerators
menu | Menu | true | true | the parent menu of this item
radio | boolean | true | false | Tells if this item is checkable and if it's a radio button
separator | boolean | true | false | Tells if this item is a separator
subMenu | Menu | true | true | the submenu included in this item

## MouseEvent
Class for mouse events

### mouseEvent:getPosition ()
get the position of the mouse

**Returns:**
* integer: X coordinate
* integer: Y coordinate

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
altDown | boolean | true | false | is Alt key down?
button | integer | true | false | flags indicating which buttons where down
clickCount | integer | true | false | number of consecutive clicks (1=single click, 2=double click, 3=triple click)
cmdDown | boolean | true | false | is command key down?
controlDown | boolean | true | false | is Ctrl key down?
ctrlDown | boolean | true | false | is Ctrl key down?
leftDown | boolean | true | false | is the left button down?
metaDown | boolean | true | false | is meta key down?
middleDown | boolean | true | false | is the middle button down?
modifiers | integer | true | false | modifier keys (see MOD_XXX constants)
rawControlDown | boolean | true | false | is Ctrl key down?
rightDown | boolean | true | false | is the right button down?
shiftDown | boolean | true | false | is Shift key down?
wheelRotation | integer | true | false | rotation of the mouse wheel
x | integer | true | false | X coordinate of the mouse
y | integer | true | false | Y coordinate of the mouse

### Constants
Name | Type | Description
-----|-----|-----|-----|-----
ENTER_WINDOW | integer | event type identifier for mouse entering event
LEAVE_WINDOW | integer | event type identifier for mouse leaving event
LEFT_DCLICK | integer | event type identifier for left double click event
LEFT_DOWN | integer | event type identifier for left mouse down event
LEFT_UP | integer | event type identifier for left mouse up event
MIDDLE_DCLICK | integer | event type identifier for middle double click event
MIDDLE_DOWN | integer | event type identifier for middle mouse down event
MIDDLE_UP | integer | event type identifier for middle mouse up event
MOTION | integer | event type identifier for mouse motion event
MOUSEWHEEL | integer | event type identifier for mouse wheel rotation event
RIGHT_DCLICK | integer | event type identifier for right double click event
RIGHT_DOWN | integer | event type identifier for right mouse down event
RIGHT_UP | integer | event type identifier for right mouse up event

## OutputStream
Output stream class

### OutputStream (file)
Create a new output stream. You can open a file by passing one parameter, or create an in-memory stream by passing no parameter.

**Parameters:**
* file: string|Path: File name or path referencing a file to open for writing, or nil to create an in-memory stream

### outputStream:close ()
Close the stream

### outputStream:seek (position, anse='current')
Seek in the stream

**Parameters:**
* position: integer: position to seek to
* anse: string: one of 'begin', 'current' or 'end' to seek respectively from the beginning of the stream, its current position, or from the end.

**Returns:**
* integer: the new position of the stream, or -1 if unknown

### outputStream:tell ()
Tells the current position of the stream

**Returns:**
* integer: the position of the stream, or -1 if unknown

### outputStream:toString ()
Converts all the content written to this stream into a string. It works only for in-memory buffers created with the no-args constructor.

**Returns:**
* string: content of the stream as a string. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.

### outputStream:write (data)
Write content to the stream

**Parameters:**
* data: string: data to write

**Returns:**
* integer: number of bytes written

### outputStream:writeFrom (input)
Write all data read from an input stream

**Parameters:**
* input: InputStream: input stream to read from

## Path
File class allowing file/directory/path manipulations

### Path (volume, path, name, extension, format)
Path constructor. You may specify from 1 to 4 parameters. With 1 parameter you specify the full path. With 2 parameters, you specify the path and the name. With 3 parameters you specify path, name and extension. With 4 parameters you specify volume, path, name and extension. You can additionally specify path options by passing an integer as the last parameter.

**Parameters:**
* volume: string: volume
* path: string: directory path
* name: string: file name
* extension: string: file extension
* format: integer: path otptions and format

### path:appendDir (path)
Append a directory component at the end of the path

**Parameters:**
* path: string: path component to append

### Path.dirExists (file)
Check if a directory exists

**Parameters:**
* file: File|string: path to test

**Returns:**
* boolean: true if a directory exists under the path given

### Path.exists (file=path to test)
Check if a file or directory exists

**Parameters:**
* file: File|string: 

**Returns:**
* boolean: true if a file or directory exists under the path given

### Path.fileExists (file)
Check if a file exists

**Parameters:**
* file: File|string: path to test

**Returns:**
* boolean: true if a file exists under the path given

### Path.getCwd ()
get the current working directory (CWD)

**Returns:**
* string: current working directory (CWD)

### path:getDirCount ()
get the number of path segments of this File

**Returns:**
* integer: number of path components of this File

### path:getDirs ()
get all path segments of this File in a table

**Returns:**
* table: table of path segments

### Path.getHomeDir ()
get the user's home directory

**Returns:**
* string: user's home directory

### Path.getHumanReadableSize (file)
get a file size in a human readable form

**Parameters:**
* file: File|integer: File object or file size in bytes

**Returns:**
* string: file size in an human readable form

### path:hasExt ()
checks if this File has an extension

**Returns:**
* boolean: true if this file has an extension

### path:hasName ()
checks if this File has a name

**Returns:**
* boolean: true if this file has a name

### path:hasVolume ()
checks if this File has a volume part

**Returns:**
* boolean: true if this file has a vollume part

### path:insertDir (position, path)
insert a directory component inside the path

**Parameters:**
* position: integer: 0-based position where to insert the directory component
* path: string: path component to insert

### path:isAbsolute ()
Checks if this File points to an absolute path

**Returns:**
* boolean: true if the path to this File is absolute

### path:isCaseSensitive ()
Checks if the file name is case sensitive

**Returns:**
* boolean: true if the file name is case sensitive

### path:isDir ()
Checks if this File is in fact a directory

**Returns:**
* boolean: true if this File is in fact a directory

### path:isDirReadable ()
checks if the directory is readable / listable

**Returns:**
* boolean: true if the directory is readable / listable

### path:isDirWritable ()
checks if the directory is writable / allows to create or delete files inside it

**Returns:**
* boolean: true if the directory is writable / allows to create or delete files inside it

### path:isFileExecutable ()
checks if the file is executable

**Returns:**
* boolean: true if the file is executable

### path:isFileReadable ()
checks if the file is readable

**Returns:**
* boolean: true if the file is readable

### path:isFileWritable ()
checks if the file is writable

**Returns:**
* boolean: true if the file is writable

### path:isRelative ()
Checks if this File points to a relative path

**Returns:**
* boolean: true if the path to this File is relative

### path:makeAbsolute (path)
Transform a relative path to an absolute path

**Parameters:**
* path: string: path to use as a base to make this path absolute

### path:makeRelativeTo (path)
Transform an absolute path to a relative path

**Parameters:**
* path: string: path to use as a base to make this path relative to

### Path.mkdir (path, permissions=0, flags)
create directories

**Parameters:**
* path: File|string: path of directories to create
* permissions: integer: permissions of the new directories
* flags: integer: additional options

**Returns:**
* boolean: true if the operation is successful

### path:normalize (options, path)
normalize the path

**Parameters:**
* options: integer: normalization options
* path: string: path to use as a base

### path:prependDir (path)
Prepend a path component

**Parameters:**
* path: string: path component to prepend

### path:removeDir (position)
Remove a path component

**Parameters:**
* position: integer: position of the path component to remove

### path:removeLastDir ()
Remove the last path component of the path

### Path.rmdir (path)
delete a directory

**Parameters:**
* path: File|string: directory to delete

**Returns:**
* boolean: true if the operation is successful

### path:setCwd (path)
Change the current working directory (CWD)

**Parameters:**
* path: File|string: new current working directory to set

### path:size ()
get the size of this File

**Returns:**
* integer: size in bytes of this File

### Path.splitPath (path)
split a path into volume, path, name, extension

**Parameters:**
* path: string: path to split

**Returns:**
* string: volume
* string: path
* string: name
* string: extension
* boolean: true if the path had an extension

### Path.stripExtension (path)
Remove the extension of a file name

**Parameters:**
* path: string: path from which to remove extension

**Returns:**
* string: path with extension removed

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
absolutePath | string | true | false | full absolute path including file name
extension | string | true | true | file extension
fullName | string | true | true | file name with extension
fullPath | string | true | false | full path including file name
longPath | string | true | false | long form of full path
name | string | true | true | file name without extension
path | string | true | true | path without file name
shortPath | string | true | false | short form of full path
volume | string | true | true | volume

## Process
Process class allowing to execute external commands

### Process (command)
Create and launch a new process

**Parameters:**
* command: string: command-line containing the path to the executable to run as well as all command-line parameters

### process:activate ()
Attempt to activate the main window of the process

### process:bind (handler)
Bind an event listener that will be triggered when the process terminates

**Parameters:**
* handler: function: handler that will be called when the process terminates

### process:closeOutput ()
Close the output stream connected to the standard input stream of the process, indicating to it that you aren't going to send data anymore

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
errorAvailable | boolean | true | false | tells if there is data available on the error stream
errorStream | InputStream | true | false | input stream connected to the standard error stream of the process
inputAvailable | boolean | true | false | tells if there is data available on the input stream
inputStream | InputStream | true | false | input stream connected to the standard output stream of the process
outputStream | OutputStream | true | false | output stream connected to the standard input stream of the process
pid | integer | true | false | process ID of the process

## ProcessEvent
Event notified when a process terminates

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
exitCode | integer | true | false | exit code of the process, usually 0 for success, any positive value when the process failed, and negative value when the process couldn't be executed at all
pid | integer | true | false | Process ID of the process

## TextEditor
Type representing a text edition zone. 

### textEditor:appendText (text)
Append some text at the end of the zone

**Parameters:**
* text: string: text to append

### textEditor:canCopy ()
Checks if some text can be copied right now

**Returns:**
* boolean: true if some text can be copied

### textEditor:canCut ()
Checks if some text can be cut right now

**Returns:**
* boolean: true if some text can be cut

### textEditor:canPaste ()
Checks if some text can be pasted right now

**Returns:**
* boolean: true if some text can be pasted

### textEditor:canRedo ()
Checks if the last operation can be redone

**Returns:**
* boolean: true if the last operation can be redone

### textEditor:canUndo ()
Checks if the last operation can be undone

**Returns:**
* boolean: true if the last operation can be undone

### textEditor:changeValue (text)
Change the content of the zone without triggering events

**Parameters:**
* text: string: new text

### textEditor:clear ()
Clear the entire zone

### textEditor:columnLineToPosition (x, y)
Convert a Column/Line based position into a character position

**Parameters:**
* x: integer: X coordinate / column (1-based)
* y: integer: Y coordinate / Line (1-based, 0=current line, negatives counts from the end.

**Returns:**
* integer: 1-based charachter position

### textEditor:getLine (line=0)
Get a line of text from the zone

**Parameters:**
* line: integer: 1-based line number, 0=current line, negatives counts from the end.

**Returns:**
* string: the text of the given line

### textEditor:getLineLength (line=0)
Get the length of a line

**Parameters:**
* line: integer: 1-based line number, 0=current line, negatives counts from the end.

**Returns:**
* integer: length of the given line

### textEditor:getNumberOfLines ()
Gets the total number of lines of the zone

**Returns:**
* integer: number of lines of the zone

### textEditor:getRange (start, end)
Get a range of text from the zone

**Parameters:**
* start: integer: start of the range (1-based, negatives counts from the end)
* end: integer: end of the range (1-based, negatives counts from the end)

**Returns:**
* string: the requested range of text

### textEditor:getSelection ()
Get the current position of the selection anchor and end points (1-based)

**Returns:**
* integer: anchor point of the current selection
* integer: end point of the current selection

### textEditor:positionToColumnLine (position=0)
Convert a 1-based character position into an X/Y or Column/Line based position

**Parameters:**
* position: integer: 1-based character position, 0=current position.

**Returns:**
* integer: X coordinate / Column
* integer: Y coordinate / Line

### textEditor:remove (start, end)
Remove some text from the zone

**Parameters:**
* start: integer: position of the first character to remove (1-based, negatives counts from the end)
* end: integer: position of the last character to remove (1-based, negatives counts from the end)

### textEditor:replace (start, end, replacement)
Remove some text from the zone and replace it by something else

**Parameters:**
* start: integer: position of the first character to remove (1-based, negatives counts from the end)
* end: integer: position of the last character to remove (1-based, negatives counts from the end)
* replacement: string: new text to be inserted at the place of the removed region

### textEditor:selectAll ()
Select the entire zone

### textEditor:selectNone ()
Deselect everything

### textEditor:setSelection (start, end)
Change the selection anchor and end points

**Parameters:**
* start: integer: selection start / anchor point (1-based, negatives counts from the end)
* end: integer: selection end point (1-based, negatives counts from the end)

### textEditor:writeText (text)
Write some text at the current insertion point

**Parameters:**
* text: string: text to write

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
editable | boolean | true | true | tells if the zone is editable
empty | boolean | true | false | tells if the zone is empty
insertionPoint | integer | true | true | current position of the insertion point (1-based)
lastPosition | integer | true | false | position corresponding to the end of the zone
modified | boolean | true | true | Tells if the text content of the zone has been modified since the last save
selectedText | string | true | false | currently selected text
value | string | true | true | entire text of the zone

## Timer
Timer class

### timer:bind (handler)
Bind an event handler to this timer

**Parameters:**
* handler: function: event handler to be triggered when the time is out

**Returns:**
* handler: a event handle that can be passed to app.unbind

### timer:start (interval=-1, oneShot=false)
Start the timer and run it periodically or one shot. If the timer is already running, it is reset.

**Parameters:**
* interval: integer: interval in miliseconds. If 0 or negative, use the previous interval.
* oneShot: boolean: set time timer to be periodic (false) or one shot (true)

**Returns:**
* boolean: true if the timer has started, false if it was unable to start

### timer:startOnce (interval=-1)
Start the timer tand run it only once (one shot). If the timer is already running, it is reset.

**Parameters:**
* interval: integer: interval in miliseconds. If 0 or negative, use the previous interval.

**Returns:**
* boolean: true if the timer has started, false if it was unable to start

### timer:stop ()
Stops the timer if it's running.

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
id | integer | true | false | ID of the event generated
interval | integer | true | false | interval of the timer in miliseconds
oneShot | boolean | true | false | tells if the timer is periodic (false) or one shot (true).
running | boolean | true | false | tells if the timer is currently running (true) or stopped (false).

## WebRequest
Class allowing to make web requests

### WebRequest (url, method='GET', data, contentType, contentLength)
Create a new web request

**Parameters:**
* url: string: URL of the request
* method: string: HTTP method of the request
* data: string|InputStream: a string or an input stream to send as request content. The method is automatically changed from 'GET' to 'POST'  if necessary.
* contentType: string: Content-Type for request content
* contentLength: integer: length of the request content in bytes

### webRequest:setData (data, contentType, contentLength)
Set the request body

**Parameters:**
* data: string|InputStream: string or input stream of the request body
* contentType: string: Content-Type of the request body
* contentLength: integer: length of the data in bytes

### webRequest:setHeader (name, value)
SEt a request header

**Parameters:**
* name: string: header name
* value: string: header value

### webRequest:submit (callback)
Effectively launch the web request. The request is launched asynchronously, and a callback function is called when the response is fully received.

**Parameters:**
* callback: function: function to call when the response is received. The fonction receive a WebResponse object as parameter.

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
method | string | false | true | HTTP method of the request
verifyDisabled | boolean | true | true | whether to ignore SSL/TLS certificate verifications. Note that disabling SSL/TLS certificate verifications make requests less secure.

## WebResponse
Object containing a response to a web request

### webResponse:getHeader (name)
Get a response header 

**Parameters:**
* name: string: header name

**Returns:**
* string: header value, or empty string if not found

### Properties
Name | Type | Readable | Writable | Description
-----|-----|-----|-----|-----
contentLength | integer | true | false | size in bytes of the response, or -1 if unknown
contentType | string | true | false | Content type of the response
responseFileName | string | true | false | file name where the response is stored if storing in a file was requested
responseStream | InputStream | true | false | Get an input stream for reading the response content
responseText | string | true | false | content of the response as a string. Note that the string is returned as it is read. It may not be encoded in UTF-8, and may even be binary data not humanly readable.
status | integer | true | false | HTTP response code of the response, for example 200
statusText | string | true | false | Text of the HTTP response code, for example 'OK'
suggestedFileName | string | true | false | suggested file name for downloading a file
url | string | true | false | final URL of the resource. This can be different from the original requested URL in case of redirection.

## io
Additions to the standard io lua module

### io.readfile (filename, properties)
Reads a file from disk

**Parameters:**
* filename: string: name of the file to read
* properties: table: table of properties specifying encoding, line ending, etc.

**Returns:**
* string: content of the file, read and decoded as requested

### io.writefile (filename, text, properties)
Write a file to disk

**Parameters:**
* filename: string: name of the file to write to
* text: string: text content of the file
* properties: table: table of properties specifying encoding, line ending, etc.

## json
JSON parsing library

### json.dump (values...)
Dump a lua value into JSON

**Parameters:**
* values...: any: any lua values to convert to JSON

**Returns:**
* string: the resulting JSON string

### json.load (jsonString...)
Load JSON data

**Parameters:**
* jsonString...: string: JSON strings to parse

**Returns:**
* string: the resulting lua object parsed from JSON

## string
Additions and replacements to the standard string lua module

### string.capitalize (string)
Capitalize the string

**Parameters:**
* string: string: string to capitalize

**Returns:**
* string: the transformed string

### string.lower (string)
Turn a string into lowercase

**Parameters:**
* string: string: string to translate to lowercase

**Returns:**
* string: the transformed string

### string.pfind (subject, pattern, start=1)
Works almost like string.find, using PCRE2 regular expression instead of lua pattern syntax: search for the first match and return the position of where it has been found

**Parameters:**
* subject: string: subject string
* pattern: string: PCRE2 regular expression pattern. By starting the pattern with /, you can specify options with the traditional /pattern/options syntax as known in PHP PCRE or perl. Available options: i=ignore case, m=multiline, s=dot all, x=extended, u=unicode UCP extended support, U=ungreedy, A=anchored, D=dollar end only, E=end ancohred, J=allow duplicate names.
* start: integer: starting position where to start the search

**Returns:**
* integer: starting position of the first match
* integer: ending position of the first match
* string...: matched subgroups

### string.pgmatch (subject, pattern, start=1)
Works almost like string.gmatch, using PCRE2 regular expression instead of lua pattern syntax: return an interator explist to iterate over each matches of the string.

**Parameters:**
* subject: string: subject string
* pattern: string: PCRE2 regular expression pattern (see string.find)
* start: integer: starting position where to start the search

**Returns:**
* function: iterator function
* userdata: iterator state data

### string.pgsub (subject, pattern, start=1, replacement, max=0)
Works almost like string.gsub, using PCRE2 regular expression instead of lua pattern syntax: perform a global substitution, i.e. replace each match by a replacement

**Parameters:**
* subject: string: subject string
* pattern: string: PCRE2 regular expression pattern (see string.find)
* start: integer: optional: starting position where to starting search for matches
* replacement: string|function|table: replacement to apply: whether a replacement string with PCRE2 substitution syntax, a function receiving matched groups as parameters and returning the replacement string, or a table which will be looked up with the first matched group as a key. In a replacement string, Use $0 to $9 or $g{name} to refer to matched groups by index or by name.
* max: integer: maximum number of replacements to make, <=0 indicating no limit

**Returns:**
* string: new string with all replacements performed
* integer: number of replacements made 

### string.pmatch (subject, pattern, start=1)
Works almost like string.match, using PCRE2 regular expression instead of lua pattern syntax: search for the first match and return the matched groups

**Parameters:**
* subject: string: subject string
* pattern: string: PCRE2 regular expression pattern (see string.find)
* start: integer: starting position where to start the search

**Returns:**
* string...: matched subgroups

### string.upper (string)
Turn a string into uppercase

**Parameters:**
* string: string: string to translate to uppercase

**Returns:**
* string: the transformed string

## utf8
Additions to the utf8 lua module

### utf8.decode (string, encoding)
Decode a string from some encoding into UTF-8

**Parameters:**
* string: string: the string to decode
* encoding: string: the encoding in which the initial string is encoded

**Returns:**
* string: the decoded string in UTF-8

### utf8.encode (string, encoding)
Encode an UTF-8 string into some encoding

**Parameters:**
* string: string: the UTF-8 string to be encoded
* encoding: string: the encoding to encode to

**Returns:**
* string: the string encoded in the given encoding
 

End of the doc
