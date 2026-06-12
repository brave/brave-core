/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/side_panel/side_panel_entry.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

TEST(SidePanelEntryTest, DefaultContentWidthIs400) {
  EXPECT_EQ(400, SidePanelEntry::kSidePanelDefaultContentWidth);
}

}  // namespace brave
