#include "MyPluginUiHost.h"

using namespace System;
using namespace System::Threading;
using namespace System::Windows::Forms;

namespace MyPluginUiGlue
{
	MyPluginUiHost::MyPluginUiHost(MyPluginControllerBridge^ bridge)
		: m_bridge(bridge)
	{
		m_formReady = gcnew ManualResetEvent(false);

		m_uiThread = gcnew Thread(gcnew ThreadStart(this, &MyPluginUiHost::ThreadMain));
		m_uiThread->SetApartmentState(ApartmentState::STA);
		m_uiThread->IsBackground = true;
		m_uiThread->Name = "MyPluginUi";
		m_uiThread->Start();

		// Wait for MainForm to exist before returning, so an immediate Show()/
		// NotifyUnoEvent() call from the constructor's caller can't race construction.
		m_formReady->WaitOne();
	}

	void MyPluginUiHost::ThreadMain()
	{
		Application::EnableVisualStyles();
		Application::SetCompatibleTextRenderingDefault(false);

		m_form = gcnew MyPluginUi::MainForm(m_bridge);
		// Force handle creation now so InvokeRequired/Invoke work correctly for any
		// caller that races the initial Show().
		auto forceHandle = m_form->Handle;

		m_formReady->Set();

		Application::Run(m_form);
	}

	void MyPluginUiHost::Show()
	{
		if (m_form == nullptr || m_form->IsDisposed) return;

		if (m_form->InvokeRequired)
			m_form->BeginInvoke(gcnew MethodInvoker(m_form, &Form::Show));
		else
			m_form->Show();
	}

	void MyPluginUiHost::Close()
	{
		if (m_form == nullptr || m_form->IsDisposed) return;

		if (m_form->InvokeRequired)
			m_form->BeginInvoke(gcnew MethodInvoker(m_form, &Form::Close));
		else
			m_form->Close();
	}

	void MyPluginUiHost::Shutdown()
	{
		if (m_form != nullptr && !m_form->IsDisposed)
		{
			m_form->BeginInvoke(gcnew MethodInvoker(m_form, &Form::Close));
		}

		if (m_uiThread != nullptr && m_uiThread->IsAlive)
		{
			m_uiThread->Join(2000);
		}
	}

	void MyPluginUiHost::NotifyUnoEvent(int eventType, unsigned short channel)
	{
		if (m_form == nullptr || m_form->IsDisposed) return;

		array<Object^>^ args = gcnew array<Object^>{ eventType, static_cast<int>(channel) };

		if (m_form->InvokeRequired)
			m_form->BeginInvoke(gcnew Action<int, int>(m_form, &MyPluginUi::MainForm::OnUnoEvent), args);
		else
			m_form->OnUnoEvent(eventType, channel);
	}
}
