/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/v2/agent/agent_launcher.h"

#include <array>
#include <memory>
#include <utility>
#include <vector>

#include "base/check_op.h"
#include "base/files/file_path.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {
namespace {

using LaunchError = AgentLauncher::LaunchError;

// Helper function: returns a non-empty path to the agent without touching the
// filesystem.
base::FilePath AgentPath() {
  return base::FilePath(FILE_PATH_LITERAL("agent"));
}

// The resolver runs on a blocking sequence, so it must not reach back into the
// test: the answer is bound by value.
AgentLauncher::AgentPathResolver ResolverReturning(base::FilePath path) {
  return base::BindRepeating([](const base::FilePath& path) { return path; },
                             std::move(path));
}

// Wraps |on_failure| so |on_resolved| runs when the wrapper's bound state is
// destroyed. That is the only externally visible signal that a launch has been
// dealt with, because three of the four outcomes - superseded, succeeded, or
// outlived by its launcher - produce no call at all. Destruction also follows a
// normal call, so a test that wants "resolved without reporting" asserts on
// both this and the report future.
AgentLauncher::LaunchFailureCallback WatchResolution(
    base::OnceClosure on_resolved,
    AgentLauncher::LaunchFailureCallback on_failure) {
  return base::BindOnce(
      [](base::ScopedClosureRunner, AgentLauncher::LaunchFailureCallback cb,
         LaunchError error) { std::move(cb).Run(error); },
      base::ScopedClosureRunner(std::move(on_resolved)), std::move(on_failure));
}

// Stands in for internal::LaunchAgentProcess(): records each launch and parks
// its failure callback, so the test decides when - and whether - a launch
// fails. Invoked on the launcher's sequence, so touching test state is safe.
class FakeProcessLauncher {
 public:
  AgentLauncher::ProcessLauncher GetCallback() {
    return base::BindRepeating(&FakeProcessLauncher::OnLaunch,
                               base::Unretained(this));
  }

  size_t launch_count() const { return callbacks_.size(); }
  const base::FilePath& last_path() const { return last_path_; }

  // Fails the |index|'th launch. The callback is bound to the launcher's
  // sequence, so the reply is posted rather than run inline.
  void FailLaunch(size_t index, LaunchError error) {
    CHECK_LT(index, callbacks_.size());
    CHECK(callbacks_[index]);
    std::move(callbacks_[index]).Run(error);
  }

  // Succeeds the |index|'th launch. Dropping the callback unrun is how the
  // platform layer reports success - there is no success reply.
  void SucceedLaunch(size_t index) {
    CHECK_LT(index, callbacks_.size());
    callbacks_[index].Reset();
  }

 private:
  void OnLaunch(AgentLauncher::LaunchFailureCallback failure_callback,
                const base::FilePath& agent_path) {
    last_path_ = agent_path;
    callbacks_.push_back(std::move(failure_callback));
  }

  base::FilePath last_path_;
  std::vector<AgentLauncher::LaunchFailureCallback> callbacks_;
};

}  // namespace

class AgentLauncherTest : public testing::Test {
 protected:
  std::unique_ptr<AgentLauncher> CreateLauncher(base::FilePath path) {
    return AgentLauncher::CreateForTesting(ResolverReturning(std::move(path)),
                                           process_launcher_.GetCallback());
  }

  // Waits for the path lookup to come back and reach the process launcher.
  [[nodiscard]] bool WaitForLaunchCount(size_t count) {
    return base::test::RunUntil(
        [&] { return process_launcher_.launch_count() == count; });
  }

  base::test::TaskEnvironment task_environment_;
  FakeProcessLauncher process_launcher_;
};

// An empty path is the only way of saying "not installed", and it must be
// answered without attempting to create a process.
TEST_F(AgentLauncherTest, ReportsAppNotFoundWhenPathIsEmpty) {
  base::test::TestFuture<LaunchError> reported;
  std::unique_ptr<AgentLauncher> launcher = CreateLauncher(base::FilePath());

  launcher->Launch(reported.GetCallback());

  EXPECT_EQ(reported.Get(), LaunchError::kAppNotFound);
  EXPECT_EQ(process_launcher_.launch_count(), 0u);
}

TEST_F(AgentLauncherTest, ReportsWhenLaunchFails) {
  base::test::TestFuture<LaunchError> reported;
  std::unique_ptr<AgentLauncher> launcher = CreateLauncher(AgentPath());

  launcher->Launch(reported.GetCallback());

  ASSERT_TRUE(WaitForLaunchCount(1))
      << "the launch never reached the platform layer";
  EXPECT_EQ(process_launcher_.last_path(), AgentPath());
  // Nothing is reported until the platform layer says something.
  EXPECT_FALSE(reported.IsReady());

  process_launcher_.FailLaunch(0, LaunchError::kLaunchFailed);

  EXPECT_EQ(reported.Get(), LaunchError::kLaunchFailed);
}

