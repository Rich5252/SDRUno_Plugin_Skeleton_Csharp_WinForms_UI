#pragma once

// Pure native header -- no C++/CLI types appear here. This is the only file the main
// plugin project needs to see; it #includes it and links against MyPluginUiGlue.lib
// without any change to its own compiler settings (no /clr needed there).

#include "iunoplugincontroller.h"

#ifdef MYPLUGINUIGLUE_EXPORTS
#define MYPLUGINUIGLUE_API __declspec(dllexport)
#else
#define MYPLUGINUIGLUE_API __declspec(dllimport)
#endif

extern "C"
{
	MYPLUGINUIGLUE_API void* __cdecl MyPluginUiGlue_Create(IUnoPluginController* controller);
	MYPLUGINUIGLUE_API void __cdecl MyPluginUiGlue_Destroy(void* handle);
	MYPLUGINUIGLUE_API void __cdecl MyPluginUiGlue_Show(void* handle);
	MYPLUGINUIGLUE_API void __cdecl MyPluginUiGlue_Close(void* handle);
	MYPLUGINUIGLUE_API void __cdecl MyPluginUiGlue_NotifyEvent(void* handle, int eventType, unsigned short channel);
}
