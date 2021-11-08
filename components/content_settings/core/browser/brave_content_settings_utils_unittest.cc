/*  Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"
#include "chrome/test/base/testing_profile.h"
#include "components/content_settings/core/common/content_settings_pattern.h"
#include "content/public/test/browser_task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

using content_settings::ConvertPatternToWildcardSchemeAndPort;

class BraveContentSettingsUtilsTest : public testing::Test {
 public:
  BraveContentSettingsUtilsTest() = default;
  BraveContentSettingsUtilsTest(const BraveContentSettingsUtilsTest&) = delete;
  BraveContentSettingsUtilsTest& operator=(
      const BraveContentSettingsUtilsTest&) = delete;
  ~BraveContentSettingsUtilsTest() override = default;

  void SetUp() override { profile_ = std::make_unique<TestingProfile>(); }

  TestingProfile* profile() { return profile_.get(); }

 private:
  content::BrowserTaskEnvironment task_environment_;
  std::unique_ptr<TestingProfile> profile_;
};

TEST_F(BraveContentSettingsUtilsTest,
       TestConvertPatternToWildcardSchemeAndPort) {
  // Full wildcard pattern.
  EXPECT_EQ(absl::nullopt, ConvertPatternToWildcardSchemeAndPort(
                               ContentSettingsPattern::Wildcard()));

  // Brave first party placeholder pattern.
  EXPECT_EQ(absl::nullopt,
            ConvertPatternToWildcardSchemeAndPort(
                ContentSettingsPattern::FromString("https://firstParty/*")));

  // file:// scheme pattern.
  EXPECT_EQ(absl::nullopt,
            ConvertPatternToWildcardSchemeAndPort(
                ContentSettingsPattern::FromString("file:///a/b/c.zip")));

  // Wildcard host pattern.
  EXPECT_EQ(absl::nullopt,
            ConvertPatternToWildcardSchemeAndPort(
                ContentSettingsPattern::FromString("http://*:8080/*")));

  // Wildcard scheme, no port.
  EXPECT_EQ(absl::nullopt,
            ConvertPatternToWildcardSchemeAndPort(
                ContentSettingsPattern::FromString("*://brave.com/*")));
  EXPECT_EQ(absl::nullopt,
            ConvertPatternToWildcardSchemeAndPort(
                ContentSettingsPattern::FromString("*://brave.com:*/")));

  // Wildcard scheme, has port.
  auto pattern = ConvertPatternToWildcardSchemeAndPort(
      ContentSettingsPattern::FromString("*://brave.com:8080/*"));
  EXPECT_NE(absl::nullopt, pattern);
  EXPECT_EQ(pattern->ToString(), "brave.com");
  EXPECT_TRUE(pattern->Matches(GURL("http://brave.com:80/path1")));
  EXPECT_TRUE(pattern->Matches(GURL("https://brave.com/path2")));
  EXPECT_FALSE(pattern->Matches(GURL("http://brave2.com:8080")));
  pattern.reset();

  // Scheme, no port.
  pattern = ConvertPatternToWildcardSchemeAndPort(
      ContentSettingsPattern::FromString("http://brave.com/"));
  EXPECT_NE(absl::nullopt, pattern);
  EXPECT_EQ(pattern->ToString(), "brave.com");
  EXPECT_TRUE(pattern->Matches(GURL("ftp://brave.com:80/path1")));
  EXPECT_TRUE(pattern->Matches(GURL("https://brave.com/path2")));
  EXPECT_FALSE(pattern->Matches(GURL("http://brave2.com:8080")));
  pattern.reset();

  // Scheme and port.
  pattern = ConvertPatternToWildcardSchemeAndPort(
      ContentSettingsPattern::FromString("https://brave.com:56558/"));
  EXPECT_NE(absl::nullopt, pattern);
  EXPECT_EQ(pattern->ToString(), "brave.com");
  EXPECT_TRUE(pattern->Matches(GURL("wss://brave.com:80/path1")));
  EXPECT_TRUE(pattern->Matches(GURL("https://brave.com/path2")));
  EXPECT_FALSE(pattern->Matches(GURL("http://brave2.com:8080")));
}