// Success has no reply, so the callback is dropped.
TEST_F(AgentLauncherTest, NoReportWhenLaunchSucceeds) {
  base::test::TestFuture<LaunchError> reported;
  base::test::TestFuture<void> resolved;
  std::unique_ptr<AgentLauncher> launcher = CreateLauncher(AgentPath());

  launcher->Launch(
      WatchResolution(resolved.GetCallback(), reported.GetCallback()));
  ASSERT_TRUE(WaitForLaunchCount(1));

  process_launcher_.SucceedLaunch(0);

  ASSERT_TRUE(resolved.Wait()) << "the dropped callback was never destroyed";
  EXPECT_FALSE(reported.IsReady());
}

// A failure from a launch that a later one superseded says nothing about the
// current attempt.
TEST_F(AgentLauncherTest, SupersededLaunchDoesNotReport) {
  base::test::TestFuture<LaunchError> first_reported;
  base::test::TestFuture<void> first_resolved;
  base::test::TestFuture<LaunchError> second_reported;
  std::unique_ptr<AgentLauncher> launcher = CreateLauncher(AgentPath());

  launcher->Launch(WatchResolution(first_resolved.GetCallback(),
                                   first_reported.GetCallback()));
  ASSERT_TRUE(WaitForLaunchCount(1));

  launcher->Launch(second_reported.GetCallback());
  ASSERT_TRUE(WaitForLaunchCount(2));

  // The first launch fails only now, after the second one has started.
  process_launcher_.FailLaunch(0, LaunchError::kLaunchFailed);

  ASSERT_TRUE(first_resolved.Wait())
      << "the superseded failure was never dispatched";
  EXPECT_FALSE(first_reported.IsReady());
  EXPECT_FALSE(second_reported.IsReady());

  // The launch in force still reports.
  process_launcher_.FailLaunch(1, LaunchError::kLaunchFailed);

  EXPECT_EQ(second_reported.Get(), LaunchError::kLaunchFailed);
  EXPECT_FALSE(first_reported.IsReady());
}

// Covers the earlier of the two cancellation points: superseded while the path
// lookup is still in flight, the older launch must not reach the process
// launcher at all.
TEST_F(AgentLauncherTest, SupersededBeforePathResolvesDoesNotLaunch) {
  base::test::TestFuture<LaunchError> first_reported;
  base::test::TestFuture<void> first_resolved;
  base::test::TestFuture<LaunchError> second_reported;
  std::unique_ptr<AgentLauncher> launcher = CreateLauncher(AgentPath());

  launcher->Launch(WatchResolution(first_resolved.GetCallback(),
                                   first_reported.GetCallback()));
  launcher->Launch(second_reported.GetCallback());

  // The first launch is resolved when its path reply is dispatched and found
  // cancelled; had it survived, the callback would have been handed to the
  // process launcher instead of destroyed.
  ASSERT_TRUE(first_resolved.Wait())
      << "the superseded path lookup was never dispatched";
  ASSERT_TRUE(WaitForLaunchCount(1));
  EXPECT_FALSE(first_reported.IsReady());

  process_launcher_.FailLaunch(0, LaunchError::kLaunchFailed);

  EXPECT_EQ(second_reported.Get(), LaunchError::kLaunchFailed);
  EXPECT_EQ(process_launcher_.launch_count(), 1u);
}

// A burst of launches produces one report, not one per call.
TEST_F(AgentLauncherTest, BurstOfLaunchesReportsOnce) {
  constexpr size_t kLaunches = 5;
  std::array<base::test::TestFuture<LaunchError>, kLaunches> reported;
  std::array<base::test::TestFuture<void>, kLaunches> resolved;
  std::unique_ptr<AgentLauncher> launcher = CreateLauncher(base::FilePath());

  for (size_t i = 0; i < kLaunches; ++i) {
    launcher->Launch(
        WatchResolution(resolved[i].GetCallback(), reported[i].GetCallback()));
  }

  for (base::test::TestFuture<void>& launch_resolved : resolved) {
    ASSERT_TRUE(launch_resolved.Wait()) << "a launch was never dispatched";
  }

  const size_t report_count = std::ranges::count_if(
      reported, [](const auto& item) { return item.IsReady(); });
  EXPECT_EQ(report_count, 1u);

  // The most recent launch gets the answer.
  ASSERT_TRUE(reported.back().IsReady());
  EXPECT_EQ(reported.back().Get(), LaunchError::kAppNotFound);
}

// A launch outliving the launcher must not call back into it. Covers weak
// pointer teardown, and is supposed to catch base::Unretained "simplification"
// regressions by ASAN.
TEST_F(AgentLauncherTest, DestructionDuringLaunchReportsNothing) {
  base::test::TestFuture<LaunchError> reported;
  base::test::TestFuture<void> resolved;
  std::unique_ptr<AgentLauncher> launcher = CreateLauncher(AgentPath());

  launcher->Launch(
      WatchResolution(resolved.GetCallback(), reported.GetCallback()));
  ASSERT_TRUE(WaitForLaunchCount(1));

  launcher.reset();

  process_launcher_.FailLaunch(0, LaunchError::kLaunchFailed);

  ASSERT_TRUE(resolved.Wait());
  EXPECT_FALSE(reported.IsReady());
}

}  // namespace brave_vpn::v2
