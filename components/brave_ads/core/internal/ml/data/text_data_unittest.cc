/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/ml/data/text_data.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads::ml {

class BraveAdsTextDataTest : public test::TestBase {};

TEST_F(BraveAdsTextDataTest, TextDataInitialization) {
  // Arrange
  const std::string text = "The quick brown fox jumps over the lazy dog";

  // Act
  const TextData text_data(text);

  // Assert
  EXPECT_EQ(text, text_data.GetText());
}

}  // namespace brave_ads::ml
