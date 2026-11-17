---
title: AI agents
---

An AI agent is a conversation with a language model which can read and modify the files of a project, called the workspace.
The model is accessed through an HTTP API compatible with the OpenAI chat completions format. It can be a local server (LM Studio, Ollama, llama.cpp, etc.) or an online service.

## Opening an agent

Press F11, or use the AI agent item of the Tools menu.

The workspace root is the workspace root of the active document, as determined by `workspace_root_indicators` in config.ini. If the active document isn't saved in a file, a dialog box asks you to choose a directory.
The workspace root of an agent can't be changed. The agent can't access any file outside of it.

There is a single agent per workspace root. If an agent already exists for the root, its window is shown again with its conversation.
To work on several projects at the same time, open an agent for each of them. Each one has its own window and its own conversation.

If several agents are configured, a dialog box asks you which one to use.

## Conversation window

Type your message at the end of the conversation and press Shift+Enter to send it.
While the agent is working, you can't send another message. The beginning and the end of the work are announced by your screen reader, even if the window is hidden.

Shortcut | Action
-----|-----
Shift+Enter | Send the message
Escape | Hide the window. The conversation and a pending request are kept; press F11 to come back
Shift+Escape | Cancel the pending request
Ctrl+Up / Ctrl+Down | Go to the previous / next message
Ctrl+Shift+Up / Ctrl+Shift+Down | Select up to the beginning of the previous message / the end of the current message
Ctrl+Enter | [Quick jump](quickJump.html) using the word under the cursor, for example a file name followed by a line number such as `main.cpp:12`
Ctrl+M | Choose the model
Ctrl+Shift+L | Clear the conversation
Ctrl+S | Save the conversation

The context menu also allows to close the agent. Contrary to hiding the window, closing the agent definitively deletes the conversation.

The following commands can be typed instead of a message:

Command | Effect
-----|-----
`/clear` | Clear the conversation
`/model` | List available models
`/model name` | Use another model
`/undo` | Remove the last exchange (your message and the answer) from the conversation
`/retry` | Remove the last exchange and send the same message again
`/save` | Save the conversation
`/tokens` | Show the number of tokens used since the beginning of the conversation, and the size of the context at the last request
`/help` | Show the list of commands and shortcuts

Answers are displayed once complete, as written by the model, usually in markdown. Each tool call made by the agent is shown on a single line, with its name, its parameters, and the first line of its result.

When the server reports it, the number of tokens used is shown after the answer: input tokens (and, if the server indicates it, how many of them were read from the prompt cache) and output tokens. When the agent has called tools, an answer requires several requests: the counts are then summed over all of them. Since the whole conversation is sent again with each request, input tokens grow with the length of the conversation. The `/tokens` command shows the totals since the beginning of the conversation, including requests of exchanges which failed, were cancelled or removed with `/undo`; `/clear` resets them.

## Tools

Tools are what allows the agent to interact with the workspace and the editor.

Tool | Description
-----|-----
`list_files` | List files and directories, optionally recursively and with a file name pattern such as `src/**/*.cpp`
`read_file` | Read a text file, entirely or only some lines
`search_files` | Search a text or a regular expression in all files of the workspace or of a directory, case-insensitively by default
`replace_in_files` | Replace a text or a regular expression in all files of the workspace or of a directory, case-sensitively by default. With a regular expression, `$1` to `$9` in the replacement insert captured groups
`edit_file` | Replace an exact passage of a file by a new text
`write_file` | Create a file, or replace the whole content of an existing file. Missing directories are created
`get_editor_context` | Know which file is active in the editor, the position of the cursor, the selected text, and the other open files
`open_in_editor` | Open a file in the editor, optionally moving the cursor to a line, selecting a passage, or searching a text
`move_file` | Move or rename a file or a directory
`delete_file` | Delete a file or a directory with all its content, by moving it to the trash
`run_command` | Run a command line in the workspace or one of its subdirectories (see below)
`run_lua` | Run Lua code inside the editor, with access to the whole [scripting API](scripting-reference.html) (see below)

Tools are executed in a separate thread: the editor stays usable while a command or a long search is running. Shift+Escape cancels the work of the agent; a running command is then killed.

### Paths

The agent only sees paths relative to the workspace root, with `/` as separator.
Directories ignored by `list_files` and `search_files` are given by `agent_ignored_dirs` in config.ini (see below). Binary files are ignored by `search_files` and can't be read by `read_file`.

### Open files

When a file is open in the editor, `read_file`, `search_files`, `replace_in_files` and `edit_file` work with the content of the editor, including unsaved changes, instead of the file on disk.
Modifications made in an open file aren't saved: you can review them, undo them with Ctrl+Z, and save them yourself.
When a file isn't open, it is read and written directly on disk. Its encoding and line endings are kept. New files are created according to editorconfig, or `default_charset` and `default_line_ending` of config.ini.

When a file is moved or renamed by `move_file`, the corresponding open tabs follow, including their unsaved changes.
When a file is deleted by `delete_file`, the corresponding open tabs are closed without asking to save them. The confirmation tells how many tabs will be closed, and how many of them have unsaved changes which will be lost.

### Commands

`run_command` runs a command line through the shell given by the `shell` setting of the agent, by default `cmd /s /c "{command}"`. The working directory is the workspace root, or one of its subdirectories chosen by the agent.
Note that a command can do anything your user account can do, including accessing files outside of the workspace: always read it before allowing it.

- The command can't read any input, and no window is shown.
- It is killed, with all the processes it started, when it runs longer than its timeout (`commandTimeout` and `commandMaxTimeout` limits), or when you cancel the work of the agent with Shift+Escape.
- The result starts with the exit code, followed by the standard output and errors. They are read separately, so their relative order may not be exactly kept; add `2>&1` to the `shell` setting if it matters. The output is decoded as UTF-8 if it is valid UTF-8, and otherwise with the OEM code page of the console. When it is too long, its beginning and its end are kept.
- A program started in the background by the command isn't waited for.

