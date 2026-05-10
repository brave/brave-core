// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/query_filter/browser/schema_utils.h"

#include <string>
#include <vector>

#include "base/values.h"
#include "brave/components/query_filter/common/schema.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace query_filter {

namespace {

schema::Rule MakeRule(std::vector<std::string> include,
                      std::vector<std::string> exclude,
                      std::vector<std::string> params) {
  base::ListValue include_list;
  for (const auto& s : include) {
    include_list.Append(s);
  }
  base::ListValue exclude_list;
  for (const auto& s : exclude) {
    exclude_list.Append(s);
  }
  base::ListValue params_list;
  for (const auto& s : params) {
    params_list.Append(s);
  }

  base::DictValue dict;
  dict.Set("include", std::move(include_list));
  dict.Set("exclude", std::move(exclude_list));
  dict.Set("params", std::move(params_list));

  auto rule = schema::Rule::FromValue(base::Value(std::move(dict)));
  CHECK(rule.has_value());
  return std::move(*rule);
}

}  // namespace

TEST(SchemaUtilsTest, EmptyRulesReturnsEmptySet) {
  const std::vector<schema::Rule> rules;
  auto result =
      GetBlocklistedParamsForSpec(rules, "https://example.com/?gclid=123");
  EXPECT_TRUE(result.empty());
}

TEST(SchemaUtilsTest, GlobalWildcardIncludeMatchesAnyUrl) {
  std::vector<schema::Rule> rules;
  rules.push_back(MakeRule({"*://*/*"}, {}, {"gclid", "fbclid"}));

  auto result =
      GetBlocklistedParamsForSpec(rules, "https://example.com/?gclid=123");
  EXPECT_EQ(result.size(), 2u);
  EXPECT_TRUE(result.contains("gclid"));
  EXPECT_TRUE(result.contains("fbclid"));
}

TEST(SchemaUtilsTest, DomainSpecificIncludeMatchesOnlyThatDomain) {
  std::vector<schema::Rule> rules;
  rules.push_back(MakeRule({"*://*.instagram.com/*", "*://instagram.com/*"}, {},
                           {"igshid", "igsh"}));

  // Should match instagram.com
  auto result1 =
      GetBlocklistedParamsForSpec(rules, "https://www.instagram.com/p/abc/");
  EXPECT_TRUE(result1.contains("igshid"));
  EXPECT_TRUE(result1.contains("igsh"));

  // Should match bare instagram.com
  auto result2 =
      GetBlocklistedParamsForSpec(rules, "https://instagram.com/p/abc/");
  EXPECT_TRUE(result2.contains("igshid"));

  // Should NOT match an unrelated domain
  auto result3 =
      GetBlocklistedParamsForSpec(rules, "https://example.com/?igshid=123");
  EXPECT_TRUE(result3.empty());
}

TEST(SchemaUtilsTest, ExcludePatternSkipsMatchingUrl) {
  std::vector<schema::Rule> rules;
  rules.push_back(
      MakeRule({"*://*.facebook.com/*"}, {"*://*.safe.com/*"}, {"evil_param"}));

  // URL matching include but not exclude – param should be blocked.
  auto result1 = GetBlocklistedParamsForSpec(
      rules, "https://mobile.facebook.com/?evil_param=1");
  EXPECT_TRUE(result1.contains("evil_param"));

  // URL matching include AND exclude – rule should be skipped entirely.
  auto result2 =
      GetBlocklistedParamsForSpec(rules, "https://safe.com/?evil_param=1");
  EXPECT_TRUE(result2.empty());
}

TEST(SchemaUtilsTest, BlankIncludePatternsAreIgnored) {
  std::vector<schema::Rule> rules;
  // A rule with only blank/empty include strings should never match.
  rules.push_back(MakeRule({"", "   "}, {}, {"tracker"}));

  auto result =
      GetBlocklistedParamsForSpec(rules, "https://example.com/?tracker=1");
  EXPECT_TRUE(result.empty());
}

TEST(SchemaUtilsTest, MultipleMatchingRulesAggregateParams) {
  std::vector<schema::Rule> rules;
  rules.push_back(MakeRule({"*://*/*"}, {}, {"gclid", "fbclid"}));
  rules.push_back(
      MakeRule({"*://*.twitter.com/*", "*://twitter.com/*"}, {}, {"ref_src"}));

  // twitter.com matches both rules.
  auto result = GetBlocklistedParamsForSpec(
      rules, "https://twitter.com/?gclid=1&ref_src=2");
  EXPECT_TRUE(result.contains("gclid"));
  EXPECT_TRUE(result.contains("fbclid"));
  EXPECT_TRUE(result.contains("ref_src"));

  // example.com only matches the global rule.
  auto result2 =
      GetBlocklistedParamsForSpec(rules, "https://example.com/?ref_src=1");
  EXPECT_TRUE(result2.contains("gclid"));
  EXPECT_TRUE(result2.contains("fbclid"));
  EXPECT_FALSE(result2.contains("ref_src"));
}

TEST(SchemaUtilsTest, NonMatchingIncludeReturnsEmptySet) {
  std::vector<schema::Rule> rules;
  rules.push_back(
      MakeRule({"*://*.youtube.com/*", "*://youtube.com/*"}, {}, {"si"}));

  auto result =
      GetBlocklistedParamsForSpec(rules, "https://example.com/?si=abc");
  EXPECT_TRUE(result.empty());
}

TEST(SchemaUtilsTest, InvalidUrlSpecReturnsEmptySet) {
  std::vector<schema::Rule> rules;
  rules.push_back(MakeRule({"*://*/*"}, {}, {"gclid"}));

  // An empty/invalid spec should not crash and should return an empty set.
  auto result = GetBlocklistedParamsForSpec(rules, "");
  EXPECT_TRUE(result.empty());

  auto result2 = GetBlocklistedParamsForSpec(rules, "not a url");
  EXPECT_TRUE(result2.empty());
}

TEST(SchemaUtilsTest, SubdomainPatternDoesNotMatchRootDomain) {
  std::vector<schema::Rule> rules;
  // Pattern requires at least one subdomain.
  rules.push_back(MakeRule({"*://*.example.com/*"}, {}, {"tracker"}));

  // Subdomain should match.
  auto result1 =
      GetBlocklistedParamsForSpec(rules, "https://sub.example.com/?tracker=1");
  EXPECT_TRUE(result1.contains("tracker"));

  // Bare root domain should NOT match "*://*.example.com/*".
  auto result2 =
      GetBlocklistedParamsForSpec(rules, "https://example.com/?tracker=1");
  EXPECT_TRUE(result2.empty());
}

TEST(SchemaUtilsTest, ExcludePatternTakesPrecedenceOverInclude) {
  std::vector<schema::Rule> rules;
  // Both include and exclude cover example.com; exclude wins.
  rules.push_back(MakeRule({"*://*/*"}, {"*://example.com/*"}, {"gclid"}));

  auto result =
      GetBlocklistedParamsForSpec(rules, "https://example.com/?gclid=1");
  EXPECT_TRUE(result.empty());

  // A different domain is not excluded – param should be blocked.
  auto result2 =
      GetBlocklistedParamsForSpec(rules, "https://other.com/?gclid=1");
  EXPECT_TRUE(result2.contains("gclid"));
}

}  // namespace query_filter
