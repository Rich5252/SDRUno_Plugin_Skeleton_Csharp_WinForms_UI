#define MYPLUGINUIGLUE_EXPORTS
#include "MyPluginUiGlueApi.h"
#include "MyPluginControllerBridge.h"
#include "MyPluginUiHost.h"
#include <msclr/gcroot.h>

using namespace msclr;
using namespace MyPluginUiGlue;

// Plain native struct holding gcroot handles -- this is what lets a 100% native caller
// (the main plugin project, no /clr) hold a "handle" to managed objects via void*.
struct MyPluginUiGlueHandle
{
	gcroot<MyPluginControllerBridge^> bridge;
	gcroot<MyPluginUiHost^> host;
};

extern "C"
{
	void* __cdecl MyPluginUiGlue_Create(IUnoPluginController* controller)
	{
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
