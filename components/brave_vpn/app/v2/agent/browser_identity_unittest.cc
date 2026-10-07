/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/agent/browser_identity.h"

#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/files/scoped_file.h"
#include "base/memory/scoped_refptr.h"
#include "base/process/launch.h"
#include "base/process/process.h"
#include "base/process/process_handle.h"
#include "base/strings/string_util.h"
#include "base/test/multiprocess_test.h"
#include "build/build_config.h"
#include "components/named_mojo_ipc_server/connection_info.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "testing/multiprocess_func_list.h"

#if BUILDFLAG(IS_WIN)
#include <windows.h>
#elif BUILDFLAG(IS_MAC)
#include <mach/mach.h>
#include <unistd.h>
#elif BUILDFLAG(IS_LINUX)
#include <sys/socket.h>
#include <unistd.h>
#endif

namespace brave_vpn::v2 {
namespace {

using named_mojo_ipc_server::ConnectionInfo;

// Exposes the protected constructor, to build identities from made-up platform
// data, such as two instances of one pid that simulate pid reuse.
class TestBrowserIdentity final : public BrowserIdentity {
 public:
  using BrowserIdentity::PlatformData;

  static scoped_refptr<BrowserIdentity> CreateWithPlatformData(
      base::ProcessId pid,
      PlatformData platform_data) {
    return base::WrapRefCounted(
        new TestBrowserIdentity(pid, std::move(platform_data)));
  }

 private:
  using BrowserIdentity::BrowserIdentity;
  ~TestBrowserIdentity() override = default;
};

// Platform data for process instance |instance| of |pid|: the same |instance|
// means the same process, a different one a process that reused the pid.
TestBrowserIdentity::PlatformData MakePlatformData(base::ProcessId pid,
                                                   uint32_t instance) {
#if BUILDFLAG(IS_WIN)
  return {.process = base::Process(), .creation_time = instance};
#elif BUILDFLAG(IS_MAC)
  audit_token_t token{};
  token.val[5] = static_cast<unsigned int>(pid);  // audit_token_to_pid()
  token.val[7] = instance;                        // audit_token_to_pidversion()
  return {.audit_token = token};
#elif BUILDFLAG(IS_LINUX)
  return {.pidfd = base::ScopedFD(), .start_time_ticks = instance};
#endif  // BUILDFLAG(IS_WIN)
}

// Fills |info| as the server would for a connection from |pid|. On macOS only
// the current process can be described: its audit token is the only one a test
// can obtain.
void DescribeConnectionFrom(base::ProcessId pid, ConnectionInfo& info) {
  info.pid = pid;
#if BUILDFLAG(IS_WIN)
  DWORD session_id = 0;
  CHECK(::ProcessIdToSessionId(pid, &session_id));
  info.session_id = session_id;
  info.process =
      base::Process::OpenWithAccess(pid, PROCESS_QUERY_LIMITED_INFORMATION);
  CHECK(info.process.IsValid());
#elif BUILDFLAG(IS_MAC)
  CHECK_EQ(pid, base::GetCurrentProcId());
  mach_msg_type_number_t count = TASK_AUDIT_TOKEN_COUNT;
  CHECK_EQ(task_info(mach_task_self(), TASK_AUDIT_TOKEN,
                     reinterpret_cast<task_info_t>(&info.audit_token), &count),
           KERN_SUCCESS);
#elif BUILDFLAG(IS_LINUX)
  info.credentials = {.pid = pid, .uid = geteuid(), .gid = getegid()};
#endif  // BUILDFLAG(IS_WIN)
}

scoped_refptr<BrowserIdentity> CaptureCurrentProcess() {
  ConnectionInfo info;
  DescribeConnectionFrom(base::GetCurrentProcId(), info);
  return BrowserIdentity::Create(info);
}

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)
MULTIPROCESS_TEST_MAIN(BrowserIdentityTestExitImmediately) {
  return 0;
}

// Returns a child that has exited. On Linux it has also been reaped; on
// Windows the returned handle keeps its pid from being reused.
base::Process SpawnExitedChild() {
  base::Process child = base::SpawnMultiProcessTestChild(
      "BrowserIdentityTestExitImmediately",
      base::GetMultiProcessTestChildBaseCommandLine(), base::LaunchOptions());
  CHECK(child.IsValid());
  int exit_code = -1;
  CHECK(child.WaitForExit(&exit_code));
  return child;
}
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_WIN)

}  // namespace

TEST(BrowserIdentityTest, CapturesCurrentProcess) {
  scoped_refptr<BrowserIdentity> identity = CaptureCurrentProcess();
  ASSERT_TRUE(identity);
  EXPECT_EQ(identity->pid(), base::GetCurrentProcId());
  EXPECT_TRUE(base::StartsWith(identity->GetDescription(), "pid="));
#if BUILDFLAG(IS_WIN)
  constexpr std::string_view kPlatformField = "; creation_time=";
#elif BUILDFLAG(IS_MAC)
  constexpr std::string_view kPlatformField = "; pidversion=";
#elif BUILDFLAG(IS_LINUX)
  constexpr std::string_view kPlatformField = "; start_time_ticks=";
#endif
  EXPECT_NE(identity->GetDescription().find(kPlatformField), std::string::npos);
}

