---
title: AI agents
---

An AI agent is a conversation with a language model which can read and modify the files of a project, called the workspace.
The model is accessed through an HTTP API compatible with the OpenAI chat completions format. It can be a local server (LM Studio, Ollama, llama.cpp, etc.) or an online service.

## Opening an agent

Press F11, or use the AI agent item of the Tools menu.

The workspace root is the workspace root of the active document, as determined by `workspace_root_indicators` in config.ini. If the active document isn't saved in a file, a dialog box asks you to choose a directory.
The workspace root of an agent can't be changed. Outside of it, the agent must ask you before accessing any file, unless the file is listed in `readablePaths` or `writablePaths` (see Paths below).

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

The agent sees paths relative to the workspace root, with `/` as separator. Files outside of the workspace are designated by absolute paths, still with `/`, for example `C:/MinGW/include/stdio.h` or `//server/share/file.txt`.

Outside of the workspace:

- Reading (`list_files`, `read_file`, `search_files`, `open_in_editor`) is allowed without confirmation in the files and directories listed in `readablePaths` or `writablePaths`. Elsewhere, you are asked first, whatever the permission of the tool.
- Modifying (`edit_file`, `write_file`, `replace_in_files`, `move_file`, `delete_file`), as well as running a command with a working directory outside of the workspace, is allowed according to the permission of the tool in the files and directories listed in `writablePaths`. Elsewhere, the confirmation is always asked, even if the tool is allowed, and it warns you that the file is outside of the workspace.
- `get_editor_context` never tells anything about documents outside of the workspace.

When `readablePaths` or `writablePaths` are given, the agent is told it can use them.
Directories ignored by `list_files` and `search_files` are given by `agent_ignored_dirs` in config.ini (see below). Binary files are ignored by `search_files` and can't be read by `read_file`.

### Open files

When a file is open in the editor, `read_file`, `search_files`, `replace_in_files` and `edit_file` work with the content of the editor, including unsaved changes, instead of the file on disk.
Modifications made in an open file aren't saved: you can review them, undo them with Ctrl+Z, and save them yourself.
When a file isn't open, it is read and written directly on disk. Its encoding and line endings are kept. New files are created according to editorconfig, or `default_charset` and `default_line_ending` of config.ini.

When a file is moved or renamed by `move_file`, the corresponding open tabs follow, including their unsaved changes.
When a file is deleted by `delete_file`, the corresponding open tabs are closed without asking to save them. The confirmation tells how many tabs will be closed, and how many of them have unsaved changes which will be lost.

### Commands

`run_command` runs a command line through the shell given by the `shell` setting of the agent, by default `cmd /s /c "{command}"`. The working directory is the workspace root, or one of its subdirectories chosen by the agent.
Note that a command can do anything your user account can do, including accessing files outside of the workspace: always read it before allowing it. Commands which don't need a confirmation can be listed in `allowedCommands` (see the Configuration section).

- The command can't read any input, and no window is shown.
- It is killed, with all the processes it started, when it runs longer than its timeout (`commandTimeout` and `commandMaxTimeout` limits), or when you cancel the work of the agent with Shift+Escape.
- The result starts with the exit code, followed by the standard output and errors. They are read separately, so their relative order may not be exactly kept; add `2>&1` to the `shell` setting if it matters. The output is decoded as UTF-8 if it is valid UTF-8, and otherwise with the OEM code page of the console. When it is too long, its beginning and its end are kept.
- A program started in the background by the command isn't waited for.

### Lua

`run_lua` runs Lua code in the editor, as in the Lua console: if the code is an expression, its value is returned; otherwise it is run as statements, which can end with `return`. The output of `print` is returned to the agent instead of being shown in the console.
The code has access to the whole scripting API, and can therefore do anything a plugin can do. Global variables it defines are kept in an environment of its own: they persist from one call to the next during the conversation, don't affect plugins, and are forgotten when the conversation is cleared.
There is currently no protection against a script which never ends.

