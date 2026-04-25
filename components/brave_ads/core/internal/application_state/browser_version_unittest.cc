/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/application_state/browser_version.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BraveAdsBrowserVersion*

namespace brave_ads {

class BraveAdsBrowserVersionTest : public test::TestBase {};

TEST_F(BraveAdsBrowserVersionTest, GetNumberAfterSetNumber) {
  // Arrange
  fake_browser_version_.SetNumber("1.33.7.42");

  // Act & Assert
  EXPECT_EQ("1.33.7.42", BrowserVersion::GetInstance().GetNumber());
}

}  // namespace brave_ads