TEST(BrowserIdentityTest, CapturesOfOneProcessAreSameProcess) {
  scoped_refptr<BrowserIdentity> first = CaptureCurrentProcess();
  scoped_refptr<BrowserIdentity> second = CaptureCurrentProcess();
  ASSERT_TRUE(first);
  ASSERT_TRUE(second);
  EXPECT_TRUE(first->IsSameProcess(*second));
  EXPECT_TRUE(second->IsSameProcess(*first));
}

TEST(BrowserIdentityTest, RejectsNullPid) {
  // Zeroed, as for a peer that could not be identified.
  ConnectionInfo info;
  EXPECT_FALSE(BrowserIdentity::Create(info));
}

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)
TEST(BrowserIdentityTest, RejectsPeerRunningAsAnotherUser) {
  ConnectionInfo info;
  DescribeConnectionFrom(base::GetCurrentProcId(), info);
#if BUILDFLAG(IS_LINUX)
  info.credentials.uid = geteuid() + 1;
#else
  info.audit_token.val[1] = geteuid() + 1;  // audit_token_to_euid()
#endif
  EXPECT_FALSE(BrowserIdentity::Create(info));
}
#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_MAC)

#if BUILDFLAG(IS_WIN)
TEST(BrowserIdentityTest, RejectsExitedPeer) {
  const base::Process child = SpawnExitedChild();
  ConnectionInfo info;
  DescribeConnectionFrom(child.Pid(), info);
  EXPECT_FALSE(BrowserIdentity::Create(info));
}

TEST(BrowserIdentityTest, RejectsConnectionWithoutProcessHandle) {
  ConnectionInfo info;
  DescribeConnectionFrom(base::GetCurrentProcId(), info);
  info.process = base::Process();
  EXPECT_FALSE(BrowserIdentity::Create(info));
}

TEST(BrowserIdentityTest, RejectsUnqueryableProcessHandle) {
  // As if the server opened the peer without query rights.
  ConnectionInfo info;
  DescribeConnectionFrom(base::GetCurrentProcId(), info);
  info.process = base::Process::OpenWithAccess(info.pid, SYNCHRONIZE);
  ASSERT_TRUE(info.process.IsValid());
  EXPECT_FALSE(BrowserIdentity::Create(info));
}

TEST(BrowserIdentityTest, RejectsProcessHandleNotMatchingPid) {
  // The handle names a live process of this user, so only the pid check can
  // reject it.
  const base::Process child = SpawnExitedChild();
  ConnectionInfo info;
  DescribeConnectionFrom(child.Pid(), info);
  info.process = base::Process::OpenWithAccess(
      base::GetCurrentProcId(), PROCESS_QUERY_LIMITED_INFORMATION);
  ASSERT_TRUE(info.process.IsValid());
  EXPECT_FALSE(BrowserIdentity::Create(info));
}

TEST(BrowserIdentityTest, RejectsPeerInAnotherSession) {
  ConnectionInfo info;
  DescribeConnectionFrom(base::GetCurrentProcId(), info);
  ++info.session_id;
  EXPECT_FALSE(BrowserIdentity::Create(info));
}
#endif  // BUILDFLAG(IS_WIN)

TEST(BrowserIdentityTest, ComparesProcessInstanceNotJustPid) {
  constexpr base::ProcessId kPid = 1234;
  scoped_refptr<BrowserIdentity> original =
      TestBrowserIdentity::CreateWithPlatformData(
          kPid, MakePlatformData(kPid, /*instance=*/1));
  scoped_refptr<BrowserIdentity> same =
      TestBrowserIdentity::CreateWithPlatformData(
          kPid, MakePlatformData(kPid, /*instance=*/1));
  scoped_refptr<BrowserIdentity> pid_reused =
      TestBrowserIdentity::CreateWithPlatformData(
          kPid, MakePlatformData(kPid, /*instance=*/2));
  scoped_refptr<BrowserIdentity> other_pid =
      TestBrowserIdentity::CreateWithPlatformData(
          kPid + 1, MakePlatformData(kPid + 1, /*instance=*/1));

  EXPECT_TRUE(original->IsSameProcess(*same));
  EXPECT_TRUE(same->IsSameProcess(*original));
  EXPECT_FALSE(original->IsSameProcess(*pid_reused));
  EXPECT_FALSE(pid_reused->IsSameProcess(*original));
  EXPECT_FALSE(original->IsSameProcess(*other_pid));
}

}  // namespace brave_vpn::v2
