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

#include "base/check.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_file.h"
#include "base/posix/eintr_wrapper.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
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

// Builds a /proc/<pid>/status excerpt with |name| on the "Name:" line and
// |uid_line| as the uid line, between lines that surround it in the real file.
std::string MakeStatus(std::string_view name, std::string_view uid_line) {
  return base::StrCat({"Name:\t", name,
                       "\nUmask:\t0022\nState:\tS (sleeping)\nPid:\t1234\n",
                       uid_line, "\nGid:\t1000\t1000\t1000\t1000\n"});
}

// The kernel never allocates pid_max itself, so no process has this pid.
pid_t UnusedPid() {
  std::string contents;
  CHECK(base::ReadFileToString(base::FilePath("/proc/sys/kernel/pid_max"),
                               &contents));
  int pid_max = 0;
  CHECK(base::StringToInt(base::TrimWhitespaceASCII(contents, base::TRIM_ALL),
                          &pid_max));
  return pid_max;
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

TEST(BrowserIdentityLinuxInternalTest, ParsesEffectiveUid) {
  // All four ids differ, so picking any but the effective one fails.
  EXPECT_EQ(ParseProcessEffectiveUid(
                MakeStatus("brave", "Uid:\t1000\t1001\t1002\t1003")),
            1001u);
}

TEST(BrowserIdentityLinuxInternalTest, ParsesEffectiveUidDespiteHostileName) {
  // As the kernel prints a process that named itself "x\nUid:\t0\t0\t0\t0":
  // the newline arrives escaped, so the name stays on its own line.
  EXPECT_EQ(ParseProcessEffectiveUid(MakeStatus(
                "x\\nUid:\t0\t0\t0\t0", "Uid:\t1000\t1001\t1002\t1003")),
            1001u);
}

TEST(BrowserIdentityLinuxInternalTest, RejectsMalformedStatus) {
  for (std::string_view uid_line : {
           "",                                    // No uid line.
           "Uid:",                                // No uids.
           "Uid:\t1000\t1001\t1002",              // Too few.
           "Uid:\t1000\t1001\t1002\t1003\t1",     // Too many.
           "Uid:\t1000\tabc\t1002\t1003",         // Not a number.
           "Uid:\t1000\t-1\t1002\t1003",          // Negative.
           "Uid:\t1000\t4294967296\t1002\t1003",  // Overflows uid_t.
       }) {
    SCOPED_TRACE(uid_line);
    EXPECT_EQ(ParseProcessEffectiveUid(MakeStatus("brave", uid_line)),
              std::nullopt);
  }
  EXPECT_EQ(ParseProcessEffectiveUid(""), std::nullopt);
}

TEST(BrowserIdentityLinuxInternalTest, RejectsStatusWithTwoUidLines) {
  // Cannot come from the kernel, which escapes newlines in the name; refused
  // rather than trusting either line.
  EXPECT_EQ(ParseProcessEffectiveUid(MakeStatus(
                "x\nUid:\t0\t0\t0\t0", "Uid:\t1000\t1001\t1002\t1003")),
            std::nullopt);
}

TEST(BrowserIdentityLinuxInternalTest, ReadsEffectiveUidOfCurrentProcess) {
  // Checked against geteuid(), a source independent of the parser.
  EXPECT_EQ(ReadProcessEffectiveUid(getpid()), geteuid());
}

TEST(BrowserIdentityLinuxInternalTest, NoEffectiveUidForNonexistentProcess) {
  EXPECT_EQ(ReadProcessEffectiveUid(UnusedPid()), std::nullopt);
}

TEST(BrowserIdentityLinuxInternalTest, NoStartTimeForNonexistentProcess) {
  EXPECT_EQ(ReadProcessStartTimeTicks(UnusedPid()), std::nullopt);
}

}  // namespace brave_vpn::v2::internal
