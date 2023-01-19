/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/version_number_user_data.h"

#include <string>

#include "base/strings/stringprintf.h"
#include "base/test/values_test_util.h"
#include "bat/ads/internal/browser/browser_util.h"
#include "testing/gtest/include/gtest/gtest.h"  // IWYU pragma: keep

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads::user_data {

TEST(BatAdsVersionNumberUserDataTest, GetVersionNumber) {
  // Arrange

  // Act
  const base::Value::Dict user_data = GetVersionNumber();

  // Assert
  const std::string expected_json = base::StringPrintf(
      R"({"versionNumber":"%s"})", GetBrowserVersionNumber().c_str());

  const base::Value expected_user_data = base::test::ParseJson(expected_json);
  ASSERT_TRUE(expected_user_data.is_dict());

  EXPECT_EQ(expected_user_data, user_data);
}

}  // namespace ads::user_data
