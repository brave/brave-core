// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/screenshot/devtools_full_page_extractor.h"

#include <string>
#include <vector>

#include "base/base64.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace screenshot {

namespace {

base::DictValue MakeResponse(base::DictValue result_dict) {
  base::DictValue response;
  response.Set("result", std::move(result_dict));
  return response;
}

}  // namespace

TEST(ParseCdpScreenshotResponseTest, MissingResult_ReturnsError) {
  base::DictValue response;  // no "result" key
  auto result = ParseCdpScreenshotResponse(response);
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "No result in CDP response");
}

TEST(ParseCdpScreenshotResponseTest, MissingData_ReturnsError) {
  auto result = ParseCdpScreenshotResponse(MakeResponse(base::DictValue()));
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Missing data in CDP response");
}

TEST(ParseCdpScreenshotResponseTest, EmptyData_ReturnsError) {
  base::DictValue result_dict;
  result_dict.Set("data", "");
  auto result =
      ParseCdpScreenshotResponse(MakeResponse(std::move(result_dict)));
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Missing data in CDP response");
}

TEST(ParseCdpScreenshotResponseTest, InvalidBase64_ReturnsError) {
  base::DictValue result_dict;
  result_dict.Set("data", "not-valid-base64!!!");
  auto result =
      ParseCdpScreenshotResponse(MakeResponse(std::move(result_dict)));
  ASSERT_FALSE(result.has_value());
  EXPECT_EQ(result.error(), "Base64 decode failed");
}

TEST(ParseCdpScreenshotResponseTest, ValidBase64_ReturnsDecodedBytes) {
  const std::vector<uint8_t> expected_bytes = {0x89, 0x50, 0x4e, 0x47};
  std::string encoded = base::Base64Encode(expected_bytes);

  base::DictValue result_dict;
  result_dict.Set("data", encoded);
  auto result =
      ParseCdpScreenshotResponse(MakeResponse(std::move(result_dict)));
  ASSERT_TRUE(result.has_value());
  EXPECT_EQ(result.value(), expected_bytes);
}

}  // namespace screenshot