### Lua

`run_lua` runs Lua code in the editor, as in the Lua console: if the code is an expression, its value is returned; otherwise it is run as statements, which can end with `return`. The output of `print` is returned to the agent instead of being shown in the console.
The code has access to the whole scripting API, and can therefore do anything a plugin can do. Global variables it defines are kept in an environment of its own: they persist from one call to the next during the conversation, don't affect plugins, and are forgotten when the conversation is cleared.
There is currently no protection against a script which never ends.

### Confirmations

Each modification made by `edit_file`, `write_file`, `replace_in_files`, `move_file` or `delete_file` must be confirmed, as well as each command and each Lua script. The confirmation tells which file is concerned, and if the modification will be made in the editor or on disk.
For `replace_in_files`, a single confirmation shows the list of modified files with the number of replacements in each of them, and a few examples. For `run_command` and `run_lua`, the command or the code is shown in a text field, which has the focus so that you can read it before answering.

- Allow: the modification is made, and the agent continues.
- Deny: the modification isn't made. The agent is informed of your refusal and continues; it should ask you what to do instead.
- Stop the agent (or Escape): the modification isn't made, and the agent stops immediately, as if the request was cancelled. Modifications allowed before are kept.

For `delete_file`, `run_command` and `run_lua`, the default button is Deny.
If the agent window is hidden when a confirmation is needed, it is shown again first.

## Configuration

Agents are configured in the file `config/agents.json`. As for other configuration files, it is searched first in the user configuration directory, and then in the directory of the application.
An example is given in `config/agents.example.json`: copy it to `config/agents.json` and adapt it.

The file contains an array of objects, one per agent. Comments are allowed.
Only `name` and `url` are required.

Key | Value type | Default value | Description
-----|-----|-----|-----
name | string | | Name of the agent, shown in the window title and when choosing an agent
url | string | | Base URL of the API, for example `http://localhost:1234/v1/`. Requests are sent to `chat/completions` and `models` relatively to this URL
api | string | openai-chat | Format of the API. Only `openai-chat` is currently supported
model | string | | Model to use at the start of the conversation. It can be changed later with Ctrl+M or `/model`
headers | object | | Additional HTTP headers sent with each request, typically for authentication, for example `{"Authorization": "Bearer your-api-key"}`
systemPrompt | string | | Instructions given to the model at the beginning of the conversation
temperature | number | | Sampling temperature. When not specified, the default of the server is used
maxTokens | integer | | Maximum number of tokens of an answer. When not specified, the default of the server is used
timeout | integer | 600 | Maximum time to wait for an answer, in seconds. Local models can be very slow
tools | array or "all" | all | Tools the agent can use (see below)
shell | string | cmd /s /c "{command}" | Command line used by `run_command`. `{command}` is replaced by the command given by the agent; without `{command}`, the command is added at the end. It can also be used to run commands through a program filtering their output, for example to reduce the number of tokens
maxToolIterations | integer | 15 | Maximum number of successive tool call rounds for a single message. When exceeded, the agent stops with an error
limits | object | | Limits on the size of tool results (see below)
extraBody | object | | Additional parameters added as is to the body of each request, for parameters specific to a server or a model

### Tools

`tools` is either the string `"all"`, or an array of tool names. When it isn't specified, all tools are available.
For example, `"tools": ["list_files", "read_file", "search_files", "get_editor_context"]` gives a read-only agent which can't modify any file.
An array containing `"all"` is the same as `"all"`. An empty array disables all tools, the agent can then only answer your messages.

Small models often work better with a few tools. Tools which aren't enabled are never sent to the model, and are refused if the model tries to call them anyway.

### Limits

Key | Default value | Description
-----|-----|-----
readMaxLines | 500 | Maximum number of lines returned by `read_file`, and of selected lines returned by `get_editor_context`
listMaxEntries | 500 | Maximum number of entries returned by `list_files`
searchMaxResults | 100 | Maximum number of matches returned by `search_files`
searchMaxLineLength | 200 | Matching lines longer than this number of characters are cut in `search_files` results
maxResultChars | 50000 | Maximum size of any tool result, in bytes
commandTimeout | 60 | Time in seconds after which a command run by `run_command` is killed, when the agent doesn't give another timeout
commandMaxTimeout | 600 | Maximum timeout the agent can give to `run_command`, in seconds

When a limit is reached, the result tells the agent how to get the rest, for example by reading the next lines or by using a more specific pattern.

### Example

```json
[
{
"name": "Local agent",
"url": "http://localhost:1234/v1/",
"model": "mistralai/ministral-3-3b",
"systemPrompt": "You are a text edition and coding agent. Don't guess the content of a file before reading it. Reply concisely to the user in his language.",
"temperature": 0.2,
"tools": "all",
"limits": { "readMaxLines": 300 }
},
{
"name": "Online read-only agent",
"url": "https://api.example.com/v1/",
"model": "some-model",
"headers": { "Authorization": "Bearer your-api-key" },
"tools": ["list_files", "read_file", "search_files", "get_editor_context", "open_in_editor"]
}
]
```

### Directives in config.ini

Key | Value type | Default value | Description
-----|-----|-----|-----
agent_ignored_dirs | string | .git, .svn, .hg, node_modules, obj, objd, \_\_pycache\_\_ | Comma-separated list of directory names ignored by `list_files` and `search_files`
workspace_file_list_exclude_exts | string | | Files with these extensions are ignored by `search_files`, unless they are open or explicitly designated
