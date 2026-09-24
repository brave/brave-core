/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/single_instance.h"

#include <memory>
#include <string>

#include "base/check.h"
#include "base/command_line.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/process/process.h"
#include "base/test/gtest_util.h"
#include "base/test/multiprocess_test.h"
#include "base/test/test_timeouts.h"
#include "base/threading/platform_thread.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace brave_vpn::v2 {

namespace {

// Switch used to hand the lock directory (a per-test temp dir) to the child.
constexpr char kLockDirSwitch[] = "single-instance-lock-dir";

// Filesystem "events" used to hand-shake between parent and child, since the
// two processes share no memory. Same pattern as base's file_locking_unittest.
constexpr base::FilePath::CharType kChildLockedSignal[] =
    FILE_PATH_LITERAL("child_locked");
constexpr base::FilePath::CharType kParentReleaseSignal[] =
    FILE_PATH_LITERAL("parent_release");

// Creates an empty marker file. Returns false on failure.
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

// Tries to acquire, retrying briefly. Used after a holder dies, because the OS
// may take a moment to reap the process and release its lock.
std::unique_ptr<SingleInstance> AcquireWithRetry(const base::FilePath& dir) {
  const base::TimeTicks deadline =
      base::TimeTicks::Now() + TestTimeouts::action_timeout();
  for (;;) {
    if (auto instance = SingleInstance::TryAcquire(dir)) {
      return instance;
    }
    if (base::TimeTicks::Now() >= deadline) {
      return nullptr;
    }
    base::PlatformThread::Sleep(base::Milliseconds(10));
  }
}

}  // namespace

// Child entry point: acquire the lock, tell the parent we hold it, then hold it
// until the parent releases us. Any failure CHECK-fails with a message; the
// parent keys off the "locked" marker file appearing, not the exit code.
MULTIPROCESS_TEST_MAIN(SingleInstanceChildHolder) {
  const base::FilePath dir =
      base::CommandLine::ForCurrentProcess()->GetSwitchValuePath(
          kLockDirSwitch);
  CHECK(!dir.empty());

  std::unique_ptr<SingleInstance> instance = SingleInstance::TryAcquire(dir);
  CHECK(instance) << "child could not acquire the single-instance lock";

  CHECK(SignalEvent(dir.Append(kChildLockedSignal)));
  CHECK(WaitForEvent(dir.Append(kParentReleaseSignal)));

  return 0;  // The lock is released as the process exits.
}

class SingleInstanceTest : public base::MultiProcessTest {
 protected:
  void SetUp() override {
    base::MultiProcessTest::SetUp();
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
  }

  // Inject the per-test lock directory into every spawned child.
  base::CommandLine MakeCmdLine(const std::string& procname) override {
    base::CommandLine command_line =
        base::MultiProcessTest::MakeCmdLine(procname);
    command_line.AppendSwitchPath(kLockDirSwitch, dir());
    return command_line;
  }

  base::FilePath dir() const { return temp_dir_.GetPath(); }
  base::FilePath locked_signal() const {
    return dir().Append(kChildLockedSignal);
  }
  base::FilePath release_signal() const {
    return dir().Append(kParentReleaseSignal);
  }

  base::ScopedTempDir temp_dir_;
};

TEST_F(SingleInstanceTest, AcquireSucceedsOnFreshDirectory) {
  EXPECT_TRUE(SingleInstance::TryAcquire(dir()));
}

TEST_F(SingleInstanceTest, ReacquireSucceedsAfterOwnerDestroyed) {
  {
    std::unique_ptr<SingleInstance> instance =
        SingleInstance::TryAcquire(dir());
    ASSERT_TRUE(instance);
  }
  // Destroying the owner releases the lock, so the slot is free again.
  EXPECT_TRUE(SingleInstance::TryAcquire(dir()));
}

// Contention is only meaningful across processes: lock ownership is per
// process, so a second TryAcquire() in this process would not reliably
// reproduce it (classic POSIX record locks let the same process relock, and the
// per-platform primitives differ). Spawn a holder and verify we are refused.
TEST_F(SingleInstanceTest, SecondInstanceRejectedWhileHeldByOtherProcess) {
  base::Process child = SpawnChild("SingleInstanceChildHolder");
  ASSERT_TRUE(child.IsValid());
  ASSERT_TRUE(WaitForEvent(locked_signal()));

  // The child holds the lock; this process must be refused.
  EXPECT_FALSE(SingleInstance::TryAcquire(dir()));

  // Let the child exit cleanly, releasing the lock.
  ASSERT_TRUE(SignalEvent(release_signal()));
  int exit_code = -1;
  ASSERT_TRUE(
      child.WaitForExitWithTimeout(TestTimeouts::action_timeout(), &exit_code));
  EXPECT_EQ(0, exit_code);

  // With the holder gone, the slot is free again.
  EXPECT_TRUE(AcquireWithRetry(dir()));
}

// Crash-resistance: the kernel releases the lock when the holder dies, even
// without a graceful release. Kill the holder and confirm we can acquire.
TEST_F(SingleInstanceTest, LockReleasedWhenHolderIsKilled) {
  base::Process child = SpawnChild("SingleInstanceChildHolder");
  ASSERT_TRUE(child.IsValid());
  ASSERT_TRUE(WaitForEvent(locked_signal()));
  EXPECT_FALSE(SingleInstance::TryAcquire(dir()));

  // Terminate without signaling release -- stands in for a crash.
  ASSERT_TRUE(child.Terminate(/*exit_code=*/1, /*wait=*/true));

  EXPECT_TRUE(AcquireWithRetry(dir()));
}

#if BUILDFLAG(IS_POSIX)
// The empty-dir CHECK lives in the POSIX implementation (the lock file path is
// derived from the directory). The Windows mutex backend ignores the directory,
// so this contract is POSIX-only.
TEST(SingleInstanceDeathTest, EmptyDirectoryChecks) {
  EXPECT_CHECK_DEATH(SingleInstance::TryAcquire(base::FilePath()));
}
#endif  // BUILDFLAG(IS_POSIX)

}  // namespace brave_vpn::v2
