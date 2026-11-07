/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "build/build_config.h"
#include "components/history/core/browser/top_sites_constants.h"
#include "components/ntp_tiles/constants.h"
#include "testing/gtest/include/gtest/gtest.h"

#if BUILDFLAG(IS_ANDROID)

// Verify Brave's Android override of kTopSitesNumber (15 tiles: 4x4 - 1 for
// the plus button). The non-Android value (12) is covered by
// DesktopConstantsTest.
TEST(BraveTopSitesConstantsTest, AndroidTopSitesNumberIsOverridden) {
  EXPECT_EQ(15u, history::kTopSitesNumber);
}

#else

TEST(BraveTopSitesConstantsTest, DesktopConstantsTest) {
  // Make sure history/ntp_tiles module has proper constants for our NTP
  // requirements.
  constexpr size_t kBraveMaxNumMostVisited = 12;
  constexpr size_t kBraveMaxNumCustomLinks = 120;
  constexpr size_t kTopSitesNumber = history::kTopSitesNumber;

  EXPECT_EQ(kBraveMaxNumMostVisited, kTopSitesNumber);
  EXPECT_EQ(kBraveMaxNumCustomLinks, ntp_tiles::kMaxNumCustomLinks);
  EXPECT_EQ(kBraveMaxNumMostVisited, ntp_tiles::kMaxNumMostVisited);
  EXPECT_EQ(static_cast<int>(kBraveMaxNumMostVisited), ntp_tiles::kMaxNumTiles);
}

#endif  // BUILDFLAG(IS_ANDROID)
