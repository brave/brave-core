/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/payload_generator.h"

#include <utility>

#include "base/containers/fixed_flat_set.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "brave/components/web_discovery/browser/privacy_guard.h"

namespace web_discovery {

namespace {

constexpr char kCountryCodeFieldName[] = "ctry";
constexpr char kSearchResultKey[] = "r";
constexpr char kSearchResultURLKey[] = "u";
constexpr size_t kMinSearchResultSize = 4;

constexpr char kAliveAction[] = "alive";
constexpr char kStatusFieldName[] = "status";
constexpr char kTimestampFieldName[] = "t";

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
    dict.Set(kCountryCodeFieldName, server_config.location);
    payloads.push_back(CreatePayloadDict(rule_group, std::move(dict)));
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
