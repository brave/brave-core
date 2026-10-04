// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/tabs/tab_style.h"

#include "testing/gtest/include/gtest/gtest.h"

TEST(TabStyleTest, Get_ShouldNotLeak) {
  const TabStyle* tab_style = TabStyle::Get();
  EXPECT_NE(tab_style, nullptr);

  // Getting multiple times should return the same instance, instead of creating
  // a new instance.
  const TabStyle* tab_style2 = TabStyle::Get();
  EXPECT_EQ(tab_style, tab_style2);
}
