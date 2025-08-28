/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/patterns_v2.h"

#include <algorithm>
#include <memory>
#include <string>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/path_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace web_discovery {

class WebDiscoveryPatternsV2Test : public testing::Test {
 public:
  WebDiscoveryPatternsV2Test() = default;

 protected:
  void SetUp() override {
    base::FilePath data_path =
        base::PathService::CheckedGet(base::DIR_SRC_TEST_DATA_ROOT);

    std::string patterns_json;
    bool read_result = base::ReadFileToString(
        data_path.AppendASCII("brave/test/data/web_discovery/patterns-v2.json"),
        &patterns_json);
    ASSERT_TRUE(read_result);

    patterns_ = ParseV2Patterns(patterns_json);
    ASSERT_TRUE(patterns_);
  }

  std::unique_ptr<V2PatternsGroup> patterns_;
};

TEST_F(WebDiscoveryPatternsV2Test, BasicParsing) {
  // Check that we have the expected site patterns
  EXPECT_EQ(patterns_->site_patterns.size(), 2u);

  // Check search-bi pattern exists
  auto search_bi_it = patterns_->site_patterns.find(RelevantSite::kBing);
  EXPECT_TRUE(search_bi_it != patterns_->site_patterns.end());

  // Check search-bii pattern exists
  auto search_bii_it = patterns_->site_patterns.find(RelevantSite::kBingImages);
  EXPECT_TRUE(search_bii_it != patterns_->site_patterns.end());
}

TEST_F(WebDiscoveryPatternsV2Test, InputGroups) {
  auto search_bi_it = patterns_->site_patterns.find(RelevantSite::kBing);
  ASSERT_TRUE(search_bi_it != patterns_->site_patterns.end());

  const auto& search_bi_pattern = search_bi_it->second;

  // Check input groups
  EXPECT_EQ(search_bi_pattern.input_groups.size(), 3u);

  // Check #b .result1 input group (all)
  auto result1_it = search_bi_pattern.input_groups.find("#b .result1");
  ASSERT_TRUE(result1_it != search_bi_pattern.input_groups.end());
  EXPECT_TRUE(result1_it->second.select_all);
  EXPECT_EQ(result1_it->second.extraction_rules.size(), 2u);

  // Validate "t" extraction rule
  auto t_rule_it = result1_it->second.extraction_rules.find("t");
  ASSERT_TRUE(t_rule_it != result1_it->second.extraction_rules.end());
  EXPECT_EQ(t_rule_it->second.size(), 1u);
  EXPECT_EQ(t_rule_it->second[0].attribute, "textContent");
  EXPECT_EQ(t_rule_it->second[0].sub_selector, "a");
  EXPECT_TRUE(t_rule_it->second[0].transforms.empty());

  // Validate "u" extraction rule
  auto u_rule_it = result1_it->second.extraction_rules.find("u");
  ASSERT_TRUE(u_rule_it != result1_it->second.extraction_rules.end());
  EXPECT_EQ(u_rule_it->second.size(), 1u);
  EXPECT_EQ(u_rule_it->second[0].attribute, "href");
  EXPECT_EQ(u_rule_it->second[0].sub_selector, "a");
  EXPECT_EQ(u_rule_it->second[0].transforms.size(), 2u);

  // Check #b input group (first)
  auto b_it = search_bi_pattern.input_groups.find("#b");
  ASSERT_TRUE(b_it != search_bi_pattern.input_groups.end());
  EXPECT_FALSE(b_it->second.select_all);
  EXPECT_EQ(b_it->second.extraction_rules.size(), 1u);

  // Validate "q" extraction rule
  auto q_rule_it = b_it->second.extraction_rules.find("q");
  ASSERT_TRUE(q_rule_it != b_it->second.extraction_rules.end());
  EXPECT_EQ(q_rule_it->second.size(), 1u);
  EXPECT_EQ(q_rule_it->second[0].attribute, "textContent");
  EXPECT_EQ(q_rule_it->second[0].sub_selector, "#query");
  EXPECT_TRUE(q_rule_it->second[0].transforms.empty());

  // Check #result2 input group (first)
  auto result2_it = search_bi_pattern.input_groups.find("#result2");
  ASSERT_TRUE(result2_it != search_bi_pattern.input_groups.end());
  EXPECT_FALSE(result2_it->second.select_all);
  EXPECT_EQ(result2_it->second.extraction_rules.size(), 3u);

  // Validate "extraUrl" extraction rule
  auto extra_url_it = result2_it->second.extraction_rules.find("extraUrl");
  ASSERT_TRUE(extra_url_it != result2_it->second.extraction_rules.end());
  EXPECT_EQ(extra_url_it->second.size(), 1u);
  EXPECT_EQ(extra_url_it->second[0].attribute, "href");
  EXPECT_EQ(extra_url_it->second[0].sub_selector, "a");
  EXPECT_EQ(extra_url_it->second[0].transforms.size(), 1u);

  // Validate "extraTitle" extraction rule
  auto extra_title_it = result2_it->second.extraction_rules.find("extraTitle");
  ASSERT_TRUE(extra_title_it != result2_it->second.extraction_rules.end());
  EXPECT_EQ(extra_title_it->second.size(), 1u);
  EXPECT_EQ(extra_title_it->second[0].attribute, "textContent");
  EXPECT_EQ(extra_title_it->second[0].sub_selector, "a");
  EXPECT_TRUE(extra_title_it->second[0].transforms.empty());

  // Validate "inputValue" extraction rule
  auto input_value_it = result2_it->second.extraction_rules.find("inputValue");
  ASSERT_TRUE(input_value_it != result2_it->second.extraction_rules.end());
  EXPECT_EQ(input_value_it->second.size(), 1u);
  EXPECT_EQ(input_value_it->second[0].attribute, "value");
  EXPECT_EQ(input_value_it->second[0].sub_selector, "#input1");
  EXPECT_TRUE(input_value_it->second[0].transforms.empty());
}

