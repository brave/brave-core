/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/privacy_guard.h"

#include "brave/components/web_discovery/browser/patterns.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

class WebDiscoveryPrivacyGuardTest : public testing::Test {
 public:
  ~WebDiscoveryPrivacyGuardTest() override = default;

  // testing::Test:
  void SetUp() override {
    search_engine_pattern_.is_search_engine = true;
    search_engine_pattern_.search_template_prefix = "find?testquery=";
  }

 protected:
  PatternsURLDetails search_engine_pattern_;
};

TEST_F(WebDiscoveryPrivacyGuardTest, IsPrivateURLLikely) {
  EXPECT_FALSE(
      IsPrivateURLLikely(GURL("https://www.search1.com/search?q=test"), true));
  EXPECT_FALSE(IsPrivateURLLikely(

      GURL("https://search2.com/search?query=testing+a+nice+query"), true));
  EXPECT_FALSE(IsPrivateURLLikely(

      GURL(
          "https://search2.com/search?query=quick+brown+fox+jumped&country=us"),
      true));
  EXPECT_FALSE(
      IsPrivateURLLikely(GURL("https://www.website.com/page/test"), false));

  EXPECT_TRUE(
      IsPrivateURLLikely(GURL("http://www.website.com/page/test"), false));
  EXPECT_TRUE(IsPrivateURLLikely(GURL("https://88.88.88.88/page/test"), false));
  EXPECT_TRUE(
      IsPrivateURLLikely(GURL("https://website.com:8443/page/test"), false));
  EXPECT_TRUE(IsPrivateURLLikely(
      GURL("https://user:pass@website.com/page/test"), false));
  EXPECT_TRUE(IsPrivateURLLikely(
      GURL("https://www.search1.com/search?q=test#ABCDEFGHIJK"), true));

  EXPECT_TRUE(IsPrivateURLLikely(
      GURL("https://a.nested.sub.domain.website.co.uk/test/page"), true));
  EXPECT_TRUE(IsPrivateURLLikely(
      GURL("https://abc192738284732929abc.com/test/page"), true));
  EXPECT_TRUE(IsPrivateURLLikely(
      GURL("https://a-long-hyphenated-web-site.com/test/page"), true));
}

TEST_F(WebDiscoveryPrivacyGuardTest, IsPrivateQueryLikely) {
  EXPECT_FALSE(IsPrivateQueryLikely("test"));
  EXPECT_FALSE(IsPrivateQueryLikely("99 cake recipes"));
  EXPECT_FALSE(IsPrivateQueryLikely("grapefruit and pineapple"));
  EXPECT_FALSE(IsPrivateQueryLikely("a quick brown fox"));

  EXPECT_TRUE(IsPrivateQueryLikely(

      "ABC123ABC123ABC123ABC123ABC123ABC123ABC123ABC123ABC123ABC123"));
  EXPECT_TRUE(IsPrivateQueryLikely(

      "a long query that is potentially private and should not be considered"));
  EXPECT_TRUE(IsPrivateQueryLikely("aliases for me@testemail.com"));
  EXPECT_TRUE(IsPrivateQueryLikely("access site with user:pass@site.com"));
  EXPECT_TRUE(IsPrivateQueryLikely("php $P$MArzfx58u"));
  EXPECT_TRUE(
      IsPrivateQueryLikely("Hippopotomonstrosesquippedaliophobia symptoms"));
}

TEST_F(WebDiscoveryPrivacyGuardTest, GeneratePrivateSearchURL) {
  GURL original_url("https://example.com/search?q=aaa&country=us&f=1");

  EXPECT_EQ(
      GeneratePrivateSearchURL(original_url, "a simple test query",
                               search_engine_pattern_.search_template_prefix)
          .spec(),
      "https://example.com/find?testquery=a+simple+test+query");
  EXPECT_EQ(GeneratePrivateSearchURL(
                original_url, "another simple test query 123", std::nullopt)
                .spec(),
            "https://example.com/search?q=another+simple+test+query+123");
  EXPECT_EQ(
      GeneratePrivateSearchURL(original_url, "special chars @#$%^&=",
                               search_engine_pattern_.search_template_prefix)
          .spec(),
      "https://example.com/find?testquery=special+chars+%40%23%24%25%5E%26%3D");
}

TEST_F(WebDiscoveryPrivacyGuardTest, ShouldMaskURL) {
  EXPECT_FALSE(ShouldMaskURL(GURL("https://www.search1.com/search?q=test")));
  EXPECT_FALSE(ShouldMaskURL(

      GURL("https://search2.com/search?query=testing+a+nice+query")));
  EXPECT_FALSE(ShouldMaskURL(

      GURL("https://search2.com/search?query=quick+fox&country=us&d=1")));
  EXPECT_FALSE(ShouldMaskURL(GURL("https://www.website.com/page/test")));

  EXPECT_TRUE(ShouldMaskURL(

      GURL("https://www.website.com/page/test?id=12823871923991")));
  EXPECT_TRUE(
      ShouldMaskURL(GURL("https://www.website.com/page/test1283192831292")));
  EXPECT_TRUE(
      ShouldMaskURL(GURL("https://www.website.com/page/1283192831292?q=1")));
  EXPECT_TRUE(ShouldMaskURL(

      GURL("https://www.website.com/page/test?a=1&b=2&c=3&d=4&e=5")));
  EXPECT_TRUE(ShouldMaskURL(

      GURL("https://www.website.com/page/"
           "test?query=a+super+long+query+string+that+is+too+long")));
  EXPECT_TRUE(
      ShouldMaskURL(GURL("https://www.website.com/page/ayLxezLhK1Lh1H1")));
  EXPECT_TRUE(ShouldMaskURL(GURL("https://www.website.com/page/WebLogic")));
  EXPECT_TRUE(ShouldMaskURL(GURL("https://www.website.com/page/admin")));
  EXPECT_TRUE(ShouldMaskURL(GURL("https://www.website.com/page/edit/")));
  EXPECT_TRUE(ShouldMaskURL(GURL("https://www.website.com/page/doc?share=1")));
  EXPECT_TRUE(ShouldMaskURL(GURL("https://www.website.com/page/doc?user=abc")));
  EXPECT_TRUE(ShouldMaskURL(GURL("https://www.website.com/page/doc#logout")));
  EXPECT_TRUE(
      ShouldMaskURL(GURL("https://www.website.com/page/doc?password=abc")));
  EXPECT_TRUE(
      ShouldMaskURL(GURL("https://user:pass@www.website.com/page/test")));
  EXPECT_TRUE(
      ShouldMaskURL(GURL("https://www.website.com/page/test?e=test@test.com")));
}

TEST_F(WebDiscoveryPrivacyGuardTest, MaskURL) {
  GURL url("https://www.website.com/page/test");
  auto masked_url = MaskURL(url);
  ASSERT_TRUE(masked_url);
  EXPECT_EQ(*masked_url, url);

  masked_url = MaskURL(GURL("https://www.website.com/page/admin"));
  ASSERT_TRUE(masked_url);
  EXPECT_EQ(*masked_url, "https://www.website.com/ (PROTECTED)");

  EXPECT_FALSE(MaskURL(GURL("file:///etc")));
}

}  // namespace web_discovery
