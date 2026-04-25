/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/patterns.h"

#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "brave/components/constants/brave_paths.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/re2/src/re2/re2.h"

namespace web_discovery {

class WebDiscoveryPatternsTest : public testing::Test {
 public:
  ~WebDiscoveryPatternsTest() override = default;

 protected:
  std::unique_ptr<PatternsGroup> LoadAndParsePatterns() {
    base::FilePath data_path =
        base::PathService::CheckedGet(brave::DIR_TEST_DATA);

    std::string patterns_json;
    bool read_result = base::ReadFileToString(
        data_path.AppendASCII("web_discovery/patterns.json"), &patterns_json);
    EXPECT_TRUE(read_result);

    auto parsed_patterns = ParsePatterns(patterns_json);
    EXPECT_TRUE(parsed_patterns);
    EXPECT_EQ(parsed_patterns->normal_patterns.size(), 3u);
    EXPECT_EQ(parsed_patterns->strict_patterns.size(), 3u);

    return parsed_patterns;
  }
};

TEST_F(WebDiscoveryPatternsTest, GroupURLDetails) {
  auto parsed_patterns = LoadAndParsePatterns();
  ASSERT_TRUE(parsed_patterns);

  const auto* normal_pattern = &parsed_patterns->normal_patterns[0];
  EXPECT_EQ(normal_pattern->id, "ex1");
  EXPECT_EQ(normal_pattern->url_regex->pattern(), "^https://example1.com");
  EXPECT_TRUE(normal_pattern->is_search_engine);
  EXPECT_FALSE(normal_pattern->search_template_prefix);
  EXPECT_TRUE(normal_pattern->payload_rule_groups.empty());
  EXPECT_EQ(normal_pattern->scrape_rule_groups.size(), 1u);

  normal_pattern = &parsed_patterns->normal_patterns[1];
  EXPECT_EQ(normal_pattern->id, "nase");
  EXPECT_EQ(normal_pattern->url_regex->pattern(),
            "^https://notasearchengine.biz");
  EXPECT_FALSE(normal_pattern->is_search_engine);
  EXPECT_FALSE(normal_pattern->search_template_prefix);
  EXPECT_FALSE(normal_pattern->payload_rule_groups.empty());
  EXPECT_EQ(normal_pattern->scrape_rule_groups.size(), 2u);

  normal_pattern = &parsed_patterns->normal_patterns[2];
  EXPECT_EQ(normal_pattern->id, "ex2");
  EXPECT_EQ(normal_pattern->url_regex->pattern(),
            "^https://search.example2.com");
  EXPECT_TRUE(normal_pattern->is_search_engine);
  EXPECT_FALSE(normal_pattern->search_template_prefix);
  EXPECT_TRUE(normal_pattern->payload_rule_groups.empty());
  EXPECT_EQ(normal_pattern->scrape_rule_groups.size(), 1u);

  const auto* strict_pattern = &parsed_patterns->strict_patterns[0];
  EXPECT_EQ(strict_pattern->id, "ex1");
  EXPECT_EQ(strict_pattern->url_regex->pattern(), "^https://example1.com.");
  EXPECT_TRUE(strict_pattern->is_search_engine);
  EXPECT_EQ(strict_pattern->search_template_prefix, "search?query=");
  EXPECT_EQ(strict_pattern->payload_rule_groups.size(), 1u);
  EXPECT_EQ(strict_pattern->scrape_rule_groups.size(), 3u);

  strict_pattern = &parsed_patterns->strict_patterns[1];
  EXPECT_EQ(strict_pattern->id, "nase");
  EXPECT_EQ(strict_pattern->url_regex->pattern(),
            "^https://notasearchengine.biz");
  EXPECT_FALSE(strict_pattern->is_search_engine);
  EXPECT_EQ(strict_pattern->search_template_prefix, "directory?query=");
  EXPECT_EQ(strict_pattern->payload_rule_groups.size(), 1u);
  EXPECT_EQ(strict_pattern->scrape_rule_groups.size(), 2u);

  strict_pattern = &parsed_patterns->strict_patterns[2];
  EXPECT_EQ(strict_pattern->id, "ex2");
  EXPECT_EQ(strict_pattern->url_regex->pattern(),
            "^https://search.example2.com");
  EXPECT_TRUE(strict_pattern->is_search_engine);
  EXPECT_FALSE(strict_pattern->search_template_prefix);
  EXPECT_EQ(strict_pattern->payload_rule_groups.size(), 1u);
  EXPECT_EQ(strict_pattern->scrape_rule_groups.size(), 1u);
}

TEST_F(WebDiscoveryPatternsTest, NormalScrapeRules) {
  auto parsed_patterns = LoadAndParsePatterns();
  ASSERT_TRUE(parsed_patterns);
  const auto* normal_pattern = &parsed_patterns->normal_patterns[0];
  EXPECT_EQ(normal_pattern->scrape_rule_groups.size(), 1u);

  auto rule_group_it = normal_pattern->scrape_rule_groups.find("form .search");
  ASSERT_TRUE(rule_group_it != normal_pattern->scrape_rule_groups.end());
  const auto* rule_group = &rule_group_it->second;
  EXPECT_EQ(rule_group->size(), 1u);

  auto rule_it = rule_group->find("q");
  ASSERT_TRUE(rule_it != rule_group->end());
  const auto* rule = rule_it->second.get();

  EXPECT_EQ(rule->sub_selector, "input");
  EXPECT_EQ(rule->rule_type, ScrapeRuleType::kSearchQuery);
  EXPECT_EQ(rule->attribute, "value");
  EXPECT_TRUE(rule->functions_applied.empty());

  normal_pattern = &parsed_patterns->normal_patterns[1];
  EXPECT_EQ(normal_pattern->scrape_rule_groups.size(), 2u);

  rule_group_it = normal_pattern->scrape_rule_groups.find(".field1 input");
  ASSERT_TRUE(rule_group_it != normal_pattern->scrape_rule_groups.end());
  rule_group = &rule_group_it->second;
  EXPECT_EQ(rule_group->size(), 1u);

  rule_it = rule_group->find("t");
  ASSERT_TRUE(rule_it != rule_group->end());
  rule = rule_it->second.get();

  EXPECT_FALSE(rule->sub_selector);
  EXPECT_EQ(rule->rule_type, ScrapeRuleType::kOther);
  EXPECT_EQ(rule->attribute, "href");
  EXPECT_TRUE(rule->functions_applied.empty());

  rule_group_it = normal_pattern->scrape_rule_groups.find(".field2 input");
  ASSERT_TRUE(rule_group_it != normal_pattern->scrape_rule_groups.end());
  rule_group = &rule_group_it->second;
  EXPECT_EQ(rule_group->size(), 2u);

  rule_it = rule_group->find("t");
  ASSERT_TRUE(rule_it != rule_group->end());
  rule = rule_it->second.get();

  EXPECT_FALSE(rule->sub_selector);
  EXPECT_EQ(rule->rule_type, ScrapeRuleType::kOther);
  EXPECT_EQ(rule->attribute, "href");
  EXPECT_EQ(rule->functions_applied.size(), 1u);
  EXPECT_EQ(rule->functions_applied[0].size(), 3u);
  EXPECT_EQ(rule->functions_applied[0][0], "parseU");
  EXPECT_EQ(rule->functions_applied[0][1], "qs");
  EXPECT_EQ(rule->functions_applied[0][2], "t");

  rule_it = rule_group->find("b");
  ASSERT_TRUE(rule_it != rule_group->end());
  rule = rule_it->second.get();

  EXPECT_FALSE(rule->sub_selector);
  EXPECT_EQ(rule->rule_type, ScrapeRuleType::kOther);
  EXPECT_EQ(rule->attribute, "textContent");
  EXPECT_TRUE(rule->functions_applied.empty());

  normal_pattern = &parsed_patterns->normal_patterns[2];
  EXPECT_EQ(normal_pattern->scrape_rule_groups.size(), 1u);

  rule_group_it = normal_pattern->scrape_rule_groups.find("form .search-box");
  ASSERT_TRUE(rule_group_it != normal_pattern->scrape_rule_groups.end());
  rule_group = &rule_group_it->second;
  EXPECT_EQ(rule_group->size(), 1u);

  rule_it = rule_group->find("q");
  ASSERT_TRUE(rule_it != rule_group->end());
  rule = rule_it->second.get();

  EXPECT_EQ(rule->sub_selector, "input");
  EXPECT_EQ(rule->rule_type, ScrapeRuleType::kSearchQuery);
  EXPECT_EQ(rule->attribute, "value");
  EXPECT_TRUE(rule->functions_applied.empty());
}

TEST_F(WebDiscoveryPatternsTest, StrictScrapeRules) {
  auto parsed_patterns = LoadAndParsePatterns();
  ASSERT_TRUE(parsed_patterns);
  const auto* strict_pattern = &parsed_patterns->strict_patterns[0];
  EXPECT_EQ(strict_pattern->scrape_rule_groups.size(), 3u);

  auto rule_group_it = strict_pattern->scrape_rule_groups.find("form .search");
  ASSERT_TRUE(rule_group_it != strict_pattern->scrape_rule_groups.end());
  const auto* rule_group = &rule_group_it->second;
  EXPECT_EQ(rule_group->size(), 1u);

  auto rule_it = rule_group->find("q");
  ASSERT_TRUE(rule_it != rule_group->end());
  const auto* rule = rule_it->second.get();

  EXPECT_EQ(rule->sub_selector, "input");
  EXPECT_EQ(rule->rule_type, ScrapeRuleType::kSearchQuery);
  EXPECT_EQ(rule->attribute, "value");
  EXPECT_TRUE(rule->functions_applied.empty());

  rule_group_it = strict_pattern->scrape_rule_groups.find("qurl");
  ASSERT_TRUE(rule_group_it != strict_pattern->scrape_rule_groups.end());
  rule_group = &rule_group_it->second;
  EXPECT_EQ(rule_group->size(), 1u);

  rule_it = rule_group->find("qurl");
  ASSERT_TRUE(rule_it != rule_group->end());
  rule = rule_it->second.get();

  EXPECT_FALSE(rule->sub_selector);
  EXPECT_EQ(rule->rule_type, ScrapeRuleType::kStandard);
  EXPECT_EQ(rule->attribute, "url");
  EXPECT_EQ(rule->functions_applied.size(), 1u);
  EXPECT_EQ(rule->functions_applied[0].size(), 3u);
  EXPECT_EQ(rule->functions_applied[0][0], "maskU");
  EXPECT_EQ(rule->functions_applied[0][1], false);
  EXPECT_EQ(rule->functions_applied[0][2], false);

  rule_group_it = strict_pattern->scrape_rule_groups.find("ctry");
  ASSERT_TRUE(rule_group_it != strict_pattern->scrape_rule_groups.end());
  rule_group = &rule_group_it->second;
  EXPECT_EQ(rule_group->size(), 1u);

  rule_it = rule_group->find("ctry");
  ASSERT_TRUE(rule_it != rule_group->end());
  rule = rule_it->second.get();

  EXPECT_FALSE(rule->sub_selector);
  EXPECT_EQ(rule->rule_type, ScrapeRuleType::kStandard);
  EXPECT_EQ(rule->attribute, "ctry");
  EXPECT_TRUE(rule->functions_applied.empty());

  strict_pattern = &parsed_patterns->strict_patterns[1];
  EXPECT_EQ(strict_pattern->scrape_rule_groups.size(), 2u);

  rule_group_it = strict_pattern->scrape_rule_groups.find("#content .a1");
  ASSERT_TRUE(rule_group_it != strict_pattern->scrape_rule_groups.end());
  rule_group = &rule_group_it->second;
  EXPECT_EQ(rule_group->size(), 1u);

  rule_it = rule_group->find("age");
  ASSERT_TRUE(rule_it != rule_group->end());
  rule = rule_it->second.get();

  EXPECT_FALSE(rule->sub_selector);
  EXPECT_EQ(rule->rule_type, ScrapeRuleType::kOther);
  EXPECT_EQ(rule->attribute, "textContent");
  EXPECT_TRUE(rule->functions_applied.empty());

  strict_pattern = &parsed_patterns->strict_patterns[2];
  EXPECT_EQ(strict_pattern->scrape_rule_groups.size(), 1u);

  rule_group_it = strict_pattern->scrape_rule_groups.find("form .search-box");
  ASSERT_TRUE(rule_group_it != strict_pattern->scrape_rule_groups.end());
  rule_group = &rule_group_it->second;
  EXPECT_EQ(rule_group->size(), 1u);

  rule_it = rule_group->find("age");
  ASSERT_TRUE(rule_it != rule_group->end());
  rule = rule_it->second.get();

  EXPECT_EQ(rule->sub_selector, ".created-at");
  EXPECT_EQ(rule->rule_type, ScrapeRuleType::kOther);
  EXPECT_EQ(rule->attribute, "value");
  EXPECT_TRUE(rule->functions_applied.empty());
}

TEST_F(WebDiscoveryPatternsTest, NormalPayloadRules) {
  auto parsed_patterns = LoadAndParsePatterns();
  ASSERT_TRUE(parsed_patterns);

  const auto* normal_pattern = &parsed_patterns->normal_patterns[0];
  EXPECT_TRUE(normal_pattern->payload_rule_groups.empty());
  normal_pattern = &parsed_patterns->normal_patterns[2];
  EXPECT_TRUE(normal_pattern->payload_rule_groups.empty());

  normal_pattern = &parsed_patterns->normal_patterns[1];
  EXPECT_EQ(normal_pattern->payload_rule_groups.size(), 1u);

  const auto& payload_group_it = normal_pattern->payload_rule_groups.front();
  EXPECT_EQ(payload_group_it.key, "key1");
  EXPECT_EQ(payload_group_it.rule_type, PayloadRuleType::kSingle);
  EXPECT_EQ(payload_group_it.result_type, PayloadResultType::kSingle);
  EXPECT_EQ(payload_group_it.action, "t");
  EXPECT_TRUE(payload_group_it.rules.empty());
}

TEST_F(WebDiscoveryPatternsTest, StrictPayloadRules) {
  auto parsed_patterns = LoadAndParsePatterns();
  ASSERT_TRUE(parsed_patterns);

  const auto* strict_pattern = &parsed_patterns->strict_patterns[0];
  EXPECT_EQ(strict_pattern->payload_rule_groups.size(), 1u);

  const auto* payload_group = &strict_pattern->payload_rule_groups[0];
  EXPECT_EQ(payload_group->key, "key1");
  EXPECT_EQ(payload_group->rule_type, PayloadRuleType::kSingle);
  EXPECT_EQ(payload_group->result_type, PayloadResultType::kSingle);
  EXPECT_EQ(payload_group->action, "query");
  EXPECT_TRUE(payload_group->rules.empty());

  strict_pattern = &parsed_patterns->strict_patterns[1];
  EXPECT_EQ(strict_pattern->payload_rule_groups.size(), 1u);

  payload_group = &strict_pattern->payload_rule_groups[0];
  EXPECT_EQ(payload_group->key, "key1");
  EXPECT_EQ(payload_group->rule_type, PayloadRuleType::kQuery);
  EXPECT_EQ(payload_group->result_type, PayloadResultType::kClustered);
  EXPECT_EQ(payload_group->action, "age-info");
  EXPECT_EQ(payload_group->rules.size(), 2u);

  const auto* payload_rule = &payload_group->rules[0];
  EXPECT_EQ(payload_rule->selector, "#content .a1");
  EXPECT_EQ(payload_rule->key, "age");
  EXPECT_FALSE(payload_rule->is_join);

  payload_rule = &payload_group->rules[1];
  EXPECT_EQ(payload_rule->selector, "ctry");
  EXPECT_EQ(payload_rule->key, "ctry");
  EXPECT_FALSE(payload_rule->is_join);

  strict_pattern = &parsed_patterns->strict_patterns[2];
  EXPECT_EQ(strict_pattern->payload_rule_groups.size(), 1u);

  payload_group = &strict_pattern->payload_rule_groups[0];
  EXPECT_EQ(payload_group->key, "key2");
  EXPECT_EQ(payload_group->rule_type, PayloadRuleType::kQuery);
  EXPECT_EQ(payload_group->result_type, PayloadResultType::kClustered);
  EXPECT_EQ(payload_group->action, "age-info");
  EXPECT_EQ(payload_group->rules.size(), 1u);

  payload_rule = &payload_group->rules[0];
  EXPECT_EQ(payload_rule->selector, "form .search-box");
  EXPECT_EQ(payload_rule->key, "age");
  EXPECT_TRUE(payload_rule->is_join);
}

TEST_F(WebDiscoveryPatternsTest, BadPatterns) {
  ASSERT_FALSE(ParsePatterns("ABC"));
  ASSERT_FALSE(ParsePatterns("{}"));
  ASSERT_FALSE(ParsePatterns(R"({"normal":{}, "strict":{}})"));
}

}  // namespace web_discovery
