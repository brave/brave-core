/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ml/data/text_data.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::ml {

class BatAdsTextDataTest : public UnitTestBase {};

TEST_F(BatAdsTextDataTest, TextDataInitialization) {
  // Arrange
  const std::string expected_text = "expected text";
  const TextData text_data(expected_text);

  // Act
  const std::string& text = text_data.GetText();

  // Assert
  EXPECT_EQ(expected_text, text);
}

}  // namespace ads::ml
