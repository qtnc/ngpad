#ifndef _____PLUGININTERFACE0_____
#define _____PLUGININTERFACE0_____
#include "AbstractDocument.hpp"
#include "../common/Properties.hpp"
#include "../common/wxUtils.hpp"
#include "wx/docview.h"

struct lua_State;

struct PluginInterface {
virtual Properties& GetConfig () = 0;
virtual Properties& GetTranslations () = 0;
virtual wxDocManager* GetDocManager () = 0;
virtual const std::string& GetLocale () = 0;
virtual const wxString& GetAppDir () = 0;
virtual const wxString& GetUserDir () = 0;
virtual const wxString& GetUserLocalDir () = 0;

virtual AbstractDocument* GetCurrentDocument () = 0;
virtual AbstractDocument* OpenOrCreateDocument (const wxString& filename) = 0;
virtual void SayText (const wxString& text) = 0;

virtual lua_State* GetLuaState () = 0;
};

#endif
