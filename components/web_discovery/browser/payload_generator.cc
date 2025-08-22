/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/payload_generator.h"

#include <algorithm>
#include <iterator>
#include <utility>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/map_util.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/web_discovery/browser/patterns_v2.h"
#include "brave/components/web_discovery/browser/privacy_guard.h"
#include "brave/components/web_discovery/browser/relevant_site.h"
#include "brave/components/web_discovery/browser/util.h"

namespace web_discovery {

namespace {

constexpr char kSearchResultKey[] = "r";
constexpr char kSearchResultURLKey[] = "u";
constexpr size_t kMinSearchResultSize = 4;

constexpr char kAliveAction[] = "alive";
constexpr char kStatusFieldName[] = "status";
constexpr char kTimestampFieldName[] = "t";
constexpr char kCountryCodeFieldName[] = "ctry";

constexpr auto kQueryActions = base::MakeFixedFlatSet<std::string_view>(
    {"query", "anon-query", "widgetTitle"});

bool ValueHasContent(const base::Value& value) {
  const auto* value_str = value.GetIfString();
  if (value_str) {
    return !value_str->empty();
  }
  if (!value.is_none()) {
    return true;
  }
  return false;
}

bool AggregatedDictHasContent(const base::Value::Dict& dict) {
  for (const auto [_k1, value] : dict) {
    const auto* value_dict = value.GetIfDict();
    if (!value_dict) {
      continue;
    }
    for (const auto [_k2, sub_value] : *value_dict) {
      if (ValueHasContent(sub_value)) {
        return true;
      }
    }
  }
  return false;
}

bool IsPrivateResult(std::string_view key, const base::Value::Dict& dict) {
  if (key != kSearchResultKey) {
    return false;
  }
  const auto* url = dict.FindString(kSearchResultURLKey);
  if (!url) {
    return false;
  }
  return ShouldDropURL(GURL(*url));
}

bool ShouldDropSearchResultPayload(std::string_view key, size_t result_size) {
  if (key != kSearchResultKey) {
    return false;
  }
  return result_size < kMinSearchResultSize;
}

bool ValidateListResult(std::string_view field_key,
                        bool is_query_action,
                        const base::Value::Dict& results) {
  if (!AggregatedDictHasContent(results)) {
    VLOG(1) << "Skipped payload due to lack of content";
    return false;
  }
  // Check if we have enough results for search results
  if (is_query_action &&
      ShouldDropSearchResultPayload(field_key, results.size())) {
    VLOG(1) << "Skipped search result payload due to too few results";
    return false;
  }

  return true;
}

base::Value::Dict CreatePayloadDict(const PayloadRuleGroup& rule_group,
                                    base::Value::Dict inner_payload) {
  base::Value::Dict payload;
  payload.Set(kActionKey, rule_group.action);
  payload.Set(kInnerPayloadKey, std::move(inner_payload));
  return payload;
}

std::optional<base::Value> GenerateClusteredJoinedPayload(
    bool is_query_action,
    const PayloadRule& rule,
    const PatternsURLDetails* matching_url_details,
    const std::vector<base::Value::Dict>& attribute_values) {
  base::Value::Dict joined_data;
  size_t counter = 0;
  for (const auto& value : attribute_values) {
    if (value.empty()) {
      continue;
    }
    if (is_query_action && IsPrivateResult(rule.key, value)) {
      VLOG(1) << "Omitting private search result";
      continue;
    }
    joined_data.Set(base::NumberToString(counter++), value.Clone());
  }
  if (!ValidateListResult(rule.key, is_query_action, joined_data)) {
    return std::nullopt;
  }
  return base::Value(std::move(joined_data));
}

std::optional<base::Value::Dict> GenerateClusteredPayload(
    const PayloadRuleGroup& rule_group,
    const PatternsURLDetails* matching_url_details,
    const PageScrapeResult* scrape_result) {
  base::Value::Dict inner_payload;
  for (const auto& rule : rule_group.rules) {
    base::Value payload_rule_data;
    auto attribute_values_it = scrape_result->fields.find(rule.selector);
    if (attribute_values_it == scrape_result->fields.end() ||
        attribute_values_it->second.empty()) {
      VLOG(1) << "Skipped clustered payload due to no values for root "
                 "selector, action = "
              << rule_group.action;
      return std::nullopt;
    }
    if (rule.is_join) {
      auto joined_payload = GenerateClusteredJoinedPayload(
          kQueryActions.contains(rule_group.action), rule, matching_url_details,
          attribute_values_it->second);
      if (!joined_payload) {
        VLOG(1) << "Skipped joined clustered payload, action = "
                << rule_group.action;
        return std::nullopt;
      }
      payload_rule_data = std::move(*joined_payload);
    } else {
      const auto* value = attribute_values_it->second[0].FindString(rule.key);
      if (!value || value->empty()) {
        VLOG(1) << "Skipped non-joined clustered payload, action = "
                << rule_group.action;
        return std::nullopt;
      }
      payload_rule_data = base::Value(*value);
    }
    inner_payload.Set(rule.key, std::move(payload_rule_data));
  }
  return CreatePayloadDict(rule_group, std::move(inner_payload));
}

void GenerateSinglePayloads(const ServerConfig& server_config,
                            const PayloadRuleGroup& rule_group,
                            const PageScrapeResult* scrape_result,
                            std::vector<base::Value::Dict>& payloads) {
  auto attribute_values_it = scrape_result->fields.find(rule_group.key);
  if (attribute_values_it == scrape_result->fields.end()) {
    return;
  }
  for (const auto& attribute_value : attribute_values_it->second) {
    auto dict = attribute_value.Clone();
    dict.Set(kCountryCodeAttrId, server_config.location);
    payloads.push_back(CreatePayloadDict(rule_group, std::move(dict)));
  }
}

std::optional<base::Value> ProcessV2List(
    const V2OutputField& field,
    const std::vector<base::Value::Dict>& scraped_values,
    const V2InputGroup& input_group,
    bool is_query_action,
    bool is_search_engine) {
  // Determine required keys: use field.required_keys if provided, otherwise all
  // input group keys
  std::vector<std::string> required_keys;
  if (!field.required_keys.empty()) {
    required_keys = field.required_keys;
  } else {
    // Extract keys from input group using transform
    std::ranges::transform(input_group.extraction_rules,
                           std::back_inserter(required_keys),
                           [](const auto& pair) { return pair.first; });
  }

  base::Value::Dict joined_results;
  size_t counter = 0;

  for (const auto& scraped_item : scraped_values) {
    // Validate required keys are present using ValueHasContent
    bool has_all_required_keys =
        std::ranges::all_of(required_keys, [&](const auto& key) {
          const auto* value = scraped_item.Find(key);
          return value && ValueHasContent(*value);
        });
    if (!has_all_required_keys) {
      continue;
    }

    // Check for private results if this is a query action
    if (is_query_action && IsPrivateResult(field.key, scraped_item)) {
      VLOG(1) << "Omitting private search result";
      continue;
    }

    joined_results.Set(base::NumberToString(counter++), scraped_item.Clone());
  }

  if (joined_results.empty() ||
      !ValidateListResult(field.key, is_query_action, joined_results)) {
    return std::nullopt;
  }

  return base::Value(std::move(joined_results));
}

std::optional<base::Value> ProcessV2SingleValue(
    const std::vector<base::Value::Dict>& scraped_values,
    std::string_view field_name) {
  if (scraped_values.empty()) {
    return std::nullopt;
  }

  const auto& first_value = scraped_values[0];
  const auto* value = first_value.Find(field_name);
  if (value && ValueHasContent(*value)) {
    return value->Clone();
  }
  return std::nullopt;
}

std::optional<base::Value> ProcessV2OutputField(
    const V2OutputField& field,
    const ServerConfig& server_config,
    const PageScrapeResult& scrape_result,
    const V2SitePattern& site_pattern,
    bool is_query_action,
    bool is_search_engine) {
  std::optional<base::Value> result;

  if (field.source_selector) {
    // Field has a source - look up scraped data
    const std::string& source_selector = field.source_selector.value();
    const auto* scraped_values =
        base::FindOrNull(scrape_result.fields, source_selector);

    if (!scraped_values || scraped_values->empty()) {
      // No data for this source
      VLOG(1) << "No data for source: " << source_selector;
      return std::nullopt;
    }
    // Find the input group for this selector to check select_all
    const auto* input_group =
        base::FindOrNull(site_pattern.input_groups, source_selector);
    if (!input_group) {
      VLOG(1) << "Input group not found for selector: " << source_selector;
      return std::nullopt;
    }
    if (input_group->select_all) {
      // Use ProcessV2List for select_all=true
      result = ProcessV2List(field, *scraped_values, *input_group,
                             is_query_action, is_search_engine);
    } else {
      // Use ProcessV2SingleValue for select_all=false
      result = ProcessV2SingleValue(*scraped_values, field.key);
    }
  } else {
    // Field has no source - handle special static fields
    auto value = GetRequestValue(field.key, scrape_result.url, server_config,
                                 scrape_result);
    if (value) {
      result = base::Value(*value);
    }
  }

  // Handle optional check at the end
  if (!result.has_value()) {
    if (!field.optional) {
      VLOG(1) << "No valid content for required field: " << field.key;
      return std::nullopt;
    }
    return base::Value();  // Skip optional field with no content
  }

  return result;
}

}  // namespace

std::vector<base::Value::Dict> GenerateQueryPayloads(
    const ServerConfig& server_config,
    const PatternsURLDetails* url_details,
    std::unique_ptr<PageScrapeResult> scrape_result) {
  std::vector<base::Value::Dict> payloads;
  for (const auto& rule_group : url_details->payload_rule_groups) {
    if (rule_group.rule_type == PayloadRuleType::kQuery &&
        rule_group.result_type == PayloadResultType::kClustered) {
      auto payload = GenerateClusteredPayload(rule_group, url_details,
                                              scrape_result.get());
      if (payload) {
        payloads.push_back(std::move(*payload));
      }
    } else if (rule_group.rule_type == PayloadRuleType::kSingle &&
               rule_group.result_type == PayloadResultType::kSingle) {
      GenerateSinglePayloads(server_config, rule_group, scrape_result.get(),
                             payloads);
    }
  }
  return payloads;
}

std::vector<base::Value::Dict> GenerateQueryPayloadsV2(
    const ServerConfig& server_config,
    const V2PatternsGroup& patterns_group,
    std::unique_ptr<PageScrapeResult> scrape_result) {
  std::vector<base::Value::Dict> payloads;

  // Find the site pattern associated with this scrape result
  auto relevant_site = RelevantSiteFromID(scrape_result->id);
  if (!relevant_site.has_value()) {
    VLOG(1) << "Unknown site ID: " << scrape_result->id;
    return payloads;
  }

  // Determine if this site is a search engine
  bool is_search_engine = IsRelevantSiteSearchEngine(*relevant_site);

  const auto* site_pattern =
      base::FindOrNull(patterns_group.site_patterns, *relevant_site);
  if (!site_pattern) {
    VLOG(1) << "No site pattern found for relevant site";
    return payloads;
  }

  // Process each output group for this site
  for (const auto& output_group : site_pattern->output_groups) {
    base::Value::Dict inner_payload;

    // Determine if this is a query action
    bool is_query_action = kQueryActions.contains(output_group.action);

    // Process each field in the output group
    for (const auto& field : output_group.fields) {
      auto processed_field = ProcessV2OutputField(
          field, server_config, *scrape_result, *site_pattern, is_query_action,
          is_search_engine);
      if (!processed_field) {
        // Required field failed processing
        break;
      }

      inner_payload.Set(field.key, std::move(*processed_field));
    }

    // Create payload if we have all required data
    if (inner_payload.size() == output_group.fields.size()) {
      base::Value::Dict payload;
      payload.Set(kActionKey, output_group.action);
      payload.Set(kInnerPayloadKey, std::move(inner_payload));
      payloads.push_back(std::move(payload));
    }
  }

  return payloads;
}

base::Value::Dict GenerateAlivePayload(const ServerConfig& server_config,
                                       std::string date_hour) {
  auto inner_payload = base::Value::Dict()
                           .Set(kStatusFieldName, true)
                           .Set(kTimestampFieldName, date_hour)
                           .Set(kCountryCodeFieldName, server_config.location);
  auto payload = base::Value::Dict()
                     .Set(kActionKey, kAliveAction)
                     .Set(kInnerPayloadKey, std::move(inner_payload));
  return payload;
}

}  // namespace web_discovery
