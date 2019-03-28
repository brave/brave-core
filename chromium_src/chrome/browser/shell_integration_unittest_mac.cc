/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/shell_integration.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace shell_integration {

TEST(BraveShellIntegrationTest, MakeDefaultBrowserTestOnMac) {
  EXPECT_EQ(SET_DEFAULT_UNATTENDED, GetDefaultWebClientSetPermission());
}

}  // namespace shell_integration
