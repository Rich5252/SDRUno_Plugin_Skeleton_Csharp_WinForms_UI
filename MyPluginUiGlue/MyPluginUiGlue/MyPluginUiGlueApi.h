// MyPluginUiGlueApi.h
//
// Plain extern "C" API surface for MyPluginUiGlue.dll -- this is what the native
// SDRunoPlugin_MyPlugin project calls (via SDRunoPlugin_MyPluginUi.h's dynamic
// LoadLibrary/GetProcAddress, not a compile-time link) to drive the WinForms UI
// without itself knowing anything about C++/CLI or managed code.
//
// MyPluginUiGlue_Init() MUST be called once, right after LoadLibrary succeeds and
// BEFORE MyPluginUiGlue_Create() is ever called. It registers the AppDomain's
// AssemblyResolve handler so MyPluginUi.dll (and any other sibling managed
// assembly) can be found relative to this DLL's own folder, regardless of where
// the host process (SDRUno.exe) itself is installed. Its own body deliberately
// touches nothing from MyPluginUi.dll, so it JITs and runs using only mscorlib --
// unlike MyPluginUiGlue_Create, whose JIT compilation needs MyPluginUi.dll resolved
// before it can even start executing (because MyPluginControllerBridge implements
// IMyPluginController, defined over there). Registering the resolver from inside
// Create() itself is too late for exactly that reason -- see MyPluginUiGlueApi.cpp.

#pragma once

#include "iunoplugincontroller.h"

#ifdef MYPLUGINUIGLUE_EXPORTS
#define MYPLUGINUIGLUE_API __declspec(dllexport)
#else
#define MYPLUGINUIGLUE_API __declspec(dllimport)
#endif

extern "C"
{
	// Call first, before any other function below. Idempotent -- safe to call
	// more than once (only registers the resolver on the first call).
	MYPLUGINUIGLUE_API void __cdecl MyPluginUiGlue_Init();

	// Creates the bridge + UI host and spins up the WinForms UI thread. Returns an
	// opaque handle (really a MyPluginUiGlueHandle*) to pass to every other call
	// below, or nullptr on failure.
	MYPLUGINUIGLUE_API void* __cdecl MyPluginUiGlue_Create(IUnoPluginController* controller);

	// Shuts down the UI thread and frees the handle. Do not use the handle again
	// after this call.
	MYPLUGINUIGLUE_API void __cdecl MyPluginUiGlue_Destroy(void* handle);

	MYPLUGINUIGLUE_API void __cdecl MyPluginUiGlue_Show(void* handle);
	MYPLUGINUIGLUE_API void __cdecl MyPluginUiGlue_Close(void* handle);

	MYPLUGINUIGLUE_API void __cdecl MyPluginUiGlue_NotifyEvent(void* handle, int eventType, unsigned short channel);
}
