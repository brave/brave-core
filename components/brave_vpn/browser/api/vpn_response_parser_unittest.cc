/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/api/vpn_response_parser.h"

#include "testing/gtest/include/gtest/gtest.h"

namespace brave_vpn {

TEST(VpnResponseParserUnitTest, ParseSubscriberCredentialFromJson) {
  std::string error_value;
  EXPECT_TRUE(brave_vpn::ParseSubscriberCredentialFromJson(
                  R"({
    "error-message":
      "The provided token either does not exist or is no longer valid",
    "error-title": "Token No Longer Valid"
    })",
                  &error_value)
                  .empty());
  EXPECT_EQ(error_value, "Token No Longer Valid");
  error_value.clear();
  EXPECT_TRUE(error_value.empty());
  EXPECT_EQ(brave_vpn::ParseSubscriberCredentialFromJson(
                R"({"subscriber-credential":"test"})", &error_value),
            "test");
  EXPECT_TRUE(error_value.empty());
  EXPECT_TRUE(
      brave_vpn::ParseSubscriberCredentialFromJson(R"([])", &error_value)
          .empty());
  EXPECT_TRUE(error_value.empty());
  EXPECT_TRUE(
      brave_vpn::ParseSubscriberCredentialFromJson(R"({,})", &error_value)
          .empty());
  EXPECT_TRUE(error_value.empty());
  EXPECT_TRUE(
      brave_vpn::ParseSubscriberCredentialFromJson("", &error_value).empty());
  EXPECT_TRUE(error_value.empty());
}

}  // namespace brave_vpn
