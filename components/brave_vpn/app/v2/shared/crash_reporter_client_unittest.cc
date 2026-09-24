/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/shared/crash_reporter_client.h"

#include <memory>

#include "base/files/file_path.h"
#include "base/memory/ptr_util.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2 {

class CrashReporterClientTest : public testing::Test {
 protected:
  std::unique_ptr<CrashReporterClient> MakeClient(
      const base::FilePath& profile_dir) {
    return base::WrapUnique(
        new CrashReporterClient("app", "unknown", profile_dir));
  }
};

TEST_F(CrashReporterClientTest, CrashDumpLocationAppendsCrashpad) {
  const base::FilePath profile_dir(FILE_PATH_LITERAL("test_profile"));
  auto client = MakeClient(profile_dir);

#if BUILDFLAG(IS_WIN)
  std::wstring dir;
  EXPECT_TRUE(client->GetCrashDumpLocation(&dir));
  EXPECT_EQ(dir, profile_dir.Append(L"Crashpad").value());
#else
  base::FilePath dir;
  EXPECT_TRUE(client->GetCrashDumpLocation(&dir));
  EXPECT_EQ(dir, profile_dir.Append(FILE_PATH_LITERAL("Crashpad")));
#endif
}

TEST_F(CrashReporterClientTest, CrashMetricsLocationIsProfileDir) {
  const base::FilePath profile_dir(FILE_PATH_LITERAL("test_profile"));
  auto client = MakeClient(profile_dir);

#if BUILDFLAG(IS_WIN)
  std::wstring dir;
  EXPECT_TRUE(client->GetCrashMetricsLocation(&dir));
  EXPECT_EQ(dir, profile_dir.value());
#else
  base::FilePath dir;
  EXPECT_TRUE(client->GetCrashMetricsLocation(&dir));
  EXPECT_EQ(dir, profile_dir);
#endif
}

// Regression guard: with an empty profile dir both accessors must return false
// (and not write a bogus relative path into the out-param).
TEST_F(CrashReporterClientTest, EmptyProfileDirReturnsFalse) {
  auto client = MakeClient(base::FilePath());

#if BUILDFLAG(IS_WIN)
  std::wstring dump;
  std::wstring metrics;
#else
  base::FilePath dump;
  base::FilePath metrics;
#endif
  EXPECT_FALSE(client->GetCrashDumpLocation(&dump));
  EXPECT_FALSE(client->GetCrashMetricsLocation(&metrics));
}

// For now, the client never uploads crash reports. If later we change the
// contract, the test must be updated accordingly.
TEST_F(CrashReporterClientTest, NeverUploads) {
  auto client = MakeClient(base::FilePath(FILE_PATH_LITERAL("test_profile")));

  EXPECT_TRUE(client->IsRunningUnattended());
#if !BUILDFLAG(IS_WIN)
  // On Windows consent is delegated to install_static, which needs product
  // details initialized; on POSIX "no consent" is pinned unconditionally.
  EXPECT_FALSE(client->GetCollectStatsConsent());
#endif
}

}  // namespace brave_vpn::v2
