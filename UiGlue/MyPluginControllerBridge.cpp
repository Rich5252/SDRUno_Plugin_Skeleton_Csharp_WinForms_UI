#include "MyPluginControllerBridge.h"
#include <msclr/marshal_cppstd.h>

using namespace System;
using namespace msclr::interop;

namespace MyPluginUiGlue
{
	MyPluginControllerBridge::MyPluginControllerBridge(IUnoPluginController* controller)
		: m_controller(controller)
	{
	}

	double MyPluginControllerBridge::GetVfoFrequency(int channel)
	{
		return m_controller->GetVfoFrequency(static_cast<channel_t>(channel));
	}

	bool MyPluginControllerBridge::SetVfoFrequency(int channel, double frequencyHz)
	{
		return m_controller->SetVfoFrequency(static_cast<channel_t>(channel), frequencyHz);
	}

	double MyPluginControllerBridge::GetCenterFrequency(int channel)
	{
		return m_controller->GetCenterFrequency(static_cast<channel_t>(channel));
	}

	bool MyPluginControllerBridge::SetCenterFrequency(int channel, double frequencyHz)
	{
		return m_controller->SetCenterFrequency(static_cast<channel_t>(channel), frequencyHz);
	}

	int MyPluginControllerBridge::GetFilterBandwidth(int channel)
	{
		return m_controller->GetFilterBandwidth(static_cast<channel_t>(channel));
	}

	bool MyPluginControllerBridge::SetFilterBandwidth(int channel, int bandwidthHz)
	{
		return m_controller->SetFilterBandwidth(static_cast<channel_t>(channel), bandwidthHz);
	}

	bool MyPluginControllerBridge::IsStreamingEnabled(int channel)
	{
		return m_controller->IsStreamingEnabled(static_cast<channel_t>(channel));
	}

	bool MyPluginControllerBridge::SetAudioVolume(int channel, int volume)
	{
		return m_controller->SetAudioVolume(static_cast<channel_t>(channel), volume);
	}

	int MyPluginControllerBridge::GetAudioVolume(int channel)
	{
		return m_controller->GetAudioVolume(static_cast<channel_t>(channel));
	}

	bool MyPluginControllerBridge::SetAudioMute(int channel, bool mute)
	{
		return m_controller->SetAudioMute(static_cast<channel_t>(channel), mute);
	}

	bool MyPluginControllerBridge::GetAudioMute(int channel)
	{
		return m_controller->GetAudioMute(static_cast<channel_t>(channel));
	}

	double MyPluginControllerBridge::GetSNR(int channel)
	{
		return m_controller->GetSNR(static_cast<channel_t>(channel));
	}

	double MyPluginControllerBridge::GetPower(int channel)
	{
		return m_controller->GetPower(static_cast<channel_t>(channel));
	}

	String^ MyPluginControllerBridge::GetConfigurationKey(String^ key)
	{
		std::string nativeKey = marshal_as<std::string>(key);
		std::string value;
		if (m_controller->GetConfigurationKey(nativeKey, value))
		{
			return marshal_as<String^>(value);
		}
		return String::Empty;
	}

	bool MyPluginControllerBridge::SetConfigurationKey(String^ key, String^ value)
	{
		return m_controller->SetConfigurationKey(marshal_as<std::string>(key), marshal_as<std::string>(value));
	}
}
