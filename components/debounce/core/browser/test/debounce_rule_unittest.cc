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
  EXPECT_TRUE(DebounceRule::ParseDebounceAction("regex-path-template", &field));
  EXPECT_EQ(kDebounceRegexPathTemplate, field);
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

// Test case where the extracted URL has query params but no path.
// See: https://github.com/brave/brave-browser/issues/52168
TEST(DebounceRuleUnitTest, QueryStringWithoutPath) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://www.example.com/*"
          ],
          "exclude": [],
          "action": "redirect",
          "param": "url"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  auto* tracker_url =
      "https://www.example.com/8005?url=https%3A%2F%2Ftarget.com%3Futm_"
      "source%3Dsource1%26utm_medium%3Dcpc%26utm_content%3DContent1%26utm_"
      "campaign%3DCampaign1%2B%2528Test%2BCampaign%2529&referrer=&"
      "widget=&instance=";
  auto* target_url =
      "https://target.com/?utm_source=source1&utm_medium=cpc&utm_content="
      "Content1&utm_campaign=Campaign1+%28Test+Campaign%29";

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL(tracker_url), target_url, false);
  }
}

// Test that debouncing is rejected when the destination URL lacks a valid
// eTLD+1 (e.g., single-part hostname like "foo")
// From https://github.com/brave/brave-browser/issues/23580
TEST(DebounceRuleUnitTest, RejectUrlsWithoutValidEtldPlusOne) {
  // Simulate AMP debouncing rule that prepends scheme
  const std::string contents = R"json(
      [{
          "include": [
              "*://*.ampproject.org/c/s/*"
          ],
          "exclude": [],
          "action": "regex-path",
          "prepend_scheme": "https",
          "param": "^/c/s/(.*)$"
      }]
      )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    // Single-part hostname "foo" should be rejected,
    // no valid eTLD+1.
    CheckApplyResult(rule.get(),
                     GURL("https://theguardian.ampproject.org/c/s/foo"), "",
                     true);

    // Valid eTLD+1 should still work
    CheckApplyResult(rule.get(),
                     GURL("https://example.ampproject.org/c/s/brave.com"),
                     "https://brave.com/", false);
  }
}

// Test redirect_url_template with a single capture group (y2u.be-like case).
TEST(DebounceRuleUnitTest, RedirectUrlBasic) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://y2u.be/*"
          ],
          "exclude": [],
          "action": "regex-path-template",
          "param": "^/(.+)$",
          "redirect_url_template": "https://www.youtube.com/watch?v=$1"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://y2u.be/dQw4w9WgXcQ"),
                     "https://www.youtube.com/watch?v=dQw4w9WgXcQ", false);
  }
}

// Test redirect_url_template with multiple capture groups.
TEST(DebounceRuleUnitTest, RedirectUrlMultipleCaptures) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://tracker.example.com/*"
          ],
          "exclude": [],
          "action": "regex-path-template",
          "param": "^/([^/]+)/([^/]+)$",
          "redirect_url_template": "https://$1.example.org/page/$2"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(),
                     GURL("https://tracker.example.com/www/landing"),
                     "https://www.example.org/page/landing", false);
  }
}

// Test redirect_url_template producing same eTLD+1 as original (should fail).
TEST(DebounceRuleUnitTest, RedirectUrlSameSite) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://redir.example.com/*"
          ],
          "exclude": [],
          "action": "regex-path-template",
          "param": "^/(.+)$",
          "redirect_url_template": "https://www.example.com/page/$1"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://redir.example.com/foo"), "",
                     true);
  }
}

// Test redirect_url_template when regex doesn't match the path.
TEST(DebounceRuleUnitTest, RedirectUrlNoMatch) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://y2u.be/*"
          ],
          "exclude": [],
          "action": "regex-path-template",
          "param": "^/video/(.+)$",
          "redirect_url_template": "https://www.youtube.com/watch?v=$1"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    // Path "/dQw4w9WgXcQ" doesn't match "^/video/(.+)$".
    CheckApplyResult(rule.get(), GURL("https://y2u.be/dQw4w9WgXcQ"), "", true);
  }
}

