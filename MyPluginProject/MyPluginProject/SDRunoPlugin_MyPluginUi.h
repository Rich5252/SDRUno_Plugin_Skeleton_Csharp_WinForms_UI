// SDRunoPlugin_MyPluginUi.h
//
// Native facade over the WinForms UI. Loads MyPluginUiGlue.dll DYNAMICALLY
// (LoadLibrary + GetProcAddress) rather than linking against MyPluginUiGlue.lib at
// build time -- that's the fix for SDRUno's original "wrong format or missing
// dependencies" error: the OS's classic DLL search order resolves an implicitly-
// linked dependency relative to the *main EXE's* directory (SDRUno's install
// folder), not relative to whatever folder SDRunoPlugin_MyPlugin.dll itself was
// loaded from. This finds its own folder at runtime instead (GetModuleHandleEx +
// GetModuleFileName on itself) and loads MyPluginUiGlue.dll from that exact path.
//
// IMPORTANT project-settings change that goes with this file:
//   In SDRunoPlugin_MyPlugin's project properties, Linker > Input > Additional
//   Dependencies, MyPluginUiGlue.lib must NOT be listed. Also remove MyPluginUiGlue's
//   output folder from Linker > General > Additional Library Directories if nothing
//   else there needs it. Leaving the .lib linked recreates the exact implicit
//   load-time dependency this file works around.
//   (Keeping MyPluginUiGlue checked under Project Dependencies for build ORDER is
//   still fine and worth keeping -- that only affects build order, not linking.)
//
// MyPluginUiGlue_Init() is called first, before MyPluginUiGlue_Create(), to work
// around a second, separate problem: registering the managed AssemblyResolve
// handler (which lets MyPluginUiGlue.dll find MyPluginUi.dll next to itself,
// rather than next to SDRUno.exe) can't be done from inside MyPluginUiGlue_Create
// itself -- JIT-compiling that function requires resolving MyPluginUi.dll before
// the function's own first statement ever runs, since MyPluginControllerBridge
// implements IMyPluginController (defined in MyPluginUi.dll). MyPluginUiGlue_Init
// references nothing from MyPluginUi.dll, so it JITs and runs first, registering
// the resolver in time for Create()'s JIT compilation to succeed.

#pragma once

#include <Windows.h>
#include <string>

#include "iunoplugincontroller.h"

// Function pointer types matching the extern "C" exports in MyPluginUiGlueApi.h.
typedef void  (__cdecl* MyPluginUiGlue_InitFn)();
typedef void* (__cdecl* MyPluginUiGlue_CreateFn)(IUnoPluginController*);
typedef void  (__cdecl* MyPluginUiGlue_DestroyFn)(void*);
typedef void  (__cdecl* MyPluginUiGlue_ShowFn)(void*);
typedef void  (__cdecl* MyPluginUiGlue_CloseFn)(void*);
typedef void  (__cdecl* MyPluginUiGlue_NotifyEventFn)(void*, int, unsigned short);

class SDRunoPlugin_MyPluginUi
{
public:
	SDRunoPlugin_MyPluginUi() = default;

	explicit SDRunoPlugin_MyPluginUi(IUnoPluginController* controller)
	{
		Create(controller);
	}

	explicit SDRunoPlugin_MyPluginUi(IUnoPluginController& controller)
	{
		Create(&controller);
	}

	~SDRunoPlugin_MyPluginUi()
	{
		Destroy();
		if (m_hGlue)
		{
			FreeLibrary(m_hGlue);
			m_hGlue = nullptr;
		}
	}

	// Call once, before Show(). Returns false if the glue DLL (or any of its
	// required exports) couldn't be found/loaded -- treat that as "UI unavailable"
	// rather than letting it take down the whole plugin/SDRUno.
	bool Create(IUnoPluginController* controller)
	{
		if (!LoadGlueLibrary())
			return false;

		m_init(); // registers AssemblyResolve BEFORE m_create is ever JITted

		m_handle = m_create(controller);
		return m_handle != nullptr;
	}

	void Show()
	{
		if (m_handle && m_show) m_show(m_handle);
	}

	void Close()
	{
		if (m_handle && m_close) m_close(m_handle);
	}

	void NotifyUnoEvent(int eventType, unsigned short channel)
	{
		if (m_handle && m_notify) m_notify(m_handle, eventType, channel);
	}

	void Destroy()
	{
		if (m_handle && m_destroy)
		{
			m_destroy(m_handle);
			m_handle = nullptr;
		}
	}

	bool IsAvailable() const { return m_handle != nullptr; }

private:
	bool LoadGlueLibrary()
	{
		if (m_hGlue) return true; // already loaded

		std::wstring dllPath = GetOwnModuleFolder() + L"\\MyPluginUiGlue.dll";

		m_hGlue = LoadLibraryW(dllPath.c_str());
		if (!m_hGlue)
			return false; // e.g. MyPluginUiGlue.dll/MyPluginUi.dll missing from the same folder

		m_init    = reinterpret_cast<MyPluginUiGlue_InitFn>(GetProcAddress(m_hGlue, "MyPluginUiGlue_Init"));
		m_create  = reinterpret_cast<MyPluginUiGlue_CreateFn>(GetProcAddress(m_hGlue, "MyPluginUiGlue_Create"));
		m_destroy = reinterpret_cast<MyPluginUiGlue_DestroyFn>(GetProcAddress(m_hGlue, "MyPluginUiGlue_Destroy"));
		m_show    = reinterpret_cast<MyPluginUiGlue_ShowFn>(GetProcAddress(m_hGlue, "MyPluginUiGlue_Show"));
		m_close   = reinterpret_cast<MyPluginUiGlue_CloseFn>(GetProcAddress(m_hGlue, "MyPluginUiGlue_Close"));
		m_notify  = reinterpret_cast<MyPluginUiGlue_NotifyEventFn>(GetProcAddress(m_hGlue, "MyPluginUiGlue_NotifyEvent"));

		if (!m_init || !m_create || !m_destroy || !m_show || !m_close || !m_notify)
		{
			FreeLibrary(m_hGlue);
			m_hGlue = nullptr;
			return false;
		}

		return true;
	}

	// Folder containing THIS module (SDRunoPlugin_MyPlugin.dll) -- not the current
	// directory, not SDRUno.exe's directory. This is the actual fix: it anchors the
	// lookup to wherever the plugin itself was loaded from.
	static std::wstring GetOwnModuleFolder()
	{
		HMODULE hSelf = nullptr;
		GetModuleHandleExW(
			GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
			reinterpret_cast<LPCWSTR>(&SDRunoPlugin_MyPluginUi::GetOwnModuleFolder),
			&hSelf);

		wchar_t path[MAX_PATH] = {};
		GetModuleFileNameW(hSelf, path, MAX_PATH);

		std::wstring full(path);
		size_t pos = full.find_last_of(L"\\/");
		return (pos == std::wstring::npos) ? L"." : full.substr(0, pos);
	}

	HMODULE m_hGlue  = nullptr;
	void*   m_handle = nullptr;

	MyPluginUiGlue_InitFn         m_init    = nullptr;
	MyPluginUiGlue_CreateFn       m_create  = nullptr;
	MyPluginUiGlue_DestroyFn      m_destroy = nullptr;
	MyPluginUiGlue_ShowFn         m_show    = nullptr;
	MyPluginUiGlue_CloseFn        m_close   = nullptr;
	MyPluginUiGlue_NotifyEventFn  m_notify  = nullptr;
};
