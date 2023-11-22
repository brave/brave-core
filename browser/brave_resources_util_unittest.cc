/*  Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stddef.h>

#include "brave/grit/brave_theme_resources.h"
#include "build/build_config.h"
#include "chrome/browser/resources_util.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_WIN)
#include "brave/common/resource_bundle_helper.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/grit/branded_strings.h"
#endif

TEST(BraveResourcesUtil, CheckIds) {
  const struct {
    const char* name;
    int id;
  } kCases[] = {
    // IDRs from brave/app/theme/brave_theme_resources.grd should be valid.
    {"IDR_PRODUCT_LOGO_32_BETA", IDR_PRODUCT_LOGO_32_BETA},
    {"IDR_PRODUCT_LOGO_32_DEV", IDR_PRODUCT_LOGO_32_DEV},
    {"IDR_PRODUCT_LOGO_32_CANARY", IDR_PRODUCT_LOGO_32_CANARY},
    {"IDR_PRODUCT_LOGO_32_DEVELOPMENT", IDR_PRODUCT_LOGO_32_DEVELOPMENT},
  };

  for (auto kCase : kCases)
    EXPECT_EQ(kCase.id, ResourcesUtil::GetThemeResourceId(kCase.name));
}

#if BUILDFLAG(IS_WIN)
TEST(BraveResourcesWinTest, CheckStringsForInstaller) {
  // Test whether strings for installer are filled.
  // This test is added because these strings are initially empty and filled
  // with brave's one.
  // brave::InitializeResourceBundle can only be called once per unittest
  // running, cannot be called for every test suites. Which will lead to
  // duplicate resources
  // brave::InitializeResourceBundle();

  EXPECT_FALSE(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_SXS_SHORTCUT_NAME)
          .empty());
  EXPECT_FALSE(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_SHORTCUT_NAME_BETA)
          .empty());
  EXPECT_FALSE(
      brave_l10n::GetLocalizedResourceUTF16String(IDS_SHORTCUT_NAME_DEV)
          .empty());
  EXPECT_FALSE(brave_l10n::GetLocalizedResourceUTF16String(
                   IDS_APP_SHORTCUTS_SUBDIR_NAME_BETA)
                   .empty());
  EXPECT_FALSE(brave_l10n::GetLocalizedResourceUTF16String(
                   IDS_APP_SHORTCUTS_SUBDIR_NAME_DEV)
                   .empty());
  EXPECT_FALSE(brave_l10n::GetLocalizedResourceUTF16String(
                   IDS_INBOUND_MDNS_RULE_NAME_BETA)
                   .empty());
  EXPECT_FALSE(brave_l10n::GetLocalizedResourceUTF16String(
                   IDS_INBOUND_MDNS_RULE_NAME_CANARY)
                   .empty());
  EXPECT_FALSE(brave_l10n::GetLocalizedResourceUTF16String(
                   IDS_INBOUND_MDNS_RULE_NAME_DEV)
                   .empty());
  EXPECT_FALSE(brave_l10n::GetLocalizedResourceUTF16String(
                   IDS_INBOUND_MDNS_RULE_DESCRIPTION_BETA)
                   .empty());
  EXPECT_FALSE(brave_l10n::GetLocalizedResourceUTF16String(
                   IDS_INBOUND_MDNS_RULE_DESCRIPTION_CANARY)
                   .empty());
  EXPECT_FALSE(brave_l10n::GetLocalizedResourceUTF16String(
                   IDS_INBOUND_MDNS_RULE_DESCRIPTION_DEV)
                   .empty());
}
#endif
