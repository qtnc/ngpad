#include "../../text/TextMarkerFinder.hpp"
#include "../../app/PluginInterface.hpp"

#define export extern "C" __declspec(dllexport)

export bool LoadPlugin (PluginInterface& plugin) {
wxMessageBox("Yes it works!", plugin.GetTranslations().get("error", "failed"), wxICON_ERROR);
return true;
}
