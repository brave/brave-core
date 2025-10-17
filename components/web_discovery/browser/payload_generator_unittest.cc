/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/payload_generator.h"

#include <memory>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/test/values_test_util.h"
#include "brave/components/web_discovery/browser/content_scraper.h"
#include "brave/components/web_discovery/browser/patterns.h"
#include "brave/components/web_discovery/browser/patterns_v2.h"
#include "brave/components/web_discovery/browser/relevant_site.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

using base::test::IsSupersetOfValue;
using testing::Pointee;

namespace web_discovery {

class WebDiscoveryPayloadGeneratorTest : public testing::Test {
 public:
  ~WebDiscoveryPayloadGeneratorTest() override = default;

  // testing::Test:
  void SetUp() override {
    server_config_ = std::make_unique<ServerConfig>();
    server_config_->location = "us";
  }

  void InitPatterns() {
    url_details_ = std::make_unique<PatternsURLDetails>();

    url_details_->payload_rule_groups = std::vector<PayloadRuleGroup>(2);

    auto& single_group = url_details_->payload_rule_groups[0];
    single_group.key = ".single-element";
    single_group.rule_type = PayloadRuleType::kSingle;
    single_group.result_type = PayloadResultType::kSingle;
    single_group.action = "single_action";

    auto& query_group = url_details_->payload_rule_groups[1];
    query_group.key = "query_group";
    query_group.rule_type = PayloadRuleType::kQuery;
    query_group.result_type = PayloadResultType::kClustered;
    query_group.action = "query";
    query_group.rules = std::vector<PayloadRule>(2);

    auto& join_rule = query_group.rules[0];
    join_rule.selector = "#results";
    join_rule.key = "r";
    join_rule.is_join = true;

    auto& qurl_rule = query_group.rules[1];
    qurl_rule.selector = "qurl";
    qurl_rule.key = "qurl";
    qurl_rule.is_join = false;
  }

  void InitPatternsV2() {
    // Create V2 patterns group
    v2_patterns_group_ = std::make_unique<V2PatternsGroup>();

    // Create a test V2 site pattern
    V2SitePattern site_pattern;

    // Create input group for search results
    V2InputGroup results_input_group;
    results_input_group.select_all = true;

    // Create extraction rules for URL and text
    V2ExtractionRule url_rule;
    url_rule.sub_selector = "a";
    url_rule.attribute = "href";

    V2ExtractionRule text_rule;
    text_rule.sub_selector = "a";
    text_rule.attribute = "textContent";

    // Add rules to input group
    results_input_group.extraction_rules["url"].push_back(std::move(url_rule));
    results_input_group.extraction_rules["text"].push_back(
        std::move(text_rule));

    // Add input group to site pattern
    site_pattern.input_groups["#results"] = std::move(results_input_group);

    // Create output group
    V2OutputGroup output_group;
    output_group.action = "query";

    V2OutputField result_field;
    result_field.key = "r";
    result_field.source_selector = "#results";
    result_field.required_keys = {"url", "text"};

    V2OutputField ctry_field;
    ctry_field.key = "ctry";
    // ctry field doesn't need a source_selector as it comes from server config

    output_group.fields.push_back(std::move(result_field));
    output_group.fields.push_back(std::move(ctry_field));

    site_pattern.output_groups.push_back(std::move(output_group));

    // Create second output group for query0 action
    V2OutputGroup query0_output_group;
    query0_output_group.action = "query0";

    V2OutputField qurl_field;
    qurl_field.key = "qurl";
    // qurl field doesn't need a source_selector - comes from URL

    V2OutputField q_field;
    q_field.key = "q";
    // q field doesn't need a source_selector - comes from query extraction

    V2OutputField ctry_field2;
    ctry_field2.key = "ctry";
    // ctry field doesn't need a source_selector as it comes from server config

    query0_output_group.fields.push_back(std::move(qurl_field));
    query0_output_group.fields.push_back(std::move(q_field));
    query0_output_group.fields.push_back(std::move(ctry_field2));

    site_pattern.output_groups.push_back(std::move(query0_output_group));

    // Add site pattern to V2 patterns group
    v2_patterns_group_->site_patterns[RelevantSite::kBing] =
        std::move(site_pattern);
  }

 protected:
  std::vector<base::Value::Dict> GenerateQueryPayloadsHelper(
      std::unique_ptr<PageScrapeResult> scrape_result) {
    if (!url_details_) {
      InitPatterns();
    }
    return GenerateQueryPayloads(*server_config_.get(), url_details_.get(),
                                 std::move(scrape_result));
  }

  std::vector<base::Value::Dict> GenerateQueryPayloadsHelperV2(
      std::unique_ptr<PageScrapeResult> scrape_result) {
    if (!v2_patterns_group_) {
      InitPatternsV2();
    }
    return GenerateQueryPayloadsV2(*server_config_.get(),
                                   *v2_patterns_group_.get(),
                                   std::move(scrape_result));
  }