// Test redirect_url_template with more captures than placeholders (fail).
TEST(DebounceRuleUnitTest, RedirectUrlExtraCapturesRejected) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://tracker.example.com/*"
          ],
          "exclude": [],
          "action": "regex-path-template",
          "param": "^/([^/]+)/([^/]+)/([^/]+)$",
          "redirect_url_template": "https://$1.example.org/"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    // Three captures but only $1 in template -- mismatch.
    CheckApplyResult(rule.get(),
                     GURL("https://tracker.example.com/www/foo/bar"), "", true);
  }
}

// Test redirect_url_template referencing $2 when regex only has one capture
// group. Unresolved placeholders mean a misconfigured rule, so no match.
TEST(DebounceRuleUnitTest, RedirectUrlUnresolvedPlaceholder) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://tracker.example.com/*"
          ],
          "exclude": [],
          "action": "regex-path-template",
          "param": "^/(.+)$",
          "redirect_url_template": "https://www.example.org/$1/$2"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://tracker.example.com/foo"), "",
                     true);
  }
}

// Test redirect_url_template with no placeholders but a capturing regex (should
// fail). A static template with captures is a misconfigured rule.
TEST(DebounceRuleUnitTest, RedirectUrlStaticTemplateWithCaptures) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://tracker.example.com/*"
          ],
          "exclude": [],
          "action": "regex-path-template",
          "param": "^/(.+)$",
          "redirect_url_template": "https://www.example.org/landing"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(), GURL("https://tracker.example.com/anything"),
                     "", true);
  }
}

// Test redirect_url_template with >9 capture groups (should fail).
TEST(DebounceRuleUnitTest, RedirectUrlTooManyCaptureGroups) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://tracker.example.com/*"
          ],
          "exclude": [],
          "action": "regex-path-template",
          "param": "^/(.)(.)(.)(.)(.)(.)(.)(.)(.)(.+)$",
          "redirect_url_template": "https://example.org/$1"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    // 10 capture groups exceeds the $1..$9 limit.
    CheckApplyResult(rule.get(), GURL("https://tracker.example.com/abcdefghij"),
                     "", true);
  }
}

// Test redirect_url_template with non-contiguous placeholders (e.g., $1 and $3
// with only 2 capture groups). The set sizes match but the indices don't, so a
// simple size comparison would incorrectly pass.
TEST(DebounceRuleUnitTest, RedirectUrlNonContiguousPlaceholders) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://tracker.example.com/*"
          ],
          "exclude": [],
          "action": "regex-path-template",
          "param": "^/([^/]+)/([^/]+)$",
          "redirect_url_template": "https://$1.example.org/$3"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    CheckApplyResult(rule.get(),
                     GURL("https://tracker.example.com/www/landing"), "", true);
  }
}

// Test that regex-path action ignores redirect_url_template (wrong action).
// Rule should use existing concatenation path, not template substitution.
TEST(DebounceRuleUnitTest, RedirectUrlTemplateIgnoredForRegexPath) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://y2u.be/*"
          ],
          "exclude": [],
          "action": "regex-path",
          "param": "^/(.+)$",
          "redirect_url_template": "https://www.youtube.com/watch?v=$1"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    // regex-path concatenates captures as a raw URL, so the result is just the
    // captured path segment (dQw4w9WgXcQ) treated as a URL -- which is invalid
    // and should be rejected by downstream URL validation.
    CheckApplyResult(rule.get(), GURL("https://y2u.be/dQw4w9WgXcQ"), "", true);
  }
}

// Test that regex-path with prepend_scheme is unaffected by a stray
// redirect_url_template field. The AMP-style rule should still work normally.
TEST(DebounceRuleUnitTest, RegexPathPrependSchemeIgnoresTemplate) {
  const std::string contents = R"json(
      [{
          "include": [
              "*://*.ampproject.org/c/s/*"
          ],
          "exclude": [],
          "action": "regex-path",
          "prepend_scheme": "https",
          "param": "^/c/s/(.*)$",
          "redirect_url_template": "https://www.youtube.com/watch?v=$1"
      }]
    )json";
  std::vector<std::unique_ptr<DebounceRule>> rules = StringToRules(contents);

  for (const std::unique_ptr<DebounceRule>& rule : rules) {
    // Should redirect to the AMP target, not the template URL.
    CheckApplyResult(
        rule.get(),
        GURL("https://example.ampproject.org/c/s/www.example.com/article"),
        "https://www.example.com/article", false);
  }
}

}  // namespace debounce
