/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/agent_app.h"

#include <string>

#include "base/check.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/bind.h"
#include "base/location.h"
#include "base/process/process.h"
#include "base/run_loop.h"
#include "base/synchronization/waitable_event.h"
#include "base/task/single_thread_task_executor.h"
#include "base/test/bind.h"
#include "base/test/gtest_util.h"
#include "base/test/multiprocess_test.h"
#include "base/test/test_timeouts.h"
#include "base/threading/platform_thread.h"
#include "base/threading/thread.h"
#include "base/time/time.h"
#include "brave/components/brave_vpn/app/v2/agent/shutdown_handlers.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

#if BUILDFLAG(IS_POSIX)
#include <signal.h>
#endif  // BUILDFLAG(IS_POSIX)

namespace brave_vpn::v2 {

TEST(AgentAppDeathTest, ShutdownBeforeRunChecks) {
  AgentApp app;
  EXPECT_CHECK_DEATH(app.Shutdown());
}

// The functional signal tests exercise the real OS signal path end-to-end
// (sigaction -> self-pipe -> detector thread -> Shutdown -> RunLoop quit), so
// they must run against a separate child process: delivering signals to the
// test binary itself would mutate its process-global dispositions and race
// the test launcher's own signal handling.
//
// The Windows console-event analog is deliberately not implemented, as it
// requires the child to own a console, which may not be the case in test
// environments.
#if BUILDFLAG(IS_POSIX)

namespace {

// Switch used to hand a per-test temp dir to the child for handshaking.
constexpr char kSignalDirSwitch[] = "agent-app-signal-dir";

// Marker file the child creates once it is ready to receive signals.
constexpr base::FilePath::CharType kReadySignal[] =
    FILE_PATH_LITERAL("child_ready");

bool SignalEvent(const base::FilePath& path) {
  return base::WriteFile(path, "");
}

// Polls for |path| to appear, up to the standard action timeout.
bool WaitForEvent(const base::FilePath& path) {
  const base::TimeTicks deadline =
      base::TimeTicks::Now() + TestTimeouts::action_timeout();
  while (base::TimeTicks::Now() < deadline) {
    if (base::PathExists(path)) {
      return true;
    }
    base::PlatformThread::Sleep(base::Milliseconds(10));
  }
  return false;
}

base::FilePath SignalDirFromCommandLine() {
  const base::FilePath dir =
      base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
          kSignalDirSwitch);
  CHECK(!dir.empty());
  return dir;
}

}  // namespace

// Child entry point: Run() sets up the task runner and quit closure and then
// ShutdownHandlers::Install() registers SIGINT, SIGTERM, and SIGHUP, in that
// order. Once SIGHUP's disposition is no longer SIG_DFL, everything the signal
// path depends on is provably in place, so a watcher thread polls for that and
// only then tells the parent it is safe to send the signal under test.
MULTIPROCESS_TEST_MAIN(AgentAppRunChild) {
  const base::FilePath dir = SignalDirFromCommandLine();

  base::Thread readiness("readiness_watcher");
  CHECK(readiness.Start());
  readiness.task_runner()->PostTask(
      FROM_HERE, base::BindLambdaForTesting([dir] {
        struct sigaction current = {};
        do {
          base::PlatformThread::Sleep(base::Milliseconds(1));
          CHECK_EQ(0, sigaction(SIGHUP, nullptr, &current));
        } while (current.sa_handler == SIG_DFL);
        CHECK(SignalEvent(dir.Append(kReadySignal)));
      }));

  AgentApp app;
  return app.Run();
}

// Child whose shutdown callback never completes, simulating a hung graceful
// shutdown. Uses ShutdownHandlers directly (not AgentApp, whose shutdown cannot
// hang), so readiness can simply be signaled after Install() returns.
MULTIPROCESS_TEST_MAIN(HungShutdownChild) {
  const base::FilePath dir = SignalDirFromCommandLine();

  base::SingleThreadTaskExecutor executor;
  base::RunLoop run_loop;

  base::WaitableEvent never_signaled;
  ShutdownHandlers handlers(
      base::BindRepeating([](base::WaitableEvent* event) { event->Wait(); },
                          base::Unretained(&never_signaled)));
  CHECK(handlers.Install());
  CHECK(SignalEvent(dir.Append(kReadySignal)));

  run_loop.Run();  // Never quits; the test ends this process with a signal.
  return 0;
}

struct SignalParam {
  int signal;
  const char* name;
};

class AgentAppSignalTest : public base::MultiProcessTest,
                           public testing::WithParamInterface<SignalParam> {
 protected:
  void SetUp() override {
    base::MultiProcessTest::SetUp();
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

  // Inject the per-test signal directory into every spawned child.
  base::CommandLine MakeCmdLine(const std::string& procname) override {
    base::CommandLine command_line =
        base::MultiProcessTest::MakeCmdLine(procname);
    command_line.AppendSwitchPath(kSignalDirSwitch, temp_dir_.GetPath());
    return command_line;
  }

  base::FilePath ready_signal() const {
    return temp_dir_.GetPath().Append(kReadySignal);
  }

  int signal() const { return GetParam().signal; }

  base::ScopedTempDir temp_dir_;
};

INSTANTIATE_TEST_SUITE_P(,
                         AgentAppSignalTest,
                         testing::Values(SignalParam{SIGINT, "SIGINT"},
                                         SignalParam{SIGTERM, "SIGTERM"},
                                         SignalParam{SIGHUP, "SIGHUP"}),
                         [](const testing::TestParamInfo<SignalParam>& info) {
                           return std::string(info.param.name);
                         });

// Each handled signal must drive the full graceful path and produce a clean
// exit code.
TEST_P(AgentAppSignalTest, SignalTriggersGracefulExit) {
  base::Process child = SpawnChild("AgentAppRunChild");
  ASSERT_TRUE(child.IsValid());
  ASSERT_TRUE(WaitForEvent(ready_signal()));

  ASSERT_EQ(0, kill(child.Handle(), signal()));

  int exit_code = -1;
  ASSERT_TRUE(
      child.WaitForExitWithTimeout(TestTimeouts::action_timeout(), &exit_code));
  EXPECT_EQ(0, exit_code);
}

// If the graceful shutdown hangs, the first signal must leave the process
// running (handler consumed, callback stuck), and a second signal must kill it
// via the restored default disposition.
TEST_P(AgentAppSignalTest, SecondSignalKillsHungShutdown) {
  base::Process child = SpawnChild("HungShutdownChild");
  ASSERT_TRUE(child.IsValid());
  ASSERT_TRUE(WaitForEvent(ready_signal()));

  ASSERT_EQ(0, kill(child.Handle(), signal()));

  // The child must survive the first signal: its shutdown callback blocks
  // forever, so this negative wait is deterministic, not timing-dependent.
  int exit_code = -1;
  EXPECT_FALSE(
      child.WaitForExitWithTimeout(base::Milliseconds(250), &exit_code));

  // The handler restored SIG_DFL for this signal before writing to the pipe, so
  // the second signal takes the default disposition and terminates the process
  // even though the graceful path is stuck.
  ASSERT_EQ(0, kill(child.Handle(), signal()));
  ASSERT_TRUE(
      child.WaitForExitWithTimeout(TestTimeouts::action_timeout(), &exit_code));
  // Terminated by the signal's default disposition, so anything but a clean
  // exit; base reports a non-zero code for signal deaths.
  EXPECT_NE(0, exit_code);
}

#endif  // BUILDFLAG(IS_POSIX)

}  // namespace brave_vpn::v2
