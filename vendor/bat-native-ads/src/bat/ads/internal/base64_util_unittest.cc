/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base64_util.h"

#include <cstdint>

#include "base/base64.h"
#include "testing/gtest/include/gtest/gtest.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

TEST(BatAdsBase64UtilTest, Base64ToBytesForEmptyString) {
  // Arrange
  const std::string text = "";
  const std::vector<uint8_t> text_as_bytes(text.begin(), text.end());
  const std::string text_as_base64 = base::Base64Encode(text_as_bytes);

  // Act
  const std::vector<uint8_t> bytes = Base64ToBytes(text_as_base64);

  // Assert
  EXPECT_EQ(text_as_bytes, bytes);
}

TEST(BatAdsBase64UtilTest, Base64ToBytes) {
  // Arrange
  const std::string text = "The quick brown fox jumps over 13 lazy dogs.";
  const std::vector<uint8_t> text_as_bytes(text.begin(), text.end());
  const std::string text_as_base64 = base::Base64Encode(text_as_bytes);

  // Act
  const std::vector<uint8_t> bytes = Base64ToBytes(text_as_base64);

  // Assert
  EXPECT_EQ(text_as_bytes, bytes);
}

}  // namespace ads
