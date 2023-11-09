/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ui/views/controls/menu/menu_config.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/views/test/views_test_base.h"

namespace views {

using MenuConfigTest = ViewsTestBase;

TEST_F(MenuConfigTest, ChangedValueTest) {
  EXPECT_EQ(4, views::MenuConfig::instance().item_vertical_margin);
  EXPECT_EQ(16, views::MenuConfig::instance().item_horizontal_padding);
}

}  // namespace views
