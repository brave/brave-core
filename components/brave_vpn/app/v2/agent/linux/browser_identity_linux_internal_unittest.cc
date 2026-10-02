/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/linux/browser_identity_linux_internal.h"

#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cerrno>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/strings/strcat.h"
#include "base/test/multiprocess_test.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

namespace brave_vpn::v2::internal {
namespace {

// Builds a /proc/<pid>/stat line with |comm| as field 2 and |start_time| as
// field 22.
std::string MakeStatLine(std::string_view comm, std::string_view start_time) {
  std::string line = base::StrCat({"1234 (", comm, ") S"});
  for (int field = 4; field < 22; ++field) {
    line += " 1";
  }
  base::StrAppend(&line, {" ", start_time, " 4096 100\n"});
  return line;
}

MULTIPROCESS_TEST_MAIN(BrowserIdentityLinuxInternalTestExitImmediately) {
  return 0;
}

}  // namespace

TEST(BrowserIdentityLinuxInternalTest, ParsesStartTime) {
  EXPECT_EQ(ParseProcessStartTimeTicks(MakeStatLine("brave", "98765")), 98765u);
}

TEST(BrowserIdentityLinuxInternalTest, ParsesStartTimeDespiteHostileComm) {
  for (std::string_view comm : {"a b", "a) b (", ")", "(", ") S 1 2 3"}) {
    SCOPED_TRACE(comm);
    EXPECT_EQ(ParseProcessStartTimeTicks(MakeStatLine(comm, "98765")), 98765u);
  }
}

TEST(BrowserIdentityLinuxInternalTest, RejectsMalformedStat) {
  EXPECT_EQ(ParseProcessStartTimeTicks(""), std::nullopt);
  EXPECT_EQ(ParseProcessStartTimeTicks("1234 brave S 1 1 1"), std::nullopt);
  EXPECT_EQ(ParseProcessStartTimeTicks("1234 (brave) S 1 1 1"), std::nullopt);
  EXPECT_EQ(ParseProcessStartTimeTicks(MakeStatLine("brave", "abc")),
            std::nullopt);
}

TEST(BrowserIdentityLinuxInternalTest, ReadsStartTimeOfCurrentProcess) {
  std::string stat_contents;
  ASSERT_TRUE(base::ReadFileToStringNonBlocking(
      base::FilePath("/proc/self/stat"), &stat_contents));
  const std::optional<uint64_t> expected =
      ParseProcessStartTimeTicks(stat_contents);
  ASSERT_TRUE(expected);
  EXPECT_EQ(ReadProcessStartTimeTicks(getpid()), expected);
}

TEST(BrowserIdentityLinuxInternalTest, ProcessIsAliveUntilReaped) {
  const base::ScopedFD self_pidfd = OpenPidfd(getpid());
  if (!self_pidfd.is_valid() && errno == ENOSYS) {
    GTEST_SKIP() << "pidfd_open() needs Linux 5.3+";
  }
  ASSERT_TRUE(self_pidfd.is_valid());
  EXPECT_TRUE(IsProcessAlive(self_pidfd));

  base::Process child = base::SpawnMultiProcessTestChild(
      "BrowserIdentityLinuxInternalTestExitImmediately",
      base::GetMultiProcessTestChildBaseCommandLine(), base::LaunchOptions());
  ASSERT_TRUE(child.IsValid());
  const base::ScopedFD pidfd = OpenPidfd(child.Pid());
  ASSERT_TRUE(pidfd.is_valid());

  // Wait for the child to exit without reaping it: a zombie still holds its
  // pid, so it must count as alive.
  siginfo_t exit_info{};
  ASSERT_EQ(HANDLE_EINTR(waitid(P_PID, static_cast<id_t>(child.Pid()),
                                &exit_info, WEXITED | WNOWAIT)),
            0);
  EXPECT_TRUE(IsProcessAlive(pidfd));

  int exit_code = -1;
  ASSERT_TRUE(child.WaitForExit(&exit_code));  // Reaps the child.
  EXPECT_FALSE(IsProcessAlive(pidfd));
}

}  // namespace brave_vpn::v2::internal
