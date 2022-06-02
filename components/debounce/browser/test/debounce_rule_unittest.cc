/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/debounce/browser/debounce_rule.h"
#include "base/json/json_reader.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/googletest/src/googletest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace debounce {

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

std::vector<std::unique_ptr<DebounceRule>> StringToRules(std::string contents) {
  absl::optional<base::Value> root = base::JSONReader::Read(contents);
  std::vector<std::unique_ptr<DebounceRule>> rules;
  base::flat_set<std::string> host_cache;
  DebounceRule::ParseRules(std::move(root->GetList()), &rules, &host_cache);
  return rules;
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

}  // namespace debounce
