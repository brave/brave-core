/* This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/common/chrome_constants.h"

#include <string>

#include "testing/gtest/include/gtest/gtest.h"

// Only checks prefix (Brave Browser) because test build don't update branding.
TEST(ChromeConstantsTest, ProductStringTest) {
  EXPECT_EQ(std::string(chrome::kFrameworkName).substr(0, 13),
            "Brave Browser");
}
