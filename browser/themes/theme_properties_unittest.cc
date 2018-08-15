/* This Source Code Form is subject to the terms of the Mozilla Public
+ * License, v. 2.0. If a copy of the MPL was not distributed with this file,
+ * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/themes/theme_properties.h"
#include "chrome/browser/themes/theme_properties.h"
#include "testing/gtest/include/gtest/gtest.h"

TEST(BraveThemeTest, ObtainsBraveOverrideColors) {
  SkColor actualColor = ThemeProperties::GetDefaultColor(BRAVE_COLOR_FOR_TEST, false);
  SkColor expectedColor = SkColorSetRGB(11, 13, 17);
  ASSERT_EQ(actualColor, expectedColor);
}
