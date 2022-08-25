/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/base/strings/string_html_parse_util.h"

#include <string>
#include <vector>

#include "bat/ads/internal/base/unittest/unittest_base.h"

// npm run test -- brave_unit_tests --filter=BatAds*

namespace ads {

class BatAdsStringHtmlUtilTest : public UnitTestBase {
 protected:
  BatAdsStringHtmlUtilTest() = default;

  ~BatAdsStringHtmlUtilTest() override = default;
};

TEST_F(BatAdsStringHtmlUtilTest, ParseTagAttributeSimple) {
  // Arrange
  const std::string html_1 =
      "<meta property=\"og:title\" description=\"a detailed summary\" "
      "content=\"this is info \">";
  const std::string html_2 =
      "<div href=\"brave.com\" description=\"this is12 34 info\">";
  const std::vector<std::vector<std::string>> samples = {
      {html_1, "og:title", "content", "this is info "},
      {html_1, "title", "content", "this is info "},
      {html_1, "description", "content", "this is info "},
      {html_1, "descript", "description", "a detailed summary"},
      {html_1, "og:description", "description", ""},
      {html_2, "og:title", "content", ""},
      {html_2, "title", "content", ""},
      {html_2, "description", "content", ""},
      {html_2, "href", "description", "this is12 34 info"},
      {html_2, "div", "href", "brave.com"}};

  for (const auto& sample : samples) {
    // Act
    const std::string parsed =
        ParseTagAttribute(sample[0], sample[1], sample[2]);
    // Assert
    ASSERT_EQ(sample[3], parsed);
  }
}

}  // namespace ads