  std::unique_ptr<ServerConfig> server_config_;

 private:
  std::unique_ptr<PatternsURLDetails> url_details_;
  std::unique_ptr<V2PatternsGroup> v2_patterns_group_;
};

TEST_F(WebDiscoveryPayloadGeneratorTest, GenerateQueryPayloads) {
  GURL test_url("https://example.com/test");
  auto scrape_result = std::make_unique<PageScrapeResult>(test_url, "test_id");

  std::vector<base::Value::Dict> single_dicts;
  single_dicts.push_back(
      base::Value::Dict().Set("ab", "value1").Set("cd", "value2"));
  single_dicts.push_back(
      base::Value::Dict().Set("ef", "value3").Set("gh", "value4"));
  scrape_result->fields[".single-element"] = std::move(single_dicts);

  std::vector<base::Value::Dict> result_dicts1;
  result_dicts1.push_back(base::Value::Dict().Set("njk", "joinvalue1"));
  result_dicts1.push_back(base::Value::Dict().Set("abc", "joinvalue2"));
  result_dicts1.push_back(base::Value::Dict().Set("njk", "joinvalue3"));
  result_dicts1.push_back(base::Value::Dict().Set("abc", "joinvalue4"));

  std::vector<base::Value::Dict> result_dicts2;
  result_dicts2.push_back(
      base::Value::Dict().Set("qurl", "https://example.com/test1"));
  scrape_result->fields["#results"] = std::move(result_dicts1);
  scrape_result->fields["qurl"] = std::move(result_dicts2);

  auto payloads = GenerateQueryPayloadsHelper(std::move(scrape_result));
  ASSERT_EQ(payloads.size(), 3u);

  const auto* payload = &payloads[0];
  EXPECT_THAT(payload,
              Pointee(IsSupersetOfValue(
                  base::Value::Dict()
                      .Set(kActionKey, "single_action")
                      .Set(kInnerPayloadKey, base::Value::Dict()
                                                 .Set("ctry", "us")
                                                 .Set("ab", "value1")
                                                 .Set("cd", "value2")))));

  payload = &payloads[1];
  EXPECT_THAT(payload,
              Pointee(IsSupersetOfValue(
                  base::Value::Dict()
                      .Set(kActionKey, "single_action")
                      .Set(kInnerPayloadKey, base::Value::Dict()
                                                 .Set("ctry", "us")
                                                 .Set("ef", "value3")
                                                 .Set("gh", "value4")))));

  payload = &payloads[2];
  ASSERT_THAT(
      payload,
      Pointee(IsSupersetOfValue(
          base::Value::Dict()
              .Set(kActionKey, "query")
              .Set(kInnerPayloadKey,
                   base::Value::Dict().Set(
                       "r", base::Value::Dict()
                                .Set("0", base::Value::Dict().Set("njk",
                                                                  "joinvalue1"))
                                .Set("1", base::Value::Dict().Set("abc",
                                                                  "joinvalue2"))
                                .Set("2", base::Value::Dict().Set("njk",
                                                                  "joinvalue3"))
                                .Set("3", base::Value::Dict().Set(
                                              "abc", "joinvalue4")))))));
}

TEST_F(WebDiscoveryPayloadGeneratorTest, GenerateAlivePayload) {
  std::string date_hour = "2023051509";

  auto alive_payload = GenerateAlivePayload(*server_config_.get(), date_hour);

  ASSERT_THAT(
      alive_payload,
      IsSupersetOfValue(base::Value::Dict()
                            .Set(kActionKey, "alive")
                            .Set(kInnerPayloadKey, base::Value::Dict()
                                                       .Set("ctry", "us")
                                                       .Set("t", date_hour)
                                                       .Set("status", true))));
}

TEST_F(WebDiscoveryPayloadGeneratorTest, ExcludePrivateResult) {
  GURL test_url("https://example.com/search");
  auto scrape_result = std::make_unique<PageScrapeResult>(test_url, "test_id");

  std::vector<base::Value::Dict> result_dicts1;
  for (int i = 0; i < 5; i++) {
    base::Value::Dict result_dict;
    std::string url = "https://example.com/result";
    if (i == 1) {
      url = "https://88.88.88.88/example";
    } else {
      url += base::NumberToString(i == 0 ? 0 : i - 1);
    }
    result_dict.Set("u", url);
    result_dicts1.push_back(std::move(result_dict));
  }
  scrape_result->fields["#results"] = std::move(result_dicts1);
  std::vector<base::Value::Dict> result_dicts2;
  base::Value::Dict qurl_dict;
  qurl_dict.Set("qurl", "https://example.com/test1");
  result_dicts2.push_back(std::move(qurl_dict));
  scrape_result->fields["qurl"] = std::move(result_dicts2);

  auto payloads = GenerateQueryPayloadsHelper(std::move(scrape_result));
  ASSERT_EQ(payloads.size(), 1u);
  EXPECT_THAT(
      payloads[0],
      IsSupersetOfValue(base::Value::Dict().Set(
          kInnerPayloadKey,
          base::Value::Dict().Set(
              "r", base::Value::Dict()
                       .Set("0", base::Value::Dict().Set(
                                     "u", "https://example.com/result0"))
                       .Set("1", base::Value::Dict().Set(
                                     "u", "https://example.com/result1"))
                       .Set("2", base::Value::Dict().Set(
                                     "u", "https://example.com/result2"))
                       .Set("3", base::Value::Dict().Set(
                                     "u", "https://example.com/result3"))))));
}

TEST_F(WebDiscoveryPayloadGeneratorTest, ShouldDropSearchResult) {
  GURL test_url("https://example.com/search");
  auto scrape_result = std::make_unique<PageScrapeResult>(test_url, "test_id");

  std::vector<base::Value::Dict> result_dicts;
  for (int i = 0; i < 3; i++) {
    base::Value::Dict result_dict;
    result_dict.Set("u",
                    "https://example.com/result" + base::NumberToString(i));
    result_dicts.push_back(std::move(result_dict));
  }
  scrape_result->fields["#results"] = std::move(result_dicts);

  std::vector<base::Value::Dict> qurl_dicts;
  base::Value::Dict qurl_dict;
  qurl_dict.Set("qurl", "https://example.com/test1");
  qurl_dicts.push_back(std::move(qurl_dict));
  scrape_result->fields["qurl"] = std::move(qurl_dicts);

  auto payloads = GenerateQueryPayloadsHelper(std::move(scrape_result));
  ASSERT_EQ(payloads.size(), 0u);
}

TEST_F(WebDiscoveryPayloadGeneratorTest, ContentMissing) {
  GURL test_url("https://example.com/search");
  auto scrape_result = std::make_unique<PageScrapeResult>(test_url, "test_id");

  std::vector<base::Value::Dict> result_dicts;
  for (int i = 0; i < 9; i++) {
    base::Value::Dict result_dict;
    if (i < 5) {
      result_dict.Set("x", "");
    } else {
      result_dict.Set("x", base::Value());
    }
    result_dicts.push_back(std::move(result_dict));
  }
  scrape_result->fields["#results"] = std::move(result_dicts);

  std::vector<base::Value::Dict> qurl_dicts;
  base::Value::Dict qurl_dict;
  qurl_dict.Set("qurl", "https://example.com/test1");
  qurl_dicts.push_back(std::move(qurl_dict));
  scrape_result->fields["qurl"] = std::move(qurl_dicts);

  auto payloads = GenerateQueryPayloadsHelper(std::move(scrape_result));
  ASSERT_EQ(payloads.size(), 0u);
}

TEST_F(WebDiscoveryPayloadGeneratorTest, GenerateQueryPayloadsV2) {
  // Create test scrape result with V2 data structure
  GURL test_url("https://www.bing.com/search?q=apple%20types");
  auto scrape_result = std::make_unique<PageScrapeResult>(
      test_url, std::string(*RelevantSiteToID(RelevantSite::kBing)));
  scrape_result->query = "apple types";

  // Add scraped results data and build expected dict in one loop
  std::vector<base::Value::Dict> results_data;
  base::Value::Dict expected_r_dict;

  for (int i = 0; i < 8; ++i) {
    auto result_dict =
        base::Value::Dict()
            .Set("url", "https://example.com/result" + base::NumberToString(i))
            .Set("text", "Result " + base::NumberToString(i));

    results_data.push_back(result_dict.Clone());
    expected_r_dict.Set(base::NumberToString(i), std::move(result_dict));
  }

  scrape_result->fields["#results"] = std::move(results_data);

  // Generate V2 payloads using helper with lazy initialization
  auto payloads = GenerateQueryPayloadsHelperV2(std::move(scrape_result));

  // Verify generated payloads - should have both query and query0 actions
  ASSERT_EQ(payloads.size(), 2u);

  // Verify first payload (query action)
  EXPECT_THAT(
      payloads[0],
      IsSupersetOfValue(base::Value::Dict()
                            .Set(kActionKey, "query")
                            .Set(kInnerPayloadKey,
                                 base::Value::Dict()
                                     .Set("ctry", "us")
                                     .Set("r", std::move(expected_r_dict)))));

  // Verify second payload (query0 action)
  EXPECT_THAT(
      payloads[1],
      IsSupersetOfValue(
          base::Value::Dict()
              .Set(kActionKey, "query0")
              .Set(kInnerPayloadKey,
                   base::Value::Dict()
                       .Set("ctry", "us")
                       .Set("qurl",
                            "https://www.bing.com/search?q=apple%20types")
                       .Set("q", "apple types"))));
}

}  // namespace web_discovery
