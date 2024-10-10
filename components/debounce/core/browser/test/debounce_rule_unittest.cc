// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/debounce/core/browser/debounce_rule.h"

#include "base/json/json_reader.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace debounce {

// Helper methods
std::vector<std::unique_ptr<DebounceRule>> StringToRules(std::string contents) {
  auto parsed = DebounceRule::ParseRules(contents);
  EXPECT_TRUE(parsed.has_value());
  return std::move(parsed.value().first);
}

void CheckApplyResult(DebounceRule* rule,
                      GURL original_url,
                      std::string expected_url,
                      TestingPrefServiceSimple* prefs,
                      bool expected_error) {
  GURL final_url;
  EXPECT_EQ(!expected_error, rule->Apply(original_url, &final_url, prefs));
  EXPECT_EQ(expected_url, final_url.spec());
}

void CheckApplyResult(DebounceRule* rule,
                      GURL original_url,
                      std::string expected_url,
                      bool expected_error) {
  TestingPrefServiceSimple prefs;
  CheckApplyResult(rule, original_url, expected_url, &prefs, expected_error);
}

TEST(DebounceRuleUnitTest, DebounceActionChecking) {
  DebounceAction field = kDebounceNoAction;
  EXPECT_TRUE(DebounceRule::ParseDebounceAction("regex-path", &field));
  EXPECT_EQ(kDebounceRegexPath, field);
  EXPECT_TRUE(DebounceRule::ParseDebounceAction("base64,redirect", &field));
  EXPECT_EQ(kDebounceBase64DecodeAndRedirectToParam, field);
  EXPECT_TRUE(DebounceRule::ParseDebounceAction("redirect", &field));
  EXPECT_EQ(kDebounceRedirectToParam, field);
  EXPECT_FALSE(DebounceRule::ParseDebounceAction("abc", &field));
}

// Note: we use json as the delimiter for the raw-string-literals for the test
// JSON blobs in these test cases because we have the default )" in the regexes
TEST(DebounceRuleUnitTest, CheckBaseCase) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [],
          "action": "regex-path",
          "param": "^/(.*)$"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(),
                     GURL("https://test.com/https://brave.com/test/abc.jpg"),
                     "https://brave.com/test/abc.jpg", false);
  }
}

TEST(DebounceRuleUnitTest, MalformedParam) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "param": "())"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://test.com/https://brave.com"), "",
                     true);
  }
}

TEST(DebounceRuleUnitTest, ParamCapturesNoStrings) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "param": "brave.com"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://test.com/https://brave.com"), "",
                     true);
  }
}

TEST(DebounceRuleUnitTest, ParamCapturesMoreThanOneString) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [],
          "action": "regex-path",
          "param": "(brave).(com)"
      }]
      )json";

  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://test.com/https://brave.com"), "",
                     true);
  }
}

TEST(DebounceRuleUnitTest, ParamCapturesNonURLNoPrependScheme) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "param": "^/(.*)$"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://test.com/brave.com"), "", true);
  }
}

TEST(DebounceRuleUnitTest, ParamCapturesNonURLWithPrependScheme) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "prepend_scheme": "http",
          "param": "^/(.*)$"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://test.com/brave.com"),
                     "http://brave.com/", false);
  }
}

TEST(DebounceRuleUnitTest, TwoCaptureGroups) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "prepend_scheme": "https",
          "param": "^/([^/]+)/xyz(/.*)$"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://test.com/brave.com/xyz/abc.jpg"),
                     "https://brave.com/abc.jpg", false);
  }
}

TEST(DebounceRuleUnitTest, CurlyBracesInRegexGetParseError) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "prepend_scheme": "https",
          "param": "^/turbo/([^/]+)/xyz(/\d{4})/xyzzy(/.*)$"
      }]
      )json";

  auto parsed = DebounceRule::ParseRules(contents);
  EXPECT_FALSE(parsed.has_value());
}

TEST(DebounceRuleUnitTest, ThreeCaptureGroups) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "prepend_scheme": "https",
          "param": "^/turbo/([^/]+)/xyz(/[0-9]+)/xyzzy(/.*)$"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(
        rule.get(),
        GURL("https://test.com/turbo/brave.com/xyz/2022/xyzzy/abc.jpg"),
        "https://brave.com/2022/abc.jpg", false);
  }
}

