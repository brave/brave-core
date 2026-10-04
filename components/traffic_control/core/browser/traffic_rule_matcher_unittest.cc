// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_rule_matcher.h"

#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "brave/components/traffic_control/core/mojom/traffic_control.mojom.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "url/gurl.h"

namespace traffic_control {

namespace {

mojom::TrafficRulePtr MakeRule(
    std::string_view id,
    bool enabled,
    std::optional<std::string> url_filter,
    std::optional<std::string> container_id = std::string("container-1")) {
  return mojom::TrafficRule::New(
      std::string(id), enabled, mojom::Condition::New(std::move(url_filter)),
      mojom::Target::New(std::move(container_id),
                         /*temporary_container=*/false));
}

}  // namespace

TEST(TrafficRuleMatcherTest, MatchesHostFilter) {
  TrafficRuleMatcher matcher;
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com"));
  matcher.Rebuild(std::move(rules));

  auto match = matcher.FindMatchingRule(GURL("https://example.com/path"));
  ASSERT_TRUE(match);
  EXPECT_EQ("r1", match->id);

  EXPECT_FALSE(matcher.FindMatchingRule(GURL("https://other.com/")));
}

TEST(TrafficRuleMatcherTest, MultilineAndComments) {
  TrafficRuleMatcher matcher;
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule(
      "r1", true, "# comment\n\nexample.com\n# another\nfoo.example.org"));
  matcher.Rebuild(std::move(rules));

  EXPECT_TRUE(matcher.FindMatchingRule(GURL("https://example.com/")));
  EXPECT_TRUE(matcher.FindMatchingRule(GURL("https://foo.example.org/")));
  EXPECT_FALSE(matcher.FindMatchingRule(GURL("https://unrelated.test/")));
}

TEST(TrafficRuleMatcherTest, ListOrderWins) {
  TrafficRuleMatcher matcher;
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("first", true, "example.com", "c1"));
  rules.push_back(MakeRule("second", true, "example.com", "c2"));
  matcher.Rebuild(std::move(rules));

  auto match = matcher.FindMatchingRule(GURL("https://www.example.com/"));
  ASSERT_TRUE(match);
  EXPECT_EQ("first", match->id);
}

TEST(TrafficRuleMatcherTest, DisabledRulesOmitted) {
  TrafficRuleMatcher matcher;
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("disabled", false, "example.com"));
  rules.push_back(MakeRule("enabled", true, "example.com"));
  matcher.Rebuild(std::move(rules));

  auto match = matcher.FindMatchingRule(GURL("https://example.com/"));
  ASSERT_TRUE(match);
  EXPECT_EQ("enabled", match->id);
}

TEST(TrafficRuleMatcherTest, UnsetFilterDoesNotMatch) {
  TrafficRuleMatcher matcher;
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("no-filter", true, std::nullopt));
  matcher.Rebuild(std::move(rules));

  EXPECT_FALSE(matcher.FindMatchingRule(GURL("https://example.com/")));
}

TEST(TrafficRuleMatcherTest, RebuildClearsPreviousRules) {
  TrafficRuleMatcher matcher;
  std::vector<mojom::TrafficRulePtr> rules;
  rules.push_back(MakeRule("r1", true, "example.com"));
  matcher.Rebuild(std::move(rules));
  EXPECT_TRUE(matcher.FindMatchingRule(GURL("https://example.com/")));

  rules.clear();
  rules.push_back(MakeRule("r2", true, "other.com"));
  matcher.Rebuild(std::move(rules));
  EXPECT_FALSE(matcher.FindMatchingRule(GURL("https://example.com/")));
  EXPECT_TRUE(matcher.FindMatchingRule(GURL("https://other.com/")));
}

}  // namespace traffic_control
