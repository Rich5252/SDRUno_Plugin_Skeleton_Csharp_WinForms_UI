# MyPlugin — SDRUno Plugin with WinForms UI

A from-scratch SDRUno plugin using a native plugin DLL, a C++/CLI bridge, and a
WinForms UI, built as a reusable template. This README documents the architecture
and the (many) real, non-obvious problems that were solved to get from "builds
cleanly" to "actually runs reliably inside SDRUno" — worth reading before you touch
the DLL-loading, threading, or controller-call code, since most of it exists to work
around specific failures, not for style.

## Architecture

```
SDRunoPlugin_MyPlugin.dll   (native, x86/Win32 -- the DLL SDRUno actually loads)
        |  dynamically LoadLibrary's MyPluginUiGlue.dll from its OWN folder at
        |  runtime (NOT linked at build time -- see "DLL search path" below)
        v
MyPluginUiGlue.dll           (C++/CLI, classic /clr, .NET Framework 4.8) --
        |  owns the STA UI thread, wraps IUnoPluginController for the UI,
        |  references MyPluginUi.dll                        applies mutex/reentrancy
        v                                                     protection around calls
MyPluginUi.dll                 (pure C#, WinForms, .NET Framework 4.8) --
                                  your actual UI/logic
```

Reference direction is one-way (`MyPluginUiGlue -> MyPluginUi`), so there's no
circular project reference: `MyPluginUi` defines `IMyPluginController` (an
interface) and `MainForm`; `MyPluginUiGlue`'s `MyPluginControllerBridge` implements
that interface and forwards calls to the real `IUnoPluginController` SDRUno hands
the plugin at creation time.

Why three DLLs instead of one: SDRUno's plugin ABI (`IUnoPlugin`/
`IUnoPluginController`) is a native, vtable-based C++ interface — the DLL SDRUno
loads has to be native or mixed-mode, it cannot be pure C#. Splitting native plugin
logic, the C++/CLI bridge, and the WinForms UI into three DLLs keeps the UI in
ordinary, easy-to-write C# while keeping the SDRUno-facing surface fully native.

**Both projects targeting managed code build for .NET Framework 4.8 with classic
`/clr`** (not `/clr:netcore`). This was tried and reverted — see "Toolchain notes"
below.

**Everything builds Win32 (x86), not x64.** SDRUno itself is a 32-bit process; a
64-bit plugin DLL fails to load at all.

## Key issues resolved during development

Roughly in the order they were found and fixed. Each one produced a different,
often misleading symptom, so the "what it looked like" column matters if you're
trying to recognise a regression later.

### 1. DLL search path ("The plugin cannot be loaded... wrong dll format or missing dependencies")

**Symptom:** SDRUno's own dialog, immediately on Load Plugins, even though `dumpbin`
showed clean dependencies and exports matching a known-working plugin exactly.

**Root cause:** `SDRunoPlugin_MyPlugin.dll` was *implicitly linked* against
`MyPluginUiGlue.lib`. Windows' classic DLL search order resolves an implicitly
linked dependency relative to the **main EXE's own directory** (SDRUno's install
folder) and the current directory — **not** relative to whichever folder the DLL
declaring the dependency was itself loaded from. Since `MyPluginUiGlue.dll` lives
in the plugin's own folder (e.g. `CommunityPlugins\`), not next to `SDRUno.exe`, it
could never be found via an implicit link, no matter how correct the deployment
folder itself was.

**Fix:** `SDRunoPlugin_MyPluginUi.h` no longer links `MyPluginUiGlue.lib` at all.
Instead it finds its own module's folder at runtime
(`GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, ...)` +
`GetModuleFileName`) and calls `LoadLibraryW` + `GetProcAddress` on that exact path.
This makes the plugin self-contained regardless of where SDRUno itself is
installed. **`MyPluginUiGlue.lib` must not appear in `SDRunoPlugin_MyPlugin`'s
Linker > Input > Additional Dependencies** — if it's relinked in, this bug comes
straight back.

### 2. CLR assembly resolution (`FileNotFoundException`, `HRESULT 0x80070002`)

**Symptom:** Past the DLL-loading error, SDRUno crashed outright (Windows Error
Reporting showed `clr.dll` throwing `0xE0434352` wrapping `0x80070002`, i.e.
file-not-found).

**Root cause:** Same problem as #1, one layer up. When the CLR boots because a
native host (`SDRUno.exe`) called `LoadLibrary` on a mixed-mode DLL, it creates a
default AppDomain whose `ApplicationBase` is **SDRUno.exe's own folder**, not
wherever `MyPluginUiGlue.dll` was actually loaded from. So the first time managed
code touched a `MyPluginUi` type, the CLR's default assembly probing looked for
`MyPluginUi.dll` next to `SDRuno.exe`, didn't find it, and threw.

