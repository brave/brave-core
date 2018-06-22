/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/resources_util.h"

#include <stddef.h>

#include "base/macros.h"
#include "brave/grit/brave_theme_resources.h"
#include "build/build_config.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveResourcesUtil, CheckIds) {
  const struct {
    const char* name;
    int id;
  } kCases[] = {
    // IDRs from brave/app/theme/brave_theme_resources.grd should be valid.
    {"IDR_PRODUCT_LOGO_32_BETA", IDR_PRODUCT_LOGO_32_BETA},
    {"IDR_PRODUCT_LOGO_32_DEV", IDR_PRODUCT_LOGO_32_DEV},
    {"IDR_PRODUCT_LOGO_32_CANARY", IDR_PRODUCT_LOGO_32_CANARY},
  };

  for (size_t i = 0; i < arraysize(kCases); ++i)
    EXPECT_EQ(kCases[i].id, ResourcesUtil::GetThemeResourceId(kCases[i].name));
}
