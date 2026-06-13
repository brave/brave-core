/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "components/history/core/browser/top_sites_constants.h"
#include "testing/gtest/include/gtest/gtest.h"

static_assert(BUILDFLAG(IS_ANDROID), "Android-only test");

// Verify Brave's Android override of kTopSitesNumber (15 tiles: 4x4 - 1 for
// the plus button). The non-Android value (12) is covered by
// BraveNewTabUITest.ConstantsTest in brave_new_tab_ui_unittest.cc.
TEST(BraveTopSitesConstantsTest, AndroidTopSitesNumberIsOverridden) {
  EXPECT_EQ(15u, history::kTopSitesNumber);
}
