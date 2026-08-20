// MyPluginUiGlue_SmokeTest.cpp
//
// Minimal native console EXE that does exactly what SDRUno does with the plugin DLL:
// LoadLibrary it, GetProcAddress the plain extern "C" exports, and call them. If a
// WinForms window pops up from this, the classic-/clr mixed-mode DLL genuinely works
// when loaded by code that has no idea it's hosting .NET -- which is all SDRUno is.
//
// IMPORTANT: build this as an ORDINARY native Console App project. Do NOT enable
// Common Language Runtime Support on it -- the entire point is to prove the DLL works
// from a caller that knows nothing about CLR, same as SDRUno.exe.
//
// Setup:
//  1. New Project > Console App (C++), plain native, no /clr.
//  2. Project Properties > C/C++ > General > Additional Include Directories: add your
//     SDK header folder (wherever iunoplugincontroller.h and its iuno*.h dependents
//     live -- the same path your main plugin project uses).
//  3. Copy MyPluginUiGlue.dll and MyPluginUi.dll next to this exe's own output (or
//     change the LoadLibraryW path below to point at wherever they actually built).
//  4. Run with Ctrl+F5 (not F5/Debug) so the console window stays open once it exits.

#include <Windows.h>
#include <iostream>
#include <string>

#include "iunoplugincontroller.h"

// ---------------------------------------------------------------------------------
// IUnoPluginController is pure virtual -- MyPluginUiGlue_Create needs a real object
// to call into, because MainForm_Load calls GetVfoFrequency/IsStreamingEnabled the
// moment the window is shown. This fake just returns harmless canned values so the
// UI has something to display instead of crashing on a null controller.
// ---------------------------------------------------------------------------------
class FakeUnoPluginController : public IUnoPluginController
{
public:
	void RegisterStreamObserver(channel_t, IUnoStreamObserver*) override {}
	void UnregisterStreamObserver(channel_t, IUnoStreamObserver*) override {}
	void RegisterStreamProcessor(channel_t, IUnoStreamProcessor*) override {}
	void UnregisterStreamProcessor(channel_t, IUnoStreamProcessor*) override {}
	void RegisterAudioObserver(channel_t, IUnoAudioObserver*) override {}
	void UnregisterAudioObserver(channel_t, IUnoAudioObserver*) override {}
	void RegisterAudioProcessor(channel_t, IUnoAudioProcessor*) override {}
	void UnregisterAudioProcessor(channel_t, IUnoAudioProcessor*) override {}
	void RegisterMpxObserver(channel_t, IUnoMpxObserver*) override {}
	void UnregisterMpxObserver(channel_t, IUnoMpxObserver*) override {}
	void RegisterAnnotator(IUnoAnnotator*) override {}
	void UnregisterAnnotator(IUnoAnnotator*) override {}

	DemodulatorType GetDemodulatorType(channel_t) override { return DemodulatorUSB; }
	bool SetDemodulatorType(channel_t, DemodulatorType) override { return true; }

	double GetVfoFrequency(channel_t) override { return 14074000.0; } // 14.074 MHz (FT8)
	bool SetVfoFrequency(channel_t, double) override { return true; }

	double GetCenterFrequency(channel_t) override { return 14074000.0; }
	bool SetCenterFrequency(channel_t, double) override { return true; }

	int GetFilterBandwidth(channel_t) override { return 3000; }
	bool SetFilterBandwidth(channel_t, int) override { return true; }

	bool IsStreamingEnabled(channel_t) override { return true; }

	double GetSampleRate(channel_t) override { return 2000000.0; }
	bool SetSampleRate(channel_t, double) override { return true; }
	double GetAudioSampleRate(channel_t) override { return 48000.0; }

	bool SetIFGRRelative(channel_t, int) override { return true; }

	bool SetSquelchLevel(channel_t, int) override { return true; }
	int GetSquelchLevel(channel_t) override { return 0; }
	bool SetSquelchEnable(channel_t, bool) override { return true; }
	bool GetSquelchEnable(channel_t) override { return false; }

	bool SetAgcMode(channel_t, AgcMode) override { return true; }
	AgcMode GetAgcMode(channel_t) override { return AGCModeMedium; }
	bool SetAgcThreshold(channel_t, int) override { return true; }
	int GetAgcThreshold(channel_t) override { return 0; }

	bool SetNoiseBlankerLevel(channel_t, int) override { return true; }
	int GetNoiseBlankerLevel(channel_t) override { return 0; }
	bool SetNoiseReductionLevel(channel_t, int) override { return true; }
	int GetNoiseReductionLevel(channel_t) override { return 0; }

	bool SetCwPeakFilterThreshold(channel_t, int) override { return true; }
	int GetCwPeakFilterThreshold(channel_t) override { return 0; }

