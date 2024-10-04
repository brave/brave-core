/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/url_sanitizer/browser/url_sanitizer_service.h"

#include <vector>

#include "base/containers/flat_set.h"
#include "base/run_loop.h"
#include "base/test/task_environment.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace brave {

namespace {
constexpr char kTestPatterns[] = R"(
[
  {
    "include": [
      "*://*.twitter.com/*"
    ],
    "params": [
      "t"
    ]
  },
  {
    "include": [
      "*://*/*"
    ],
    "exclude": [
    ],
    "params": [
      "utm_content",
      "utm_affiliate"
    ]
  },
  {
    "include": [
      "https://dev-pages.bravesoftware.com/clean-urls/*"
    ],
    "exclude": [
      "https://dev-pages.bravesoftware.com/clean-urls/exempted/*"
    ],
    "params": [
      "brave_testing1",
      "brave_testing2"
    ]
  }
])";

}  // namespace

class URLSanitizerServiceUnitTest : public testing::Test,
                                    public URLSanitizerService {
 public:
  URLSanitizerServiceUnitTest()
      : task_environment_(base::test::TaskEnvironment::TimeSource::MOCK_TIME) {}

  void WaitInitialization(const std::string& json) {
    base::RunLoop loop;
    SetInitializationCallbackForTesting(loop.QuitClosure());
    brave::URLSanitizerComponentInstaller::RawConfig config;
    config.matchers = json;
    OnConfigReady(config);
    loop.Run();
  }

 private:
  base::test::TaskEnvironment task_environment_;
};

TEST_F(URLSanitizerServiceUnitTest, StripQueryParameter) {
  auto list = base::MakeFlatSet<std::string>(
      std::vector<std::string>{"fbclid", "second"});

  EXPECT_EQ(StripQueryParameter("fbclid=11&param1=1&second=2", list),
            "param1=1");
  EXPECT_EQ(StripQueryParameter(
                "fbclid=11&fbclid2=ok&&param1=1&foo;bar=yes&second=2", list),
            "fbclid2=ok&&param1=1&foo;bar=yes");
  EXPECT_EQ(
      StripQueryParameter(
          "fbclid=11&fbclid=11&fbclid=22&param1=1&second=2&second=2&second=2",
          list),
      "param1=1");
  EXPECT_EQ(StripQueryParameter("param1=1", list), "param1=1");
  EXPECT_EQ(StripQueryParameter("", list), "");
}

TEST_F(URLSanitizerServiceUnitTest, ClearURLS) {
  // The service has not yet been initialized.
  EXPECT_EQ(SanitizeURL(GURL("https://brave.com")), GURL("https:/brave.com"));
  WaitInitialization(kTestPatterns);

  EXPECT_EQ(
      SanitizeURL(
          GURL("https://dev-pages.bravesoftware.com/clean-urls/"
               "exempted/"
               "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&&;b&"
               "d&utm_content=removethis&e=&f=g&=end")),
      GURL("https://dev-pages.bravesoftware.com/clean-urls/"
           "exempted/?brave_testing1=foo&brave_testing2=bar&brave_"
           "testing3=keep&&;b&d&e=&f=g&=end"));
  EXPECT_EQ(
      SanitizeURL(GURL(
          "https://dev-pages.bravesoftware.com/clean-urls/"
          "?brave_testing1=foo&brave_testing2=bar&brave_testing3=keep&&;b&d&"
          "utm_content=removethis&e=&f=g&=end")),
      GURL("https://dev-pages.bravesoftware.com/clean-urls/"
           "?brave_testing3=keep&&;b&d&e=&f=g&=end"));

  WaitInitialization(R"([
    { "include": [ "*://*/*"], "params": ["query"] }
  ])");

  EXPECT_EQ(SanitizeURL(GURL("https://brave.com/?query=removethis")),
            GURL("https://brave.com/"));

  EXPECT_EQ(
      SanitizeURL(GURL("http://twitter.com/post/?query=removethis#ref=1")),
      GURL("http://twitter.com/post/#ref=1"));

  EXPECT_EQ(SanitizeURL(GURL("chrome://settings/?query=donotremovethis")),
            GURL("chrome://settings/?query=donotremovethis"));

  WaitInitialization(R"([
    { "include": [ "*://*.twitter.com/*"], "params": ["t"] }
  ])");

  EXPECT_EQ(
      SanitizeURL(
          GURL("https://twitter.com/post/?utm_content=removethis&e=&t=g&=end")),
      GURL("https://twitter.com/post/?utm_content=removethis&e=&=end"));

  EXPECT_EQ(
      SanitizeURL(GURL("https://subpage.twitter.com/post/"
                       "?utm_content=removethis&e=&t=g&=end")),
      GURL("https://subpage.twitter.com/post/?utm_content=removethis&e=&=end"));

  EXPECT_EQ(
      SanitizeURL(GURL("http://subpage.twitter.com/post/"
                       "?utm_content=removethis&e=&t=g&=end")),
      GURL("http://subpage.twitter.com/post/?utm_content=removethis&e=&=end"));

  EXPECT_EQ(
      SanitizeURL(GURL("file:///home/copy-clean-link.html?utm_source=web")),
      GURL("file:///home/copy-clean-link.html?utm_source=web"));

  EXPECT_EQ(SanitizeURL(GURL("data:text/html,foo?utm_source=web")),
            GURL("data:text/html,foo?utm_source=web"));

  EXPECT_EQ(SanitizeURL(GURL("ws://localhost:8080/?utm_source=web")),
            GURL("ws://localhost:8080/?utm_source=web"));
}

}  // namespace brave
