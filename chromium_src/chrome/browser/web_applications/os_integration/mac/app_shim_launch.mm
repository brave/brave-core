/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// The code below makes it so that, in tests, shims are spawned as direct child
// processes (fork/exec) instead of being opened through LaunchServices. The
// motivation for this is to work around the following problem:
//
// When the tests are run from a volume that macOS classifies as "removable",
// then macOS sometimes shows a GUI prompt "<shim name> would like to access
// files on a removable volume". In unattended runs (the typical case in CI),
// the prompt never gets answered. The tests (e.g. WebAppIntegration*) leak the
// running shim processes, so their requests stay pending. One unanswered prompt
// blocks at the head of `sandboxd`'s queue, and every subsequent check parks
// its own worker thread behind it, until the thread pool is exhausted and
// sandboxd deadlocks machine-wide.
//
// TCC attributes each file access to a process's "responsible process", and a
// fork/exec child inherits its parent's. The test process can read the volume
// it runs from, so a shim spawned as its child inherits that access. However,
// upstream launches the shim through LaunchServices, which makes the shim its
// own responsible process instead of inheriting the launcher's. It is therefore
// evaluated under its own identity, which lacks the grant, and hits the
// kTCCServiceSystemPolicyRemovableVolumes check described above.
//
// This is made worse by each test shim getting a brand-new bundle identity, so
// TCC's per-identity decision cache never hits: every shim is a fresh
// evaluation, i.e. another independent chance of drawing the prompt instead of
// a silent denial.
//
// Launching the shim as a direct child process in tests solves the problem:
// the shim inherits the test process's responsible process, and thus its
// access to the volume, so the check is satisfied without any prompt.

#include "chrome/browser/web_applications/os_integration/mac/app_shim_launch.h"

#import <Foundation/Foundation.h>

#include "base/apple/foundation_util.h"
#include "base/command_line.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/process/launch.h"
#include "base/task/thread_pool.h"
#include "brave/components/constants/mac_app_mode_common.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/common/content_switches.h"

namespace web_app {
namespace {

// Spawns the shim at `shim_path` as a direct child process. Must be called on a
// thread that may block. Returns an invalid process on failure.
base::Process LaunchShimDirectly(const base::FilePath& shim_path,
                                 const base::CommandLine& command_line) {
  NSBundle* bundle =
      [NSBundle bundleWithURL:base::apple::FilePathToNSURL(shim_path)];
  const base::FilePath executable_path =
      base::apple::NSURLToFilePath(bundle.executableURL.absoluteURL);
  if (executable_path.empty()) {
    LOG(ERROR) << "Could not resolve executable in bundle: " << shim_path;
    return base::Process();
  }
  base::CommandLine shim_command_line(executable_path);
  shim_command_line.AppendArguments(command_line, /*include_program=*/false);
  return base::LaunchProcess(shim_command_line, {});
}

// Counterpart of upstream's RunAppLaunchCallbacks for directly spawned shims:
// reports the launch, and waits for process exit on a worker thread to report
// termination.
//
// This deliberately does not reuse RunAppLaunchCallbacks (which observes an
// NSRunningApplication). A directly spawned shim is our own child process, so
// we are responsible for reaping it. WaitForExit both reports termination and
// reaps the child; an NSRunningApplication-based termination watch (KVO) would
// do neither, leaking a zombie in the long-lived test process. What's more,
// reporting the launch from the base::Process we already hold is immediate,
// whereas an NSRunningApplication only becomes available once the shim checks
// in with the window server (~1s after spawn).
void RunAppLaunchCallbacksForDirectLaunch(
    base::Process process,
    ShimLaunchedCallback launched_callback,
    ShimTerminatedCallback terminated_callback) {
  base::Process process_for_wait = process.Duplicate();
  content::GetUIThreadTaskRunner({})->PostTask(
      FROM_HERE,
      base::BindOnce(std::move(launched_callback), std::move(process)));
  base::ThreadPool::PostTask(
      FROM_HERE,
      {base::MayBlock(), base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN},
      base::BindOnce(
          [](base::Process process, ShimTerminatedCallback callback) {
            process.WaitForExit(nullptr);
            content::GetUIThreadTaskRunner({})->PostTask(FROM_HERE,
                                                         std::move(callback));
          },
          std::move(process_for_wait), std::move(terminated_callback)));
}

}  // namespace
}  // namespace web_app