	bool SetFmNoiseReductionEnable(channel_t, bool) override { return true; }
	bool GetFmNoiseReductionEnable(channel_t) override { return false; }
	bool SetFmNoiseReductionThreshold(channel_t, int) override { return true; }
	int GetFmNoiseReductionThreshold(channel_t) override { return 0; }

	bool SetWfmDeemphasisMode(channel_t, WfmDeemphasisMode) override { return true; }
	WfmDeemphasisMode GetWfmDeemphasisMode(channel_t) override { return DeemphasisNone; }

	bool SetAudioVolume(channel_t, int) override { return true; }
	int GetAudioVolume(channel_t) override { return 50; }
	bool SetAudioMute(channel_t, bool) override { return true; }
	bool GetAudioMute(channel_t) override { return false; }

	double GetSNR(channel_t) override { return 20.0; }
	double GetPower(channel_t) override { return -60.0; }

	void RequestUnload(IUnoPlugin*) override {}

	bool GetConfigurationKey(std::string, std::string& value) override { value = ""; return false; }
	bool SetConfigurationKey(std::string, std::string) override { return true; }

	int GetVRXCount() override { return 1; }
	bool GetVRXEnable(channel_t) override { return true; }
	bool SetVRXEnable(channel_t, bool) override { return true; }

	int GetStepSize(channel_t) override { return 10; }

	int GetVFOSelect(channel_t) override { return 0; }
	bool SetVFOSelect(channel_t, int) override { return true; }

	double GetSP1MinFrequency(channel_t) override { return 0.0; }
	double GetSP1MaxFrequency(channel_t) override { return 30000000.0; }

	double GetMPXLevel(channel_t) override { return 0.0; }
	bool SetMPXLevel(channel_t, double) override { return true; }

	bool GetBiasTEnable() override { return false; }
	bool SetBiasTEnable(bool) override { return true; }

	int GetSP1MinPower(channel_t) override { return -140; }
	int GetSP1MaxPower(channel_t) override { return 0; }
};

// Function pointer types matching the extern "C" exports in MyPluginUiGlueApi.h.
typedef void* (__cdecl* CreateFn)(IUnoPluginController*);
typedef void(__cdecl* DestroyFn)(void*);
typedef void(__cdecl* ShowFn)(void*);
typedef void(__cdecl* CloseFn)(void*);
typedef void(__cdecl* NotifyEventFn)(void*, int, unsigned short);

int main()
{
	HMODULE hGlue = LoadLibraryW(L"MyPluginUiGlue.dll");
	if (!hGlue)
	{
		std::cerr << "LoadLibrary failed, GetLastError=" << GetLastError() << std::endl;
		std::cerr << "(Is MyPluginUiGlue.dll -- and MyPluginUi.dll alongside it -- "
			<< "actually next to this exe?)" << std::endl;
		return 1;
	}
	std::cout << "MyPluginUiGlue.dll loaded OK." << std::endl;

	auto create = reinterpret_cast<CreateFn>(GetProcAddress(hGlue, "MyPluginUiGlue_Create"));
	auto destroy = reinterpret_cast<DestroyFn>(GetProcAddress(hGlue, "MyPluginUiGlue_Destroy"));
	auto show = reinterpret_cast<ShowFn>(GetProcAddress(hGlue, "MyPluginUiGlue_Show"));
	auto closeFn = reinterpret_cast<CloseFn>(GetProcAddress(hGlue, "MyPluginUiGlue_Close"));
	auto notify = reinterpret_cast<NotifyEventFn>(GetProcAddress(hGlue, "MyPluginUiGlue_NotifyEvent"));

	if (!create || !destroy || !show || !closeFn || !notify)
	{
		std::cerr << "GetProcAddress failed for one or more exports, GetLastError="
			<< GetLastError() << std::endl;
		FreeLibrary(hGlue);
		return 1;
	}
	std::cout << "All 5 exports resolved OK." << std::endl;

	FakeUnoPluginController fakeController;

	void* handle = create(&fakeController);
	std::cout << "Create() returned handle " << handle << std::endl;

	show(handle);
	std::cout << "Show() called -- a WinForms window should now be visible "
		<< "(with 14.074000 MHz displayed, from the fake controller)." << std::endl;

	std::cout << "Press Enter to fire a fake FrequencyChanged event..." << std::endl;
	std::cin.get();
	notify(handle, /* UnoEvent::FrequencyChanged */ 3, 0);

	std::cout << "Press Enter to close and destroy..." << std::endl;
	std::cin.get();

	closeFn(handle);
	destroy(handle);
	FreeLibrary(hGlue);

	std::cout << "Done -- no crash on teardown is itself a good sign." << std::endl;
	return 0;
}