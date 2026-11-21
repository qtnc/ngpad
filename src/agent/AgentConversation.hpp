#ifndef _____AGENT_CONVERSATION_HPP
#define _____AGENT_CONVERSATION_HPP
#include "../common/wxUtils.hpp"
#include "nlohmann/json.hpp"

// Extension of saved conversations; the only place where it is defined
#define AGENT_CONVERSATION_EXT ".chat.json"
#define AGENT_CONVERSATION_FORMAT "ngpad-agent-conversation"
#define AGENT_CONVERSATION_VERSION 1
#define AGENT_CONVERSATIONS_DEFAULT_DIR "conversations"

struct AgentConfig;

// Filter for file dialogs, built from AGENT_CONVERSATION_EXT
wxString GetConversationFileFilter ();
// Add the extension if the path doesn't already end with it
wxString AddConversationExtension (const wxString& path);
bool HasConversationExtension (const wxString& path);

// Directory of automatically saved conversations: conversationsDir of the agent, relative to the workspace root,
// or by default the conversations directory next to the config directory containing agents.json
wxString GetConversationsDir (const AgentConfig& config, const wxString& root);
// New file name for an automatically saved conversation: date, time and workspace name
wxString MakeAutoConversationPath (const wxString& dir, const wxString& root);

// Read and check a conversation file (format and version)
bool ReadConversationFile (const wxString& path, nlohmann::json& data, wxString& error);
// Write through a temporary file, so that a crash never leaves a truncated file. Missing directories are created.
bool WriteConversationFile (const wxString& path, const nlohmann::ordered_json& data, wxString& error);

#endif
