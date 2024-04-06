#ifndef _____CONSTANTS_HPP
#define _____CONSTANTS_HPP

#define APP_NAME "NGPad"
#define APP_VENDOR "QuentinC"
#define APP_DISPLAY_NAME APP_NAME
#define APP_WEBSITE_URL "http://quentinc.net/"
#define APP_COPYRIGHT_INFO "Copyright © 2023"
#define APP_VERSION_STRING "2024.4.6"

#define CONFIG_FILENAME "config/config.ini"
#define SESSION_FILENAME "config/session.ini"
#define FIND_REPLACE_HISTORY_FILENAME "config/find-replace-history.ini"
#define EDITORCONFIG_ROOT_FILENAME "config/editorconfig.ini"
#define EDITORCONFIG_FILENAME ".editorconfig"
#define LUA_INIT_SCRIPT_FILENAME "init.lua"
#define DEFAULT_NEW_DOC_BASE_FILENAME "file.ext"

#define MODE_SDI 0
#define MODE_MDI 1
#define MODE_NB 2
#define MODE_AUINB 3

#define SESSION_RELOAD_NEVER 0
#define SESSION_RELOAD_WHEN_EMPTY 1
#define SESSION_RELOAD_ALWAYS 2

#define LE_CRLF 0
#define LE_LF 1
#define LE_CR 2

#define IDM_FIRST 1000
#define IDM_FINDNEXT 1200
#define IDM_FINDPREV 1201
#define IDM_READONLY 1202
#define IDM_LINEWRAP 1203
#define IDM_LINE_ENDING 1220
#define IDM_LINE_ENDING_OTHER 1229
#define IDM_INDENT 1230
#define IDM_ENCODING 1242
#define IDM_ENCODING_OTHER 1399
#define IDM_MULTIFIND 1400
#define IDM_MULTIREPLACE 1401
#define IDM_EXEC_COMMAND 1402
#define IDM_LUA_CONSOLE 1403
#define IDM_CLEAR_CONSOLE 1404
#define IDM_FILE_TREE 1405
#define IDM_SWITCH_PANE 1410
#define IDM_SHOW_PANE 1411

#define IDM_TEST 9998
#define IDM_LAST 9999

#endif