### Confirmations

By default, each modification made by `edit_file`, `write_file`, `replace_in_files`, `move_file` or `delete_file` must be confirmed, as well as each command and each Lua script. The `tools` setting can change it (see Tools in the Configuration section). The confirmation tells which file is concerned, and if the modification will be made in the editor or on disk.
For `replace_in_files`, a single confirmation shows the list of modified files with the number of replacements in each of them, and a few examples. For `run_command` and `run_lua`, the command or the code is shown in a text field, which has the focus so that you can read it before answering.

- Allow: the modification is made, and the agent continues.
- Allow for this session: the same, and the agent won't ask again until it is closed (clearing the conversation doesn't change it). What is allowed depends on the confirmation:
  - for a modification in the workspace, every use of the same tool, for example all modifications made by `edit_file`;
  - for a command, all commands starting with the text given in the field below the command, by default the program and its subcommand, for example `git diff`. You can modify it before answering, with the same rules as `allowedCommands` (see the Configuration section): for example, a command containing `&` or `>` still always asks;
  - for a file outside of the workspace, the file or directory given in the field, by default the directory which contains the file. You can modify it before answering, with the same rules as `readablePaths` and `writablePaths`. When the agent wanted to read, reading is allowed; when it wanted to modify, reading and modifying are allowed.
- Always allow: the same as Allow for this session, and the permission is also saved in the entry of the agent in `config/agents.json`: `"tool": "allow"` in `tools`, or the command or path added to `allowedCommands`, `readablePaths` or `writablePaths`. Agents already open with the same configuration only see it once closed and opened again. Saving rewrites the file: its comments are lost, and its previous version is kept in `config/agents.json.bak`.
- Deny: the modification isn't made. The agent is informed of your refusal and continues; it should ask you what to do instead.
- Stop the agent (or Escape): the modification isn't made, and the agent stops immediately, as if the request was cancelled. Modifications allowed before are kept.

Lua scripts and deletions can only be allowed once: each of them is confirmed.
For `delete_file`, `run_command`, `run_lua`, and modifications outside of the workspace, the default button is Deny.
If the agent window is hidden when a confirmation is needed, it is shown again first.

## Configuration

Agents are configured in the file `config/agents.json`. As for other configuration files, it is searched first in the user configuration directory, and then in the directory of the application.
An example is given in `config/agents.example.json`: copy it to `config/agents.json` and adapt it.

The file contains an array of objects, one per agent. Comments are allowed, but they are lost when you choose Always allow in a confirmation, since the file is then rewritten (see Confirmations above).
Only `name` and `url` are required.

The file can also be an object with two keys: `agents`, the array of agents, and `defaults`, an object giving values common to all agents. Any key of an agent can be given in `defaults`. When an agent gives the same key, its own value is used; when both values are objects, such as `limits` or `headers`, they are merged key by key, the agent still winning.

```json
{
"defaults": { "timeout": 300, "limits": { "readMaxLines": 300 } },
"agents": [
{ "name": "Local agent", "url": "http://localhost:1234/v1/" },
{ "name": "Other agent", "url": "http://localhost:5678/v1/", "limits": { "listMaxEntries": 100 } }
]
}
```

Here, both agents have a timeout of 300 seconds and read at most 300 lines; the second one also lists at most 100 entries.

`allowedCommands`, `readablePaths` and `writablePaths` are an exception: the items given in `defaults` and those of the agent are added together.

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
tools | array or object | all tools | Tools the agent can use, and their permissions (see below)
allowedCommands | array | | Commands `run_command` can run without confirmation (see below)
readablePaths | array | | Files and directories outside of the workspace the agent can read without asking (see below)
writablePaths | array | | Files and directories outside of the workspace the agent can read and modify as in the workspace (see below)
shell | string | cmd /s /c "{command}" | Command line used by `run_command`. `{command}` is replaced by the command given by the agent; without `{command}`, the command is added at the end. It can also be used to run commands through a program filtering their output, for example to reduce the number of tokens
maxToolIterations | integer | 15 | Maximum number of successive tool call rounds for a single message. When exceeded, the agent stops with an error
limits | object | | Limits on the size of tool results (see below)
extraBody | object | | Additional parameters added as is to the body of each request, for parameters specific to a server or a model

