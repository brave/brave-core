/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/side_panel/side_panel_utils.h"

#include "chrome/browser/ui/side_panel/side_panel_entry.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

TEST(SidePanelUtilsTest, ShouldShowSidePanelHeaderOnlyForBraveEntries) {
  EXPECT_TRUE(ShouldShowSidePanelHeader(SidePanelEntryId::kReadingList));
  EXPECT_TRUE(ShouldShowSidePanelHeader(SidePanelEntryId::kBookmarks));
  EXPECT_FALSE(ShouldShowSidePanelHeader(SidePanelEntryId::kCustomizeChrome));
}

}  // namespace brave
