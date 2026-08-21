#include "pch.h"

#define MYPLUGINUIGLUE_EXPORTS
#include "MyPluginUiGlueApi.h"
#include "MyPluginControllerBridge.h"
#include "MyPluginUiHost.h"
#include <msclr/gcroot.h>

using namespace msclr;
using namespace MyPluginUiGlue;

using namespace System;
using namespace System::Reflection;
using namespace System::IO;

// ---------------------------------------------------------------------------------
// AssemblyResolve: lets the CLR find MyPluginUi.dll (and any other sibling managed
// assembly) relative to THIS DLL's own folder, instead of the default probing path
// (which anchors to the host process's own directory -- SDRUno.exe's install folder,
// not wherever MyPluginUiGlue.dll actually lives). Neither this function nor
// MyPluginUiGlue_Init() below reference anything from MyPluginUi.dll, so both JIT
// and run using only mscorlib -- that's what makes it safe to call Init() before
// Create() ever needs MyPluginUi.dll resolved. See MyPluginUiGlueApi.h for why this
// two-step split (Init, then Create) matters.
// ---------------------------------------------------------------------------------
static Assembly^ OnAssemblyResolve(Object^ sender, ResolveEventArgs^ args)
{
	String^ shortName = args->Name->Split(',')[0];
	String^ selfDir = Path::GetDirectoryName(Assembly::GetExecutingAssembly()->Location);
	String^ candidate = Path::Combine(selfDir, shortName + ".dll");

	if (File::Exists(candidate))
		return Assembly::LoadFrom(candidate);

	return nullptr; // let the CLR report its normal error for anything else
}

static bool s_resolverRegistered = false;

static void EnsureAssemblyResolveRegistered()
{
	if (!s_resolverRegistered)
	{
		AppDomain::CurrentDomain->AssemblyResolve += gcnew ResolveEventHandler(&OnAssemblyResolve);
		s_resolverRegistered = true;
	}
}

// Plain native struct holding gcroot handles -- this is what lets a 100% native caller
// (the main plugin project, no /clr) hold a "handle" to managed objects via void*.
struct MyPluginUiGlueHandle
{
	gcroot<MyPluginControllerBridge^> bridge;
	gcroot<MyPluginUiHost^> host;
};

extern "C"
{
	// Deliberately references NOTHING from MyPluginUi.dll -- only AppDomain/Assembly
	// (mscorlib). That's what lets this JIT and run successfully BEFORE
	// MyPluginUi.dll can be resolved, so the resolver is live in time for
	// MyPluginUiGlue_Create's own JIT compilation (which does need MyPluginUi.dll,
	// because MyPluginControllerBridge implements IMyPluginController from there).
	void __cdecl MyPluginUiGlue_Init()
	{
		EnsureAssemblyResolveRegistered();
	}

	void* __cdecl MyPluginUiGlue_Create(IUnoPluginController* controller)
	{
		EnsureAssemblyResolveRegistered(); // harmless no-op if Init() already ran
		MyPluginUiGlueHandle* h = new MyPluginUiGlueHandle();
		h->bridge = gcnew MyPluginControllerBridge(controller);
		h->host = gcnew MyPluginUiHost(h->bridge);
		return h;
	}

	void __cdecl MyPluginUiGlue_Destroy(void* handle)
	{
		if (!handle) return;
		MyPluginUiGlueHandle* h = static_cast<MyPluginUiGlueHandle*>(handle);
		h->host->Shutdown();
		delete h;
	}

	void __cdecl MyPluginUiGlue_Show(void* handle)
	{
		if (!handle) return;
		static_cast<MyPluginUiGlueHandle*>(handle)->host->Show();
	}

	void __cdecl MyPluginUiGlue_Close(void* handle)
	{
		if (!handle) return;
		static_cast<MyPluginUiGlueHandle*>(handle)->host->Close();
	}

	void __cdecl MyPluginUiGlue_NotifyEvent(void* handle, int eventType, unsigned short channel)
	{
		if (!handle) return;
		static_cast<MyPluginUiGlueHandle*>(handle)->host->NotifyUnoEvent(eventType, channel);
	}
}