// In tests, spawn the shim as a direct child instead of opening it through
// LaunchServices (see file comment). On spawn failure, fall back to trying the
// remaining candidate shim paths, mirroring upstream's implementation.
#define BRAVE_LAUNCH_THE_FIRST_SHIM_THAT_WORKS_ON_FILE_THREAD                \
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(                     \
          switches::kTestType)) {                                            \
    base::Process process = LaunchShimDirectly(shim_path, command_line);     \
    if (process.IsValid()) {                                                 \
      RunAppLaunchCallbacksForDirectLaunch(std::move(process),               \
                                           std::move(launched_callback),     \
                                           std::move(terminated_callback));  \
    } else {                                                                 \
      LOG(ERROR) << "Failed to launch application with path: " << shim_path; \
      LaunchTheFirstShimThatWorksOnFileThread(                               \
          std::move(shim_paths), launched_after_rebuild, launch_mode,        \
          bundle_id, std::move(launched_callback),                           \
          std::move(terminated_callback));                                   \
    }                                                                        \
    return;                                                                  \
  }

#define LaunchShimForTesting LaunchShimForTesting_ChromiumImpl

#include <chrome/browser/web_applications/os_integration/mac/app_shim_launch.mm>

#undef BRAVE_LAUNCH_THE_FIRST_SHIM_THAT_WORKS_ON_FILE_THREAD
#undef LaunchShimForTesting

namespace web_app {

// Replaces the upstream LaunchServices-based implementation with a direct
// spawn.
void LaunchShimForTesting(const base::FilePath& shim_path,  // IN-TEST
                          const std::vector<GURL>& urls,
                          ShimLaunchedCallback launched_callback,
                          ShimTerminatedCallback terminated_callback,
                          const base::FilePath& chromium_path) {
  base::CommandLine command_line = BuildCommandLineForShimLaunch();
  command_line.AppendSwitch(app_mode::kLaunchedForTest);
  command_line.AppendSwitch(app_mode::kIsNormalLaunch);
  command_line.AppendSwitchPath(app_mode::kLaunchChromeForTest, chromium_path);

  // A shim spawned through LaunchServices receives its launch URLs through
  // -application:openURLs:. Our directly-spawned shim does not have this
  // mechanism, so we pass the URLs to it as command line arguments. Each URL is
  // prefixed with a special tag to distinguish it from other command line
  // arguments.
  for (const GURL& url : urls) {
    command_line.AppendArg(app_mode::kTestLaunchUrlPrefix + url.spec());
  }

  internals::GetShortcutIOTaskRunner()->PostTask(
      FROM_HERE,
      base::BindOnce(
          [](const base::FilePath& shim_path, base::CommandLine command_line,
             ShimLaunchedCallback launched_callback,
             ShimTerminatedCallback terminated_callback) {
            base::ScopedBlockingCall scoped_blocking_call(
                FROM_HERE, base::BlockingType::MAY_BLOCK);
            base::Process process = LaunchShimDirectly(shim_path, command_line);
            if (!process.IsValid()) {
              LOG(ERROR) << "Failed to launch application with path: "
                         << shim_path;
              content::GetUIThreadTaskRunner({})->PostTask(
                  FROM_HERE, base::BindOnce(std::move(launched_callback),
                                            base::Process()));
              return;
            }
            RunAppLaunchCallbacksForDirectLaunch(
                std::move(process), std::move(launched_callback),
                std::move(terminated_callback));
          },
          shim_path, command_line, std::move(launched_callback),
          std::move(terminated_callback)));
}

}  // namespace web_app
