/* Copyright (c) 2018 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_origin/buildflags/buildflags.h"
#include "chrome/common/chrome_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

// Only checks prefix (Brave Browser/Brave Origin) because test build don't
// update branding.
TEST(ChromeConstantsTest, ProductStringTest) {
#if BUILDFLAG(IS_BRAVE_ORIGIN_BRANDED)
  EXPECT_EQ(std::string(chrome::kFrameworkName).substr(0, 12), "Brave Origin");
#else
  EXPECT_EQ(std::string(chrome::kFrameworkName).substr(0, 13),
            "Brave Browser");
#endif
}
