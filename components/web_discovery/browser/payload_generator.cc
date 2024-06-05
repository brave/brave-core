/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/payload_generator.h"

#include <utility>

namespace web_discovery {

namespace {

bool ValueHasContent(const base::Value& value) {
  const auto* value_str = value.GetIfString();
  if (value_str && !value_str->empty()) {
    return true;
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

base::Value::Dict CreatePayloadDict(const PayloadRuleGroup& rule_group,
                                    base::Value::Dict inner_payload) {
  base::Value::Dict payload;
  payload.Set(kActionKey, rule_group.action);
  payload.Set(kInnerPayloadKey, std::move(inner_payload));
  return payload;
}

std::optional<base::Value::Dict> GenerateClusteredPayload(
    const PayloadRuleGroup& rule_group,
    const PageScrapeResult* scrape_result) {
  base::Value::Dict inner_payload;
  for (const auto& rule : rule_group.rules) {
    base::Value payload_rule_data;
    auto attribute_values_it = scrape_result->fields.find(rule.selector);
    if (attribute_values_it == scrape_result->fields.end() ||
        attribute_values_it->second.empty()) {
      // TODO(djandries): remove this exception and actually insert ctry
      if (rule.selector == "ctry") {
        continue;
      }
      return std::nullopt;
    }
    if (rule.is_join) {
      base::Value::Dict joined_data;
      size_t counter = 0;
      for (const auto& value : attribute_values_it->second) {
        if (value.empty()) {
          continue;
        }
        joined_data.Set(base::NumberToString(counter++), value.Clone());
      }
      if (!AggregatedDictHasContent(joined_data)) {
        VLOG(1) << "Skipped joined clustered payload";
        return std::nullopt;
      }
      payload_rule_data = base::Value(std::move(joined_data));
    } else {
      const auto* value = attribute_values_it->second[0].FindString(rule.key);
      if (!value || value->empty()) {
        VLOG(1) << "Skipped non-joined clustered payload";
        return std::nullopt;
      }
      payload_rule_data = base::Value(*value);
    }
    inner_payload.Set(rule.key, std::move(payload_rule_data));
  }
  return CreatePayloadDict(rule_group, std::move(inner_payload));
}

void GenerateSinglePayloads(const PayloadRuleGroup& rule_group,
                            const PageScrapeResult* scrape_result,
                            std::vector<base::Value::Dict>& payloads) {
  auto attribute_values_it = scrape_result->fields.find(rule_group.key);
  if (attribute_values_it == scrape_result->fields.end()) {
    return;
  }
  for (const auto& attribute_value : attribute_values_it->second) {
    payloads.push_back(CreatePayloadDict(rule_group, attribute_value.Clone()));
  }
}

}  // namespace

std::vector<base::Value::Dict> GeneratePayloads(
    const PatternsURLDetails* url_details,
    std::unique_ptr<PageScrapeResult> scrape_result) {
  std::vector<base::Value::Dict> payloads;
  for (const auto& rule_group : url_details->payload_rule_groups) {
    if (rule_group.rule_type == PayloadRuleType::kQuery &&
        rule_group.result_type == PayloadResultType::kClustered) {
      auto payload = GenerateClusteredPayload(rule_group, scrape_result.get());
      if (payload) {
        payloads.push_back(std::move(*payload));
      }
    } else if (rule_group.rule_type == PayloadRuleType::kSingle &&
               rule_group.result_type == PayloadResultType::kSingle) {
      GenerateSinglePayloads(rule_group, scrape_result.get(), payloads);
    }
  }
  return payloads;
}

}  // namespace web_discovery