TEST(DebounceRuleUnitTest, ParamCapturesURLWithPrependScheme) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "prepend_scheme": "http",
          "param": "^/(.*)$"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://test.com/https://brave.com"), "",
                     true);
  }
}

TEST(DebounceRuleUnitTest, IncorrectPrependScheme) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "prepend_scheme": "wss",
          "param": "(.*)"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://test.com/brave.com"), "", true);
  }
}

TEST(DebounceRuleUnitTest, PrefToggle) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterBooleanPref("brave.de_amp.enabled", false);
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "pref": "brave.de_amp.enabled",
          "param": "^/(.*)$"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://test.com/brave.com"), "", &prefs,
                     true);
  }
  prefs.SetBoolean("brave.de_amp.enabled", true);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://test.com/http://brave.com"),
                     "http://brave.com/", &prefs, false);
  }
}

TEST(DebounceRuleUnitTest, PrefDoesNotExist) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://test.com/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "pref": "brave.de_amp.enabled",
          "param": "^/(.*)$"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://test.com/brave.com"), "", true);
  }
}

TEST(DebounceRuleUnitTest, ArbitrarySubdomainInInclude) {
  TestingPrefServiceSimple prefs;
  prefs.registry()->RegisterBooleanPref("brave.de_amp.enabled", true);

  const std::string contents = R"json(
      [{
          "include": [
              "*://*.cdn.ampproject.org/c/s/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "prepend_scheme": "https",
          "pref": "brave.de_amp.enabled",
          "param": "^/c/s/(.*)$"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(),
                     GURL("https://www-theverge-com.cdn.ampproject.org/c/s/"
                          "www.theverge.com/platform/amp/2018/9/20/17881766/"
                          "bing-google-amp-support-mobile-news"),
                     "https://www.theverge.com/platform/amp/2018/9/20/17881766/"
                     "bing-google-amp-support-mobile-news",
                     &prefs, false);
  }
}

TEST(DebounceRuleUnitTest, UrlEncodedDestinationUrl) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://prf.hn/click/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "param":
          "^/click/camref:[0-9a-zA-Z]*/pubref:[0-9a-zA-Z-]*/destination:(.*)$"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  // From https://github.com/brave/brave-browser/issues/22429
  auto* tracker_url =
      "https://prf.hn/click/camref:1011l7xH5/"
      "pubref:cn-182c2c19daa548769cf89ef10d5a5af3-dtp/"
      "destination:https%3A%2F%2Fwww.test.com%2Ftest1%2Ftest2";
  auto* target_url = "https://www.test.com/test1/test2";

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL(tracker_url), target_url, false);
  }
}

TEST(DebounceRuleUnitTest, CatchMultiplePathsForSameInclude) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://*.anrdoezrs.net/links/*"
          ],
          "exclude": [
          ],
          "action": "regex-path",
          "param": "^/links/[0-9]*/type/dlg/sid/[-_a-zA-Z]*/(.*)$"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  // From https://github.com/brave/brave-browser/issues/22429
  auto* tracker_url_1 =
      "https://www.anrdoezrs.net/links/123/type/dlg/sid/---/https://"
      "www.carhartt.com/product/123/nintendo-one";
  auto* tracker_url_2 =
      "https://www.anrdoezrs.net/links/123/type/dlg/sid/"
      "cn-___COM_CLICK_ID___-dtp/https://www.verizon.com/deals";
  auto* target_url_1 = "https://www.carhartt.com/product/123/nintendo-one";
  auto* target_url_2 = "https://www.verizon.com/deals";
  auto* tracker_url_not_match =
      "https://www.anrdoezrs.net/links/123/type/dlg/sid/https://"
      "www.carhartt.com/product/123/nintendo-one";

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL(tracker_url_1), target_url_1, false);
    CheckApplyResult(rule.get(), GURL(tracker_url_2), target_url_2, false);
    CheckApplyResult(rule.get(), GURL(tracker_url_not_match), "", true);
  }
}

}  // namespace debounce
