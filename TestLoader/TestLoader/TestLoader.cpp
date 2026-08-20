// SDRunoPlugin_MyPlugin_SmokeTest.cpp
//
// Loads SDRunoPlugin_MyPlugin.dll -- the ACTUAL outer DLL SDRUno loads -- exactly the
// way SDRUno itself does: LoadLibrary, GetProcAddress for the 3 exports, call
// GetPluginApiLevel(), then call CreatePlugin(). Wrapped in __try/__except (SEH) so
// that if something crashes (access violation, etc.) we get an exception code printed
// to the console instead of a generic message box swallowing the real cause.
//
// This is deliberately a *second*, separate smoke test from MyPluginUiGlue_SmokeTest.cpp
// -- that one already proved MyPluginUiGlue.dll works fine when loaded directly. This one
// tests the actual chain SDRUno drives: SDRunoPlugin_MyPlugin.dll -> MyPluginUiGlue.dll
// -> MyPluginUi.dll, calling through the same CreatePlugin/DestroyPlugin/GetPluginApiLevel
// exports SDRUno calls, so if anything differs between "loads fine standalone" and
// "SDRUno says it can't load it", this should catch it.
//
// Setup:
//  1. New Project > Console App (C++), plain native, NO /clr. Build as Win32 (x86),
//     Release, to match SDRUno's own bitness and what's deployed.
//  2. Additional Include Directories: your shared SDK header folder (needs
//     iunoplugin.h, iunoplugincontroller.h, unoevent.h and its dependents).
//  3. Copy SDRunoPlugin_MyPlugin.dll, MyPluginUiGlue.dll, and MyPluginUi.dll into this
//     exe's own output folder (or just build/run directly out of a copy of your
//     CommunityPlugins folder -- either way, all three files need to be siblings of
//     wherever this exe loads SDRunoPlugin_MyPlugin.dll from).
//  4. IMPORTANT: double-check the exact signatures below against your real
//     iunoplugin.h / SDRunoPlugin_MyPluginProject.cpp -- adjust CreateFn/DestroyFn/
//     ApiLevelFn if your real declarations differ (return type of GetPluginApiLevel,
//     whether CreatePlugin takes just the controller or also a name/channel, etc.)
//  5. Run with Ctrl+F5 or F5 -- either is fine here since this harness has no CLR of
//     its own, so the loaderLockMsg MDA false-positive from the other smoke test
//     doesn't apply to it.

#include <Windows.h>
#include <iostream>
#include <string>

#include "iunoplugincontroller.h"
#include "iunoplugin.h"



// Adjust these three typedefs to match your actual exported signatures exactly.
typedef unsigned int(__cdecl* ApiLevelFn)();
typedef IUnoPlugin* (__cdecl* CreateFn)(IUnoPluginController*);
typedef void(__cdecl* DestroyFn)(IUnoPlugin*);

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

	double GetVfoFrequency(channel_t) override { return 14074000.0; }
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


int main()
{
	std::cout << "Loading SDRunoPlugin_MyPlugin.dll ..." << std::endl;

	HMODULE hPlugin = LoadLibraryW(L"SDRunoPlugin_MyPlugin.dll");
	if (!hPlugin)
	{
		DWORD err = GetLastError();
		std::cerr << "LoadLibrary FAILED, GetLastError=" << err << std::endl;
		// Common ones: 126 = ERROR_MOD_NOT_FOUND (a dependency couldn't be found),
		// 193 = ERROR_BAD_EXE_FORMAT (architecture mismatch).
		return 1;
	}
	std::cout << "LoadLibrary succeeded. Handle = " << hPlugin << std::endl;

	auto getApiLevel = reinterpret_cast<ApiLevelFn>(GetProcAddress(hPlugin, "GetPluginApiLevel"));
	auto create = reinterpret_cast<CreateFn>(GetProcAddress(hPlugin, "CreatePlugin"));
	auto destroy = reinterpret_cast<DestroyFn>(GetProcAddress(hPlugin, "DestroyPlugin"));

	std::cout << "GetProcAddress(GetPluginApiLevel) = " << (void*)getApiLevel << std::endl;
	std::cout << "GetProcAddress(CreatePlugin)       = " << (void*)create << std::endl;
	std::cout << "GetProcAddress(DestroyPlugin)      = " << (void*)destroy << std::endl;

	if (!getApiLevel || !create || !destroy)
	{
		std::cerr << "One or more exports not found by name -- this alone would make "
			<< "SDRUno report \"wrong format or missing dependencies\"." << std::endl;
		FreeLibrary(hPlugin);
		return 1;
	}

	// Call GetPluginApiLevel() inside SEH so a crash here (e.g. because it touches
	// something in MyPluginUiGlue.dll that fails to resolve) is caught and reported
	// instead of silently producing SDRUno's generic message box equivalent.
	__try
	{
		unsigned int level = getApiLevel();
		std::cout << "GetPluginApiLevel() returned 0x"
			<< std::hex << level << std::dec
			<< " -- compare this to UNOPLUGINAPIVERSION in your header (expect 0x00000002)."
			<< std::endl;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		std::cerr << "GetPluginApiLevel() THREW, SEH exception code = 0x"
			<< std::hex << GetExceptionCode() << std::dec << std::endl;
		FreeLibrary(hPlugin);
		return 1;
	}

	FakeUnoPluginController fakeController;
	IUnoPlugin* plugin = nullptr;

	std::cout << "Calling CreatePlugin(&fakeController) ..." << std::endl;
	__try
	{
		plugin = create(&fakeController);
		std::cout << "CreatePlugin() returned " << (void*)plugin << std::endl;
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		std::cerr << "CreatePlugin() THREW, SEH exception code = 0x"
			<< std::hex << GetExceptionCode() << std::dec << std::endl;
		std::cerr << "(0xC0000005 = access violation -- likely a null/bad pointer somewhere "
			<< "in the CreatePlugin->MyPluginUiGlue_Create->WinForms chain. "
			<< "0xE0434352 = a raw, unhandled .NET/CLR exception escaping into native code.)"
			<< std::endl;
		FreeLibrary(hPlugin);
		return 1;
	}

	if (plugin)
	{
		std::cout << "Success. Press Enter to DestroyPlugin() and exit..." << std::endl;
		std::cin.get();

		__try
		{
			destroy(plugin);
			std::cout << "DestroyPlugin() completed OK." << std::endl;
		}
		__except (EXCEPTION_EXECUTE_HANDLER)
		{
			std::cerr << "DestroyPlugin() THREW, SEH exception code = 0x"
				<< std::hex << GetExceptionCode() << std::dec << std::endl;
		}
	}

	FreeLibrary(hPlugin);
	std::cout << "Done." << std::endl;
	return 0;
}
