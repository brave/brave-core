/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <objbase.h>

#include <windows.h>

#include <psapi.h>
#include <shellapi.h>
#include <wrl/client.h>

#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "base/metrics/histogram_functions.h"
#include "base/process/launch.h"
#include "base/process/process_handle.h"
#include "base/synchronization/waitable_event.h"
#include "base/system/sys_info.h"
#include "base/task/thread_pool.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "base/trace_event/trace_event.h"
#include "build/branding_buildflags.h"
#include "chrome/install_static/install_util.h"
#include "chrome/installer/util/app_command.h"
#include "chrome/installer/util/per_install_values.h"
#include "chrome/installer/util/util_constants.h"
#include "third_party/abseil-cpp/absl/cleanup/cleanup.h"

#if BUILDFLAG(GOOGLE_CHROME_BRANDING) || defined(OFFICIAL_BUILD)
#include "chrome/updater/app/server/win/updater_legacy_idl.h"
#endif

namespace {

#if BUILDFLAG(GOOGLE_CHROME_BRANDING) || defined(OFFICIAL_BUILD)
// Holds the result of the IPC to CoCreate the process launcher.
struct CreateProcessLauncherResult
    : public base::RefCountedThreadSafe<CreateProcessLauncherResult> {
  Microsoft::WRL::ComPtr<IStream> stream;
  base::WaitableEvent completion_event;

 private:
  friend class base::RefCountedThreadSafe<CreateProcessLauncherResult>;
  virtual ~CreateProcessLauncherResult() = default;
};

// CoCreates the `ProcessLauncher` class, and if successful, marshals the
// resulting interface into `result->stream`. Signals `result->completion_event`
// on successful or failed completion.
void CreateAndMarshalProcessLauncher(
    scoped_refptr<CreateProcessLauncherResult> result) {
  const absl::Cleanup signal_completion_event = [&result] {
    result->completion_event.Signal();
  };

  Microsoft::WRL::ComPtr<IUnknown> unknown;
  {
    TRACE_EVENT0("startup", "InvokeGoogleUpdateForRename CoCreateInstance");
    const HRESULT hr =
        ::CoCreateInstance(__uuidof(ProcessLauncherClass), nullptr, CLSCTX_ALL,
                           IID_PPV_ARGS(&unknown));
    if (FAILED(hr)) {
      TRACE_EVENT_INSTANT1(
          "startup", "InvokeGoogleUpdateForRename CoCreateInstance failed",
          TRACE_EVENT_SCOPE_THREAD, "hr", hr);
      LOG(ERROR) << "CoCreate ProcessLauncherClass failed; hr = " << std::hex
                 << hr;
      return;
    }
  }
  const HRESULT hr = ::CoMarshalInterThreadInterfaceInStream(
      __uuidof(IUnknown), unknown.Get(), &result->stream);
  if (FAILED(hr)) {
    TRACE_EVENT_INSTANT1("startup",
                         "InvokeGoogleUpdateForRename "
                         "CoMarshalInterThreadInterfaceInStream failed",
                         TRACE_EVENT_SCOPE_THREAD, "hr", hr);
    LOG(ERROR) << "CoMarshalInterThreadInterfaceInStream "
                  "ProcessLauncherClass failed; hr = "
               << std::hex << hr;
  }
}

// CoCreates the Google Update `ProcessLauncherClass` in a `ThreadPool` thread
// with a timeout, if the `ThreadPool` is operational. The starting value for
// the timeout is 15 seconds. If the CoCreate times out, the timeout is
// increased by 15 seconds at each failed attempt and persisted for the next
// attempt.
//
// If the `ThreadPool` is not operational, the CoCreate is done
// without a timeout.
Microsoft::WRL::ComPtr<IUnknown> CreateProcessLauncher() {
  constexpr int kDefaultTimeoutIncrementSeconds = 15;
  constexpr base::TimeDelta kMaxTimeAfterSystemStartup = base::Seconds(150);

  auto result = base::MakeRefCounted<CreateProcessLauncherResult>();
  if (base::ThreadPool::CreateCOMSTATaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_BLOCKING})
          ->PostTask(FROM_HERE, base::BindOnce(&CreateAndMarshalProcessLauncher,
                                               result))) {
    installer::PerInstallValue creation_timeout(
        L"ProcessLauncherCreationTimeout");
    const base::TimeDelta timeout = base::Seconds(
        creation_timeout.Get()
            .value_or(base::Value(kDefaultTimeoutIncrementSeconds))
            .GetIfInt()
            .value_or(kDefaultTimeoutIncrementSeconds));
    const base::ElapsedTimer timer;
    const bool is_at_startup =
        base::SysInfo::Uptime() <= kMaxTimeAfterSystemStartup;
    if (!result->completion_event.TimedWait(timeout)) {
      base::UmaHistogramMediumTimes(
          is_at_startup
              ? "Startup.CreateProcessLauncher2.TimedWaitFailedAtStartup"
              : "Startup.CreateProcessLauncher2.TimedWaitFailed",
          timer.Elapsed());
      creation_timeout.Set(base::Value(static_cast<int>(timeout.InSeconds()) +
                                       kDefaultTimeoutIncrementSeconds));
      TRACE_EVENT_INSTANT0(
          "startup", "InvokeGoogleUpdateForRename CoCreateInstance timed out",
          TRACE_EVENT_SCOPE_THREAD);
      LOG(ERROR) << "CoCreate ProcessLauncherClass timed out";
      return {};
    }

    if (!result->stream) {
      return {};
    }
    base::UmaHistogramMediumTimes(
        is_at_startup
            ? "Startup.CreateProcessLauncher2.TimedWaitSucceededAtStartup"
            : "Startup.CreateProcessLauncher2.TimedWaitSucceeded",
        timer.Elapsed());

    Microsoft::WRL::ComPtr<IUnknown> unknown;
    const HRESULT hr =
        ::CoUnmarshalInterface(result->stream.Get(), __uuidof(IUnknown),
                               IID_PPV_ARGS_Helper(&unknown));
    if (FAILED(hr)) {
      TRACE_EVENT_INSTANT1(
          "startup", "InvokeGoogleUpdateForRename CoUnmarshalInterface failed",
          TRACE_EVENT_SCOPE_THREAD, "hr", hr);
      LOG(ERROR) << "CoUnmarshalInterface ProcessLauncherClass failed; hr = "
                 << std::hex << hr;
      return {};
    }

    return unknown;
  }

  // The task could not be posted to the task runner, so CoCreate without a
  // timeout. This could happen in shutdown, where the `ThreadPool` is not
  // operational.
  {
    TRACE_EVENT0("startup", "InvokeGoogleUpdateForRename CoCreateInstance");
    Microsoft::WRL::ComPtr<IUnknown> unknown;
    const HRESULT hr =
        ::CoCreateInstance(__uuidof(ProcessLauncherClass), nullptr, CLSCTX_ALL,
                           IID_PPV_ARGS(&unknown));
    if (FAILED(hr)) {
      TRACE_EVENT_INSTANT1(
          "startup", "InvokeGoogleUpdateForRename CoCreateInstance failed",
          TRACE_EVENT_SCOPE_THREAD, "hr", hr);
      LOG(ERROR) << "CoCreate ProcessLauncherClass failed; hr = " << std::hex
                 << hr;
      return {};
    }

    return unknown;
  }
}
#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING) || defined(OFFICIAL_BUILD)

