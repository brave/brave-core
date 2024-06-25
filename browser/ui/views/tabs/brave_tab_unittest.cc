// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab.h"
#include "chrome/browser/ui/views/tabs/fake_tab_slot_controller.h"
#include "chrome/test/views/chrome_views_test_base.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/views/test/views_test_utils.h"

class BraveTabTest : public ChromeViewsTestBase {
 public:
  BraveTabTest() = default;
  ~BraveTabTest() override = default;

  void LayoutAndCheckBorder(BraveTab* tab, const gfx::Rect& bounds) {
    tab->SetBoundsRect(bounds);
    views::test::RunScheduledLayout(tab);

    auto insets = tab->tab_style_views()->GetContentsInsets();
    int left_inset = insets.left();
    left_inset += BraveTab::kExtraLeftPadding;
    EXPECT_EQ(left_inset, tab->GetInsets().left());
  }
};

TEST_F(BraveTabTest, ExtraPaddingLayoutTest) {
  FakeTabSlotController tab_slot_controller;
  BraveTab tab(&tab_slot_controller);

  // Our tab should have extra padding always.
  // See the comment at BraveTab::GetInsets().
  LayoutAndCheckBorder(&tab, {0, 0, 30, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 50, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 100, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 150, 50});
  LayoutAndCheckBorder(&tab, {0, 0, 30, 50});
}