**Fix:** An `AppDomain::AssemblyResolve` handler in `MyPluginUiGlueApi.cpp` that
finds sibling assemblies relative to `Assembly::GetExecutingAssembly()->Location`
(i.e. `MyPluginUiGlue.dll`'s own folder) instead of relying on default probing.

### 3. Registering the resolver too late (chicken-and-egg with JIT compilation)

**Symptom:** Same `FileNotFoundException` crash *persisted* even after adding the
resolver above, because it was originally registered as the first statement inside
`MyPluginUiGlue_Create`.

**Root cause:** .NET JITs an entire method body in one pass the first time it's
called, *before* any of that method's own statements execute.
`MyPluginUiGlue_Create`'s body constructs `MyPluginControllerBridge`, which
implements `IMyPluginController` (defined in `MyPluginUi.dll`) — so the JIT has to
resolve `MyPluginUi.dll` just to compile `MyPluginUiGlue_Create` into native code,
*before* that function's first line (which registered the resolver) ever runs.

**Fix:** A separate exported function, `MyPluginUiGlue_Init()`, whose body
references nothing from `MyPluginUi.dll` (only `AppDomain`/`Assembly`, i.e.
mscorlib). It JITs and runs cleanly with no dependency to resolve. The native
facade (`SDRunoPlugin_MyPluginUi.h`) calls `Init()` immediately after
`LoadLibraryW` succeeds, *before* ever calling `Create()` — so the resolver is
already registered by the time `Create()`'s own JIT compilation needs it.

### 4. `loaderLockMsg` MDA — real signal vs. false positive

You'll see `<mda:loaderLockMsg break="true"/>` under the VS debugger at various
points. Two different situations produce it, and they matter differently:

- **Genuinely harmless (debugger-only):** simply loading a classic `/clr`
  mixed-mode DLL via `LoadLibrary` from a native host and calling into it shortly
  after can trip this diagnostic under a debugger even in the fully-supported,
  working case. If continuing (F5) repeatedly leads to a clean, successful run
  (UI appears, no further break, no WER crash), it was noise.
- **A real bug:** if it's accompanied by an actual crash once you're *not* running
  under a debugger (Windows Error Reporting shows a real, unhandled exception),
  something genuinely is executing managed code too early — check for any managed
  global/static variable initialized at file scope (outside a function) anywhere
  in the C++/CLI project, or a non-trivial `DllMain` not wrapped in
  `#pragma managed(push, off)`. Neither was the actual cause here (see #2/#3
  instead), but it's the first thing to rule out if you see this again.

### 5. Cross-thread contention (`SEHException`, wraps `std::system_error(EBUSY)`)

**Symptom:** UI loads and displays fine, but calling `GetVfoFrequency`/
`SetVfoFrequency` from an event-notification path sometimes threw
`System.Runtime.InteropServices.SEHException (0x80004005): External component has
thrown an exception`.

**Root cause:** SDRUno's real `IUnoPluginController` implementation isn't
thread-safe against arbitrary concurrent calls — when contended, it throws a
native `std::system_error` with code `EBUSY` (16) rather than blocking. Nothing in
`MyPluginControllerBridge.cpp` was catching this, so it propagated unhandled across
the native/managed boundary and the CLR reported it as a generic `SEHException`.

**Fix:** Ported the same pattern already proven in the sibling TX Link project
(`SDRUnoHelper.cpp`) — every frequency get/set call goes through a `std::mutex` and
retries a few times (5 attempts, 5ms apart) if it catches `std::system_error` with
`EBUSY`, only rethrowing once retries are exhausted or the error is something else.
See `MyPluginControllerBridge.cpp`.

Note: C++/CLI does not allow local lambdas inside a managed member function
(`C3923`), so the retry logic is implemented as plain named native free functions
(`GetVfoFrequencySafe`, `SetVfoFrequencySafe`, etc.), wrapped in
`#pragma managed(push, off)` / `pop`, rather than as an inline generic
lambda-based helper.

### 6. Same-thread reentrancy (mutex alone wasn't enough)

**Symptom:** External, SDRUno-UI-driven frequency changes worked fine after #5, but
clicking the plugin's own **Set** button still crashed with the identical
`SEHException` signature.

**Root cause:** SDRUno's `SetVfoFrequency` call appears to pump Windows messages
internally while it's executing. That means the `FrequencyChanged` event it fires
can get processed **on the same thread, nested inside the still-running
`SetVfoFrequency` call** — not just from a genuinely different thread the way #5
assumed. The reentrant call then tries to lock the *same* `g_controllerMutex` the
outer call already holds, **on the same thread**. Re-locking a non-recursive
`std::mutex` from the thread that already owns it is undefined behaviour — in
practice a hard deadlock that no retry loop can ever resolve, since the outer call
can't finish and release the lock until the nested call returns, and the nested
call is stuck waiting on that very lock.

**Fix:** A `thread_local bool t_insideSetCall` flag, set for the duration of the
native `Set*` call. If a `Get*` call arrives on the *same thread* while that flag
is set, it skips the native call entirely and returns a cached last-known value
instead of attempting the unsafe nested call. Being `thread_local` (not a plain
static) means a genuinely different thread calling `Get*` concurrently is
unaffected and still goes through the normal mutex+retry path.

### 7. Stale display after a reentrant Set

Because #6 returns a *cached* value during the brief reentrant window, the display
can show a slightly stale number for that one instant during a Set operation.
Fixed with one extra line in `MainForm.cs`'s `btnSetFrequency_Click`: call
`RefreshFrequencyDisplay()` again immediately after `_controller.SetVfoFrequency(...)`
returns. That call is synchronous from the C# side — it doesn't return until the
*entire* native call chain (including any nested reentrant event processing) has
completed, so by the time it returns, `t_insideSetCall` is back to `false` and the
follow-up refresh reads the true, settled value.

### 8. DLL loading is one-way per SDRUno session (by design, not a bug)

Load/unload/reload of the plugin works fine repeatedly *within* a running SDRUno
session, but once `MyPluginUiGlue.dll`/`MyPluginUi.dll` have been loaded once, the
files stay locked on disk (can't be overwritten by a rebuild) even after a clean
unload. This is a fundamental limitation of the classic .NET Framework CLR: once it
boots inside a process (triggered by the first `/clr` mixed-mode DLL load), it
stays resident for the life of the *process*, and does not support unloading
assemblies from the default AppDomain. `FreeLibrary` correctly decrements the
native module refcount, but the CLR's own internal state keeps the underlying
managed image pinned regardless.

**Practical implication:** fully close and restart `SDRUno.exe` before redeploying
an updated `MyPluginUiGlue.dll` or `MyPluginUi.dll`. `SDRunoPlugin_MyPlugin.dll`
itself, being pure native with no CLR involvement, doesn't have this restriction.

## Toolchain notes

- **Classic `/clr`, not `/clr:netcore`.** `/clr:netcore` (targeting modern .NET,
  e.g. net10.0-windows) was tried first and hit a long sequence of undocumented
  issues (`C2039`/`C1107`/`C3624` around `System.Windows.Forms` and
  `System.Windows.Forms.Primitives` resolution, missing `#using` directives the
  Reference Manager UI can't add in netcore mode). Classic `/clr` targeting .NET
  Framework 4.8 is decades-proven for exactly this native-host-loads-mixed-mode-DLL
  scenario and avoided all of it.
- **`TargetFrameworkVersion` needs the `v` prefix** (`v4.8`, not `4.8`) in
  hand-edited `.vcxproj` files — a missing `v` silently breaks `mscorlib`
  resolution with a confusing `C1107` error.
- **`CLRSupport=true` must be set for every Configuration|Platform** in the
  `.vcxproj`, not just the one you happen to be actively building.
- **Win32's default output directory omits the `$(Platform)\` segment**
  (`$(SolutionDir)$(Configuration)\`), unlike x64/other platforms
  (`$(SolutionDir)$(Platform)\$(Configuration)\`) — a long-standing MSVC quirk,
  worth an explicit Output Directory override if it trips you up again.
- Classic `/clr` **forbids** static CRT linking (`/MT`) — only a fully native
  project (no `/clr`) can use it.

## Known gaps / things to remember for next time

- **Only VFO frequency and center frequency are currently protected** by the
  mutex+retry+reentrancy-guard pattern in `MyPluginControllerBridge.cpp`.
  `GetFilterBandwidth`/`SetFilterBandwidth`, audio volume/mute, SNR, and power all
  still call straight into `m_controller` with no protection. TX Link doesn't
  exercise these either, so there's no existing reference implementation for them
  — if any of these get wired into event-reactive UI code later, apply the same
  pattern (see `GetVfoFrequencySafe`/`SetVfoFrequencySafe` as the template).
- Deployment: `SDRunoPlugin_MyPlugin.dll`, `MyPluginUiGlue.dll`, and
  `MyPluginUi.dll` must all sit in the same folder (SDRUno loads only the first by
  path, but it needs the other two as siblings — see #1/#2 above for why that
  folder doesn't need to be SDRUno's own).

## When you're ready for serial/pipe/UDP

`SDRunoPlugin_MyPlugin.cpp` has a single placeholder background-worker thread
(`StartWorker`/`StopWorker`/`WorkerLoop`), created but not started
(`StartWorker()` is commented out in the constructor). Fill in `WorkerLoop()`,
uncomment the `StartWorker()` call, and decide how results reach the UI: either add
methods to `IMyPluginController`/`MyPluginControllerBridge` (if the UI pulls data
on demand — apply the same mutex/reentrancy pattern if the worker thread and UI
events could ever contend), or add a push-style notification alongside
`NotifyUnoEvent` (if the worker thread needs to shove data at the UI as it
arrives — same `BeginInvoke`-marshaled pattern `MyPluginUiHost::NotifyUnoEvent`
already uses).
