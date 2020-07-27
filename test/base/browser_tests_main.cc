/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/command_line.h"
#include "base/test/launcher/test_launcher.h"
#include "brave/test/base/brave_test_launcher_delegate.h"
#include "build/build_config.h"

#if defined(OS_WIN)
#include "base/test/test_switches.h"
#include "base/win/win_util.h"
#endif  // defined(OS_WIN)

int main(int argc, char** argv) {
  base::CommandLine::Init(argc, argv);
  size_t parallel_jobs = base::NumParallelJobs(/*cores_per_job=*/1);
  if (parallel_jobs == 0U) {
    return 1;
  } else if (parallel_jobs > 1U) {
    parallel_jobs /= 2U;
  }

#if defined(OS_WIN)
  // Enable high-DPI for interactive tests where the user is expected to
  // manually verify results.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kTestLauncherInteractive)) {
    base::win::EnableHighDPISupport();
  }
#endif  // defined(OS_WIN)

  ChromeTestSuiteRunner runner;
  BraveTestLauncherDelegate delegate(&runner);
  return LaunchChromeTests(parallel_jobs, &delegate, argc, argv);
}
