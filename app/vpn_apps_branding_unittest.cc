/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/files/file_path.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_vpn/app/v2/shared/app_utils.h"
#include "build/build_config.h"
#include "chrome/common/chrome_paths_internal.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_WIN)
#include "chrome/install_static/install_details.h"
#include "chrome/install_static/user_data_dir.h"
#endif  // BUILDFLAG(IS_WIN)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)

namespace {
base::FilePath GetDefaultUserDataDirectory() {
  base::FilePath user_data_dir;
#if BUILDFLAG(IS_WIN)
  const install_static::InstallConstants kFakeInstallConstants = {
      sizeof(install_static::InstallConstants), 0, "", L"", L"", L"", L""};
  std::wstring result, invalid;
  install_static::GetUserDataDirectoryImpl(L"", kFakeInstallConstants, &result,
                                           &invalid);
  user_data_dir = base::FilePath(result);
#else   // BUILDFLAG(IS_WIN)
  chrome::GetDefaultUserDataDirectory(&user_data_dir);
#endif  // BUILDFLAG(IS_WIN)
  return user_data_dir;
}

// Returns the index of the last path component whose name starts with
// |prefix|. Scanning from the end (rather than the front) keeps the
// search anchored to the product/leaf directory and avoids matching an
// unrelated leading component that happens to share the prefix.
// It also transparently skips the trailing "User Data" component that
// Chrome appends to the user data dir.
std::optional<size_t> FindLastComponentIndexWithPrefix(
    const std::vector<base::FilePath::StringType>& components,
    std::string_view prefix) {
  for (size_t i = components.size(); i > 0; --i) {
    if (base::StartsWith(base::FilePath(components[i - 1]).AsUTF8Unsafe(),
                         prefix, base::CompareCase::SENSITIVE)) {
      return i - 1;
    }
  }
  return std::nullopt;
}

// Extracts the "company" segment of a "<...>/<company>/<product>" data
// directory -- i.e. the directory immediately preceding the component that
// starts with |product_prefix|. For example, given
// ".../BraveSoftware/Brave-Browser" and the prefix "Brave-Browser", this
// returns "BraveSoftware". Returns std::nullopt if no matching product
// component exists, or if nothing precedes it.
std::optional<std::string> GetCompanyName(const base::FilePath& path,
                                          std::string_view product_prefix) {
  const std::vector<base::FilePath::StringType> components =
      path.GetComponents();
  const std::optional<size_t> product_index =
      FindLastComponentIndexWithPrefix(components, product_prefix);
  if (!product_index.has_value() || *product_index == 0) {
    return std::nullopt;
  }
  return base::FilePath(components[*product_index - 1]).AsUTF8Unsafe();
}
}  // namespace

TEST(VpnAppsBrandingTest, UnprivilegedDataDirMatchesChromeDataDir) {
  base::FilePath chrome_user_data_dir = GetDefaultUserDataDirectory();
  ASSERT_FALSE(chrome_user_data_dir.empty());

  const base::FilePath vpn_unprivileged_data_dir =
      brave_vpn::v2::app_utils::GetUserDataDir("app",
                                               /*is_privileged_process=*/false);
  ASSERT_FALSE(vpn_unprivileged_data_dir.empty());

  const auto chrome_components = chrome_user_data_dir.GetComponents();
  const auto vpn_components = vpn_unprivileged_data_dir.GetComponents();

  // Locate the product directory in each path: the "Brave-*" directory
  // for the browser and the "app" directory for the VPN. Everything before it
  // is the shared "<...>/BraveSoftware" prefix we want to compare.
  const std::string browser_product_name_prefix = "Brave-";
  const std::optional<size_t> chrome_product_index =
      FindLastComponentIndexWithPrefix(chrome_components,
                                       browser_product_name_prefix);
  ASSERT_TRUE(chrome_product_index.has_value())
      << "No '" << browser_product_name_prefix << "' directory found in "
      << chrome_user_data_dir;

  const std::optional<size_t> vpn_app_index =
      FindLastComponentIndexWithPrefix(vpn_components, "app");
  ASSERT_TRUE(vpn_app_index.has_value())
      << "No 'app' directory found in " << vpn_unprivileged_data_dir;

  // The prefixes must be at the same depth and identical component-by-component
  // (comparing components avoids platform-specific path reconstruction quirks).
  ASSERT_EQ(*chrome_product_index, *vpn_app_index)
      << "Data dirs are nested at different depths:\n  " << chrome_user_data_dir
      << "\n  " << vpn_unprivileged_data_dir;
  for (size_t i = 0; i < *chrome_product_index; ++i) {
    EXPECT_EQ(base::FilePath(chrome_components[i]).AsUTF8Unsafe(),
              base::FilePath(vpn_components[i]).AsUTF8Unsafe())
        << "Path prefix differs at component " << i
        << " between Chrome's user data dir (" << chrome_user_data_dir
        << ") and the VPN data dir (" << vpn_unprivileged_data_dir << ")";
  }
}

TEST(VpnAppsBrandingTest, PrivilegedDataDirHasCorrectCompanyName) {
  base::FilePath chrome_user_data_dir = GetDefaultUserDataDirectory();
  ASSERT_FALSE(chrome_user_data_dir.empty());

  // Extract the company name from chrome_user_data_dir, e.g. the
  // "BraveSoftware" part of ".../BraveSoftware/Brave-BrowserBeta".
  const std::string browser_product_name_prefix = "Brave-";
  const std::optional<std::string> chrome_company_name =
      GetCompanyName(chrome_user_data_dir, browser_product_name_prefix);
  ASSERT_TRUE(chrome_company_name.has_value())
      << "Could not extract a company name from " << chrome_user_data_dir;

  const base::FilePath vpn_privileged_data_dir =
      brave_vpn::v2::app_utils::GetUserDataDir("app",
                                               /*is_privileged_process=*/true);
  ASSERT_FALSE(vpn_privileged_data_dir.empty());

  // Extract the company name from vpn_privileged_data_dir, e.g. the
  // "BraveSoftware" part of ".../BraveSoftware/app".
  const std::optional<std::string> vpn_company_name =
      GetCompanyName(vpn_privileged_data_dir, "app");
  ASSERT_TRUE(vpn_company_name.has_value())
      << "Could not extract a company name from " << vpn_privileged_data_dir;

  // The privileged VPN process must store its data under the same company
  // directory as the browser.
  EXPECT_EQ(*chrome_company_name, *vpn_company_name);
}

#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_LINUX)
