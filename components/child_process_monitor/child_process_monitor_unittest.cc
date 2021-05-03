/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/child_process_monitor/child_process_monitor.h"

#if defined(OS_WIN)
#include <windows.h>
#elif defined(OS_POSIX)
#include <signal.h>
#endif

#include "base/process/kill.h"
#include "base/test/bind.h"
#include "base/test/multiprocess_test.h"
#include "base/test/task_environment.h"
#include "base/test/test_timeouts.h"
#include "base/threading/platform_thread.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace brave {
namespace {
void WaitForChildTermination(base::ProcessHandle handle) {
  int exit_code;
  base::TerminationStatus status = base::TERMINATION_STATUS_STILL_RUNNING;
  do {
    status = base::GetTerminationStatus(handle, &exit_code);
    base::PlatformThread::Sleep(base::TimeDelta::FromMilliseconds(30));
  } while (status == base::TERMINATION_STATUS_STILL_RUNNING);
}
}  // namespace

class ChildProcessMonitorTest : public base::MultiProcessTest {
 protected:
  void SetUp() override {
    callback_runner_ = base::SequencedTaskRunnerHandle::Get();
  }
  void RunLoop() { task_environment_.RunUntilIdle(); }

  base::test::TaskEnvironment task_environment_;
  scoped_refptr<base::SequencedTaskRunner> callback_runner_;
};

MULTIPROCESS_TEST_MAIN(NeverDieChildProcess) {
  while (1) {
    base::PlatformThread::Sleep(TestTimeouts::action_max_timeout());
  }
  return 0;
}

TEST_F(ChildProcessMonitorTest, Terminate) {
  std::unique_ptr<ChildProcessMonitor> monitor =
      std::make_unique<ChildProcessMonitor>();

  base::Process process = SpawnChild("NeverDieChildProcess");
  bool is_called = false;
  monitor->Start(process.Duplicate(),
                 base::BindLambdaForTesting([&](base::ProcessId pid) {
                   EXPECT_TRUE(callback_runner_->RunsTasksInCurrentSequence());
                   EXPECT_EQ(pid, process.Pid());
                   is_called = true;
                 }));
  process.Terminate(0, false);
  WaitForChildTermination(process.Handle());
  RunLoop();
  EXPECT_TRUE(is_called);
}

TEST_F(ChildProcessMonitorTest, Kill) {
  std::unique_ptr<ChildProcessMonitor> monitor =
      std::make_unique<ChildProcessMonitor>();

  base::Process process = SpawnChild("NeverDieChildProcess");
  bool is_called = false;
  monitor->Start(process.Duplicate(),
                 base::BindLambdaForTesting([&](base::ProcessId pid) {
                   EXPECT_TRUE(callback_runner_->RunsTasksInCurrentSequence());
                   EXPECT_EQ(pid, process.Pid());
                   is_called = true;
                 }));
#if defined(OS_WIN)
  HANDLE handle = ::OpenProcess(PROCESS_ALL_ACCESS, 0, process.Pid());
  ::TerminateProcess(handle, 1);
#elif defined(OS_POSIX)
  ::kill(process.Pid(), SIGKILL);
#endif
  WaitForChildTermination(process.Handle());
  RunLoop();
  EXPECT_TRUE(is_called);
}

MULTIPROCESS_TEST_MAIN(FastSleepyChildProcess) {
  base::PlatformThread::Sleep(TestTimeouts::tiny_timeout() * 10);
  return 0;
}

TEST_F(ChildProcessMonitorTest, ChildExit) {
  std::unique_ptr<ChildProcessMonitor> monitor =
      std::make_unique<ChildProcessMonitor>();

  base::Process process = SpawnChild("FastSleepyChildProcess");
  bool is_called = false;
  monitor->Start(process.Duplicate(),
                 base::BindLambdaForTesting([&](base::ProcessId pid) {
                   EXPECT_TRUE(callback_runner_->RunsTasksInCurrentSequence());
                   EXPECT_EQ(pid, process.Pid());
                   is_called = true;
                 }));
  WaitForChildTermination(process.Handle());
  RunLoop();
  EXPECT_TRUE(is_called);
}

MULTIPROCESS_TEST_MAIN(SleepyCrashChildProcess) {
  base::PlatformThread::Sleep(TestTimeouts::tiny_timeout() * 10);
#if defined(OS_POSIX)
  // Have to disable to signal handler for segv so we can get a crash
  // instead of an abnormal termination through the crash dump handler.
  ::signal(SIGSEGV, SIG_DFL);
#endif
  // Make this process have a segmentation fault.
  volatile int* oops = nullptr;
  *oops = 0xDEAD;
  return 1;
}

// TODO(darkdh): This test case is not stable on Windows CI
#if !defined(OS_WIN)
TEST_F(ChildProcessMonitorTest, ChildCrash) {
  std::unique_ptr<ChildProcessMonitor> monitor =
      std::make_unique<ChildProcessMonitor>();

  base::Process process = SpawnChild("SleepyCrashChildProcess");
  bool is_called = false;
  monitor->Start(process.Duplicate(),
                 base::BindLambdaForTesting([&](base::ProcessId pid) {
                   EXPECT_TRUE(callback_runner_->RunsTasksInCurrentSequence());
                   EXPECT_EQ(pid, process.Pid());
                   is_called = true;
                 }));
  WaitForChildTermination(process.Handle());
  RunLoop();
  EXPECT_TRUE(is_called);
}
#endif

}  // namespace brave
