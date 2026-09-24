/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/app/v2/shared/app_utils.h"

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/gtest_util.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn::v2::app_utils {

// An empty product name is a programming error and must CHECK rather than
// silently produce a company-root path.
TEST(AppUtilsDeathTest, EmptyProductNameChecks) {
  EXPECT_CHECK_DEATH(
      GetUserDataDir(std::string(), /*is_privileged_process=*/false));
  EXPECT_CHECK_DEATH(
      GetUserDataDir(std::string(), /*is_privileged_process=*/true));
}

// The base directory varies by platform, but the last two path components are
// always "<company>/<product>".
TEST(AppUtilsTest, UserDataDirHasCompanyAndProduct) {
  for (bool is_privileged_process : {false, true}) {
    const base::FilePath dir = GetUserDataDir("app", is_privileged_process);

    EXPECT_EQ(dir.BaseName().value(), FILE_PATH_LITERAL("app"))
        << "is_privileged_process=" << is_privileged_process;
    EXPECT_EQ(dir.DirName().BaseName().value(),
              FILE_PATH_LITERAL("BraveSoftware"))
        << "is_privileged_process=" << is_privileged_process;
  }
}

// Privileged and unprivileged processes must resolve to different base paths
// on every platform; a swapped branch would make them collide.
TEST(AppUtilsTest, PrivilegedDiffersFromUser) {
  EXPECT_NE(GetUserDataDir("app", /*is_privileged_process=*/false),
            GetUserDataDir("app", /*is_privileged_process=*/true));
}

#if BUILDFLAG(IS_MAC)
// Bundled: walk up to the enclosing ".framework" ancestor.
TEST(AppUtilsTest, ResolveFrameworkBundledFindsAncestor) {
  const base::FilePath exe_dir(FILE_PATH_LITERAL(
      "/Apps/Brave.app/Contents/Frameworks/Brave Framework.framework/"
      "Versions/1/Helpers/Brave VPN Agent.app/Contents/MacOS"));

  EXPECT_EQ(
      ResolveFrameworkPath(exe_dir, /*am_i_bundled=*/true),
      base::FilePath(FILE_PATH_LITERAL(
          "/Apps/Brave.app/Contents/Frameworks/Brave Framework.framework")));
}

// Bundled but no ".framework" ancestor: returns empty and (crucially)
// terminates at the filesystem root instead of looping forever.
TEST(AppUtilsTest, ResolveFrameworkBundledNoFrameworkReturnsEmpty) {
  const base::FilePath exe_dir(
      FILE_PATH_LITERAL("/Apps/Standalone.app/Contents/MacOS"));

  EXPECT_TRUE(ResolveFrameworkPath(exe_dir, /*am_i_bundled=*/true).empty());
}

// Standalone: find the sibling ".framework", ignoring non-framework
// directories.
TEST(AppUtilsTest, ResolveFrameworkStandaloneFindsSibling) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());

  const base::FilePath framework =
      temp.GetPath().Append(FILE_PATH_LITERAL("Brave Framework.framework"));
  ASSERT_TRUE(base::CreateDirectory(framework));
  ASSERT_TRUE(base::CreateDirectory(
      temp.GetPath().Append(FILE_PATH_LITERAL("Resources"))));

  EXPECT_EQ(ResolveFrameworkPath(temp.GetPath(), /*am_i_bundled=*/false),
            framework);
}

// Standalone with no framework present: returns empty.
TEST(AppUtilsTest, ResolveFrameworkStandaloneNoneReturnsEmpty) {
  base::ScopedTempDir temp;
  ASSERT_TRUE(temp.CreateUniqueTempDir());

  EXPECT_TRUE(
      ResolveFrameworkPath(temp.GetPath(), /*am_i_bundled=*/false).empty());
}
#endif  // BUILDFLAG(IS_MAC)

}  // namespace brave_vpn::v2::app_utils
