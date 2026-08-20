#pragma once

// Drop-in native façade over the WinForms UI -- 100% native, no /clr needed in THIS
// project. See ../UiGlue for the C++/CLI bridge this calls into, and ../Ui for the
// actual WinForms UI code. Add the UiGlue project folder to Additional Include
// Directories and link against MyPluginUiGlue.lib (see README.md).

#include "MyPluginUiGlueApi.h"

class SDRunoPlugin_MyPluginUi
{
public:
	explicit SDRunoPlugin_MyPluginUi(IUnoPluginController& controller)
		: m_handle(MyPluginUiGlue_Create(&controller))
	{
	}

	~SDRunoPlugin_MyPluginUi()
	{
		MyPluginUiGlue_Destroy(m_handle);
	}

	void Show() { MyPluginUiGlue_Show(m_handle); }
	void Close() { MyPluginUiGlue_Close(m_handle); }

	void NotifyUnoEvent(int eventType, unsigned short channel)
	{
		MyPluginUiGlue_NotifyEvent(m_handle, eventType, channel);
	}

private:
	void* m_handle;
};
