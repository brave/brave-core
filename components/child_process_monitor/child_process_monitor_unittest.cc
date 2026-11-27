/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/child_process_monitor/child_process_monitor.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#elif BUILDFLAG(IS_POSIX)
#include <signal.h>
#endif

#include <utility>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/memory/scoped_refptr.h"
#include "base/process/kill.h"
#include "base/process/process_handle.h"
#include "base/task/sequenced_task_runner.h"
#include "base/test/multiprocess_test.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "base/test/test_timeouts.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace brave {
namespace {
void WaitForChildTermination(base::ProcessHandle handle) {
  int exit_code;
  base::TerminationStatus status = base::TERMINATION_STATUS_STILL_RUNNING;
  do {
    status = base::GetTerminationStatus(handle, &exit_code);
    base::PlatformThread::Sleep(base::Milliseconds(30));
  } while (status == base::TERMINATION_STATUS_STILL_RUNNING);
}

void ExpectSequenceAndForward(
    scoped_refptr<base::SequencedTaskRunner> expected_runner,
    base::OnceCallback<void(base::ProcessId)> callback,
    base::ProcessId pid) {
  EXPECT_TRUE(expected_runner->RunsTasksInCurrentSequence());
  std::move(callback).Run(pid);
}
}  // namespace

class ChildProcessMonitorTest : public base::MultiProcessTest {
 protected:
  void SetUp() override {
    callback_runner_ = base::SequencedTaskRunner::GetCurrentDefault();
  }

  // Checks that the monitor runs |callback| on |callback_runner_|.
  base::OnceCallback<void(base::ProcessId)> OnCallbackSequence(
      base::OnceCallback<void(base::ProcessId)> callback) {
    return base::BindOnce(&ExpectSequenceAndForward, callback_runner_,
                          std::move(callback));
  }

  base::test::TaskEnvironment task_environment_;
  scoped_refptr<base::SequencedTaskRunner> callback_runner_;
};

MULTIPROCESS_TEST_MAIN(NeverDieChildProcess) {
  while (true) {
    base::PlatformThread::Sleep(TestTimeouts::action_max_timeout());
  }
}

TEST_F(ChildProcessMonitorTest, Terminate) {
  std::unique_ptr<ChildProcessMonitor> monitor =
      std::make_unique<ChildProcessMonitor>();

  base::Process process = SpawnChild("NeverDieChildProcess");
  base::test::TestFuture<base::ProcessId> future;
  monitor->Start(process.Duplicate(), OnCallbackSequence(future.GetCallback()));
  process.Terminate(0, false);
  WaitForChildTermination(process.Handle());
  EXPECT_EQ(future.Get(), process.Pid());
}

TEST_F(ChildProcessMonitorTest, Kill) {
  std::unique_ptr<ChildProcessMonitor> monitor =
      std::make_unique<ChildProcessMonitor>();

  base::Process process = SpawnChild("NeverDieChildProcess");
  base::test::TestFuture<base::ProcessId> future;
  monitor->Start(process.Duplicate(), OnCallbackSequence(future.GetCallback()));
#if BUILDFLAG(IS_WIN)
  HANDLE handle = ::OpenProcess(PROCESS_ALL_ACCESS, 0, process.Pid());
  ::TerminateProcess(handle, 1);
#elif BUILDFLAG(IS_POSIX)
  ::kill(process.Pid(), SIGKILL);
#endif
  WaitForChildTermination(process.Handle());
  EXPECT_EQ(future.Get(), process.Pid());
}

MULTIPROCESS_TEST_MAIN(FastSleepyChildProcess) {
  base::PlatformThread::Sleep(TestTimeouts::tiny_timeout() * 10);
  return 0;
}

TEST_F(ChildProcessMonitorTest, ChildExit) {
  std::unique_ptr<ChildProcessMonitor> monitor =
      std::make_unique<ChildProcessMonitor>();

  base::Process process = SpawnChild("FastSleepyChildProcess");

  // Set up monitor before the child process has a chance to exit
  base::test::TestFuture<base::ProcessId> future;
  monitor->Start(process.Duplicate(), OnCallbackSequence(future.GetCallback()));
  EXPECT_EQ(future.Get(), process.Pid());

  WaitForChildTermination(process.Handle());
}

MULTIPROCESS_TEST_MAIN(SleepyCrashChildProcess) {
  base::PlatformThread::Sleep(TestTimeouts::tiny_timeout() * 10);
#if BUILDFLAG(IS_POSIX)
  // Have to disable to signal handler for segv so we can get a crash
  // instead of an abnormal termination through the crash dump handler.
  ::signal(SIGSEGV, SIG_DFL);
#endif
  // Make this process have a segmentation fault.
  volatile int* oops = nullptr;
  *oops = 0xDEAD;
  return 1;
}

TEST_F(ChildProcessMonitorTest, ChildCrash) {
  std::unique_ptr<ChildProcessMonitor> monitor =
      std::make_unique<ChildProcessMonitor>();

  base::Process process = SpawnChild("SleepyCrashChildProcess");

  // Set up monitor immediately after spawning to avoid race condition
  base::test::TestFuture<base::ProcessId> future;
  monitor->Start(process.Duplicate(), OnCallbackSequence(future.GetCallback()));
  EXPECT_EQ(future.Get(), process.Pid());

  WaitForChildTermination(process.Handle());
}

}  // namespace brave
