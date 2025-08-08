/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/payload_generator.h"

#include <utility>

#include "base/containers/fixed_flat_set.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/web_discovery/browser/patterns_v2.h"
#include "brave/components/web_discovery/browser/privacy_guard.h"
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

bool IsPrivateResult(const PayloadRule& rule,
                     const PatternsURLDetails* matching_url_details,
                     const base::Value::Dict& dict) {
  if (rule.key != kSearchResultKey) {
    return false;
  }
  const auto* url = dict.FindString(kSearchResultURLKey);
  if (!url) {
    return false;
  }
  return IsPrivateURLLikely(GURL(*url), matching_url_details);
}

bool ShouldDropSearchResultPayload(const PayloadRule& rule,
                                   size_t result_size) {
  if (rule.key != kSearchResultKey) {
    return false;
  }
  return result_size < kMinSearchResultSize;
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
    if (is_query_action && IsPrivateResult(rule, matching_url_details, value)) {
      VLOG(1) << "Omitting private search result";
      continue;
    }
    joined_data.Set(base::NumberToString(counter++), value.Clone());
  }
  if (!AggregatedDictHasContent(joined_data)) {
    VLOG(1) << "Skipped joined clustered payload due to lack of content";
    return std::nullopt;
  }
  if (is_query_action &&
      ShouldDropSearchResultPayload(rule, joined_data.size())) {
    VLOG(1) << "Skipped search result payload due to too few results";
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
    const PatternsV2OutputField& field,
    const std::vector<base::Value::Dict>& scraped_values,
    const std::vector<std::string>& required_keys) {
  base::Value::Dict joined_results;
  size_t counter = 0;
  
  for (const auto& scraped_item : scraped_values) {
    // Validate required keys are present (non-blank strings)
    bool has_all_required_keys = true;
    for (const auto& required_key : required_keys) {
      const auto* value = scraped_item.FindString(required_key);
      if (!value || value->empty()) {
        has_all_required_keys = false;
        break;
      }
    }
    
    if (has_all_required_keys) {
      // Check for private results if this is a search result field
      if (field.key == kSearchResultKey) {
        PayloadRule temp_rule;
        temp_rule.key = field.key;
        if (IsPrivateResult(temp_rule, nullptr, scraped_item)) {
          VLOG(1) << "Omitting private search result";
          continue;
        }
      }
      
      joined_results.Set(base::NumberToString(counter++), 
                         scraped_item.Clone());
    }
  }
  
  // Check if we have enough results for search results
  if (field.key == kSearchResultKey) {
    PayloadRule temp_rule;
    temp_rule.key = field.key;
    if (ShouldDropSearchResultPayload(temp_rule, joined_results.size())) {
      return std::nullopt;
    }
  }
  
  if (!AggregatedDictHasContent(joined_results)) {
    return std::nullopt;
  }
  
  return base::Value(std::move(joined_results));
}

std::optional<std::string> ProcessV2SingleValue(
    const std::vector<base::Value::Dict>& scraped_values) {
  if (scraped_values.empty()) {
    return std::nullopt;
  }
  
  // Take the first scraped value
  const auto& first_value = scraped_values[0];
  if (first_value.size() == 1) {
    // Single key-value pair - extract the string value
    auto it = first_value.begin();
    const auto* str_value = it->second.GetIfString();
    if (str_value && !str_value->empty()) {
      return *str_value;
    }
  }
  return std::nullopt;
}

std::optional<base::Value> ProcessV2OutputField(
    const PatternsV2OutputField& field,
    const ServerConfig& server_config,
    const PageScrapeResult& scrape_result,
    const std::string& output_group_name,
    const PatternsV2SitePattern& site_pattern) {
  if (field.source.has_value()) {
    // Field has a source - look up scraped data
    const std::string& source_selector = field.source.value();
    auto scraped_data_it = scrape_result.fields.find(source_selector);
    
    if (scraped_data_it == scrape_result.fields.end() ||
        scraped_data_it->second.empty()) {
      if (!field.optional) {
        VLOG(1) << "Missing required source data for field: " << field.key
                << " in output group: " << output_group_name;
        return std::nullopt;
      }
      return base::Value(); // Skip optional field with no data
    }
    
    const auto& scraped_values = scraped_data_it->second;
    
    // Find the input group for this selector to check select_all
    auto input_group_it = site_pattern.input_groups.find(source_selector);
    if (input_group_it == site_pattern.input_groups.end()) {
      VLOG(1) << "Input group not found for selector: " << source_selector;
      if (!field.optional) {
        return std::nullopt;
      }
      return base::Value();
    }
    
    const auto& input_group = input_group_it->second;
    
    if (input_group.select_all) {
      // Use ProcessV2List for select_all=true
      std::vector<std::string> required_keys;
      
      // If no required keys provided, use all keys from input rule
      if (field.required_keys.empty()) {
        for (const auto& [key, extraction_rule] : input_group.extraction_rules) {
          required_keys.push_back(key);
        }
      } else {
        required_keys = field.required_keys;
      }
      
      auto processed_value = ProcessV2List(field, scraped_values, required_keys);
      if (!processed_value.has_value()) {
        if (!field.optional) {
          VLOG(1) << "No valid content for required field: " << field.key
                  << " in output group: " << output_group_name;
          return std::nullopt;
        }
        return base::Value(); // Skip optional field with no valid content
      }
      return std::optional<base::Value>(std::move(processed_value.value()));
    } else {
      // Use ProcessV2SingleValue for select_all=false
      auto string_value = ProcessV2SingleValue(scraped_values);
      if (!string_value.has_value() && !field.optional) {
        return std::nullopt;
      }
      if (string_value.has_value()) {
        return std::optional<base::Value>(base::Value(*string_value));
      }
      return base::Value(); // Optional field with no value
    }
  } else {
    // Field has no source - handle special static fields
    auto value = GetRequestValue(field.key, scrape_result.url, server_config, scrape_result);
    if (!value) {
      // Unknown static field - skip if optional
      if (!field.optional) {
        VLOG(1) << "Unknown static field: " << field.key;
        return std::nullopt;
      }
      return base::Value(); // Skip optional field
    }
    return std::optional<base::Value>(base::Value(*value));
  }
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
    const PatternsV2PatternsGroup& patterns_group,
    std::unique_ptr<PageScrapeResult> scrape_result) {
  std::vector<base::Value::Dict> payloads;
  
  // Iterate through all site patterns
  for (const auto& [site, site_pattern] : patterns_group.site_patterns) {
    // Process each output group for this site
    for (const auto& output_group : site_pattern.output_groups) {
      base::Value::Dict inner_payload;
      bool has_required_data = true;
      
      // Process each field in the output group
      for (const auto& field : output_group.fields) {
        auto processed_field = ProcessV2OutputField(field, server_config, 
                                                   *scrape_result, output_group.name,
                                                   site_pattern);
        if (!processed_field.has_value()) {
          // Required field failed processing
          has_required_data = false;
          break;
        }
        
        // Skip empty optional fields
        if (processed_field->is_none()) {
          continue;
        }
        
        inner_payload.Set(field.key, std::move(processed_field.value()));
      }
      
      // Create payload if we have all required data
      if (has_required_data && !inner_payload.empty()) {
        base::Value::Dict payload;
        payload.Set(kActionKey, output_group.name);
        payload.Set(kInnerPayloadKey, std::move(inner_payload));
        payloads.push_back(std::move(payload));
      }
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