bool InvokeGoogleUpdateForRenameBrave() {
#if BUILDFLAG(GOOGLE_CHROME_BRANDING) || defined(OFFICIAL_BUILD)
  // This has been identified as very slow on some startups. Detailed trace
  // events below try to shine a light on each steps. crbug.com/1252004
  TRACE_EVENT0("startup", "upgrade_util::InvokeGoogleUpdateForRename");

  Microsoft::WRL::ComPtr<IUnknown> unknown = CreateProcessLauncher();
  if (!unknown) {
    return false;
  }

  // Chrome queries for the SxS IIDs first, with a fallback to the legacy IID,
  // to make sure that marshaling loads the proxy/stub from the correct (HKLM)
  // hive.
  Microsoft::WRL::ComPtr<IProcessLauncher> ipl;
  {
    HRESULT hr = unknown.CopyTo(__uuidof(IProcessLauncherSystem),
                                IID_PPV_ARGS_Helper(&ipl));
    if (FAILED(hr)) {
      hr = unknown.As(&ipl);
    }
    if (FAILED(hr)) {
      TRACE_EVENT0("startup",
                   "InvokeGoogleUpdateForRename QueryInterface failed");
      LOG(ERROR) << "QueryInterface failed; hr = " << std::hex << hr;
      return false;
    }
  }

  ULONG_PTR process_handle = 0;
  {
    TRACE_EVENT0("startup", "InvokeGoogleUpdateForRename LaunchCmdElevated");
    HRESULT hr = ipl->LaunchCmdElevated(
        install_static::GetAppGuid(), installer::kCmdRenameChromeExe,
        ::GetCurrentProcessId(), &process_handle);
    if (FAILED(hr)) {
      TRACE_EVENT0("startup",
                   "InvokeGoogleUpdateForRename LaunchCmdElevated failed");
      LOG(ERROR) << "IProcessLauncher::LaunchCmdElevated failed; hr = "
                 << std::hex << hr;
      return false;
    }
  }

  base::Process rename_process(
      reinterpret_cast<base::ProcessHandle>(process_handle));
  int exit_code;
  {
    TRACE_EVENT0("startup", "InvokeGoogleUpdateForRename WaitForExit");
    if (!rename_process.WaitForExit(&exit_code)) {
      TRACE_EVENT0("startup", "InvokeGoogleUpdateForRename WaitForExit failed");
      PLOG(ERROR) << "WaitForExit of rename process failed";
      return false;
    }
  }

  if (exit_code != installer::RENAME_SUCCESSFUL) {
    TRACE_EVENT0("startup", "InvokeGoogleUpdateForRename !RENAME_SUCCESSFUL");
    LOG(ERROR) << "Rename process failed with exit code " << exit_code;
    return false;
  }

  TRACE_EVENT0("startup", "InvokeGoogleUpdateForRename RENAME_SUCCESSFUL");

  return true;
#else   // BUILDFLAG(GOOGLE_CHROME_BRANDING) || defined(OFFICIAL_BUILD)
  return false;
#endif  // BUILDFLAG(GOOGLE_CHROME_BRANDING) || defined(OFFICIAL_BUILD)
}

}  // namespace

#include "src/chrome/browser/first_run/upgrade_util_win.cc"  // IWYU pragma: export