### Tools

`tools` tells which tools the agent can use, and whether you have to confirm what they do. A permission is one of:

Permission | Meaning
-----|-----
`allow` | The tool is used without confirmation
`ask` | Each use of the tool must be confirmed (see Confirmations above)
`deny` | The tool isn't available
`default` | The built-in permission of the tool: `allow` for tools without side effects (`list_files`, `read_file`, `search_files`, `get_editor_context`, `open_in_editor`), `ask` for all the others

`tools` can be given in three ways:

- Not specified: all tools are available, with their built-in permission.
- An array of tool names: only these tools are available, with their built-in permission. For example, `"tools": ["list_files", "read_file", "search_files", "get_editor_context"]` gives a read-only agent. An empty array disables all tools: the agent can then only answer your messages.
- An object giving the permission of each tool, for example `"tools": { "*": "ask", "edit_file": "allow", "run_lua": "deny" }`. The key `"*"` stands for all tools which aren't listed. Without `"*"`, tools which aren't listed aren't available.

Two exceptions apply to `"*"`:

- It never changes the permission of tools without side effects: they stay allowed. So `"tools": { "*": "deny" }` gives a read-only agent. To make them ask or deny, list them explicitly.
- It never allows `run_command` and `run_lua` without confirmation, since commands and Lua scripts can do absolutely anything your user account can do. With `"*": "allow"`, they still ask. Only `"run_command": "allow"` or `"run_lua": "allow"`, given explicitly, removes their confirmation: do it only if you fully trust the model, or if the editor runs in an isolated environment such as a virtual machine.

Small models often work better with a few tools. Tools which aren't available are never sent to the model, and are refused if the model tries to call them anyway.

### Allowed commands

`allowedCommands` is a list of commands which `run_command` can run without asking you, for example `"allowedCommands": ["git status", "git diff", "git log", "make *"]`. Other commands still need a confirmation, unless `run_command` is explicitly set to `allow` in `tools`. If `run_command` is denied, the list has no effect.

- `*` stands for any sequence of characters, spaces included, and `?` for any single character. Case is ignored.
- A pattern only has to match the beginning of the command, up to a space: `git status` allows `git status` and `git status -s`, but not `git statusx`. `make *` allows `make all` or `make clean install`, but not `make` alone: add `make` too if needed.
- A command containing one of the characters `& | < > ^ % ; $` or a backquote is never allowed automatically, even if it matches, because these characters can chain another command, redirect the output into a file, or expand variables: `git status & del /s /q *` always asks.

Keep in mind that a command can do much more than it seems with some of its options, for example writing a file. Only list commands whose options you know, and prefer precise patterns.

### Paths outside of the workspace

`readablePaths` and `writablePaths` list files and directories outside of the workspace, for example `"readablePaths": ["C:/MinGW/include/"], "writablePaths": ["../shared/"]`. See Paths in the Tools section for what they allow. A path in `writablePaths` can also be read.

- A path without wildcard designates a file, or a directory with all its content. A final `/` is optional.
- `*` stands for any part of a file or directory name, `**` for any sequence of directories, and `?` for any single character: `C:/projects/*/include/**` designates the content of the `include` directory of every project.
- Relative paths are relative to the workspace root: `../shared/` designates the directory `shared` next to the workspace.
- `/` and `\` can both be used. Case is ignored.

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
"tools": { "*": "ask", "edit_file": "allow", "write_file": "allow" },
"allowedCommands": ["git status", "git diff", "git log"],
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
