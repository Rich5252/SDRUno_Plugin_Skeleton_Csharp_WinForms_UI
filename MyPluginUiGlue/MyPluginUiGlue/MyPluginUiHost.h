#pragma once

#include "MyPluginControllerBridge.h"

// MyPluginUi::MainForm becomes visible with no #include needed, via the same Project
// Reference to MyPluginUi noted in MyPluginControllerBridge.h.

namespace MyPluginUiGlue
{
	// Owns a dedicated STA thread running Application::Run(MainForm) -- SDRUno's own
	// message loop is native, not WinForms, so the plugin has to bring its own.
	// All public methods are safe to call from any thread; they marshal onto the UI
	// thread via Control::Invoke/BeginInvoke as needed.
	public ref class MyPluginUiHost
	{
	public:
		explicit MyPluginUiHost(MyPluginControllerBridge^ bridge);

		void Show();
		void Close();

		// Blocks briefly until the UI thread has shut down. Call from the plugin's
		// destructor (via SDRunoPlugin_MyPluginUi::~SDRunoPlugin_MyPluginUi).
		void Shutdown();

		void NotifyUnoEvent(int eventType, unsigned short channel);

	private:
		void ThreadMain();
		void DoShowForm();
		void DoCloseForm();

		MyPluginControllerBridge^ m_bridge;
		System::Threading::Thread^ m_uiThread;
		System::Threading::ManualResetEvent^ m_formReady;
		MyPluginUi::MainForm^ m_form;
	};
}