TEST_F(WebDiscoveryPatternsV2Test, OutputGroups) {
  auto search_bi_it = patterns_->site_patterns.find(RelevantSite::kBing);
  ASSERT_TRUE(search_bi_it != patterns_->site_patterns.end());

  const auto& search_bi_pattern = search_bi_it->second;

  // Check output groups
  EXPECT_EQ(search_bi_pattern.output_groups.size(), 2u);

  // Find groups by action name using std::ranges::find_if
  auto query_it = std::ranges::find_if(
      search_bi_pattern.output_groups,
      [](const auto& group) { return group.action == "query"; });
  auto extra_data_it = std::ranges::find_if(
      search_bi_pattern.output_groups,
      [](const auto& group) { return group.action == "extraData"; });

  ASSERT_TRUE(query_it != search_bi_pattern.output_groups.end());
  ASSERT_TRUE(extra_data_it != search_bi_pattern.output_groups.end());

  const auto& query_group = *query_it;
  const auto& extra_data_group = *extra_data_it;

  // Validate query group
  EXPECT_EQ(query_group.fields.size(), 4u);

  // Validate query group fields
  EXPECT_EQ(query_group.fields[0].key, "r");
  EXPECT_EQ(query_group.fields[0].source_selector, "#b .result1");
  EXPECT_EQ(query_group.fields[0].required_keys.size(), 2u);
  EXPECT_EQ(query_group.fields[0].required_keys[0], "t");
  EXPECT_EQ(query_group.fields[0].required_keys[1], "u");
  EXPECT_FALSE(query_group.fields[0].optional);

  EXPECT_EQ(query_group.fields[1].key, "q");
  EXPECT_EQ(query_group.fields[1].source_selector, "#b");
  EXPECT_TRUE(query_group.fields[1].required_keys.empty());
  EXPECT_FALSE(query_group.fields[1].optional);

  EXPECT_EQ(query_group.fields[2].key, "qurl");
  EXPECT_FALSE(query_group.fields[2].source_selector.has_value());
  EXPECT_TRUE(query_group.fields[2].required_keys.empty());
  EXPECT_FALSE(query_group.fields[2].optional);

  EXPECT_EQ(query_group.fields[3].key, "ctry");
  EXPECT_FALSE(query_group.fields[3].source_selector.has_value());
  EXPECT_TRUE(query_group.fields[3].required_keys.empty());
  EXPECT_FALSE(query_group.fields[3].optional);

  // Validate extraData group
  EXPECT_EQ(extra_data_group.fields.size(), 3u);

  // Validate extraData group fields
  EXPECT_EQ(extra_data_group.fields[0].key, "extraUrl");
  EXPECT_EQ(extra_data_group.fields[0].source_selector, "#result2");
  EXPECT_TRUE(extra_data_group.fields[0].required_keys.empty());
  EXPECT_FALSE(extra_data_group.fields[0].optional);

  EXPECT_EQ(extra_data_group.fields[1].key, "extraTitle");
  EXPECT_EQ(extra_data_group.fields[1].source_selector, "#result2");
  EXPECT_TRUE(extra_data_group.fields[1].required_keys.empty());
  EXPECT_FALSE(extra_data_group.fields[1].optional);

  EXPECT_EQ(extra_data_group.fields[2].key, "inputValue");
  EXPECT_EQ(extra_data_group.fields[2].source_selector, "#result2");
  EXPECT_TRUE(extra_data_group.fields[2].required_keys.empty());
  EXPECT_TRUE(extra_data_group.fields[2].optional);
}

TEST_F(WebDiscoveryPatternsV2Test, EmptyPattern) {
  auto search_bii_it = patterns_->site_patterns.find(RelevantSite::kBingImages);
  ASSERT_TRUE(search_bii_it != patterns_->site_patterns.end());

  const auto& search_bii_pattern = search_bii_it->second;

  // Check that search-bii has empty input and output
  EXPECT_TRUE(search_bii_pattern.input_groups.empty());
  EXPECT_TRUE(search_bii_pattern.output_groups.empty());
}

TEST_F(WebDiscoveryPatternsV2Test, BadPatterns) {
  // Test invalid JSON
  ASSERT_FALSE(ParseV2Patterns("ABC"));
  ASSERT_FALSE(ParseV2Patterns("[]"));

  // Test empty patterns
  auto empty_patterns = ParseV2Patterns("{}");
  ASSERT_TRUE(empty_patterns);
  EXPECT_TRUE(empty_patterns->site_patterns.empty());
}

}  // namespace web_discovery
