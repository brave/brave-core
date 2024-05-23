/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/patterns.h"

#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"

namespace web_discovery {

namespace {

constexpr char kNormalPatternsKey[] = "normal";
constexpr char kStrictPatternsKey[] = "strict";
constexpr char kUrlPatternsKey[] = "urlPatterns";
constexpr char kSearchEnginesKey[] = "searchEngines";
constexpr char kIdMappingKey[] = "idMapping";
constexpr char kScrapeRulesKey[] = "scrape";
constexpr char kSubSelectorKey[] = "item";
constexpr char kRuleTypeKey[] = "type";
constexpr char kAttributeKey[] = "etype";

constexpr auto kRuleTypeMap =
    base::MakeFixedFlatMap<std::string_view, ScrapeRuleType>({
        {"standard", ScrapeRuleType::kStandard},
        {"searchQuery", ScrapeRuleType::kSearchQuery},
        {"widgetTitle", ScrapeRuleType::kWidgetTitle},
    });

std::optional<std::vector<ScrapeRuleGroup>> ParseScrapeRules(
    const base::Value::Dict* scrape_url_dict) {
  std::vector<ScrapeRuleGroup> result(scrape_url_dict->size());

  for (const auto [selector, rule_group_value] : *scrape_url_dict) {
    auto* rule_group_dict = rule_group_value.GetIfDict();
    if (!rule_group_dict) {
      VLOG(1) << "Rule group is not a dict";
      return std::nullopt;
    }
    auto rule_group_it = result.begin();
    rule_group_it->selector = selector;
    rule_group_it->rules = std::vector<ScrapeRule>(rule_group_dict->size());

    auto rule_it = rule_group_it->rules.begin();
    for (const auto [report_key, rule_value] : *rule_group_dict) {
      auto* rule_dict = rule_value.GetIfDict();
      if (!rule_dict) {
        VLOG(1) << "Rule details is not a dict";
        return std::nullopt;
      }
      auto* sub_selector = rule_dict->FindString(kSubSelectorKey);
      auto* attribute = rule_dict->FindString(kAttributeKey);
      auto* rule_type_str = rule_dict->FindString(kRuleTypeKey);
      if (!attribute) {
        VLOG(1) << "Attribute missing from rule";
        return std::nullopt;
      }
      rule_it->rule_type = ScrapeRuleType::kOther;
      if (rule_type_str) {
        auto rule_type_it = kRuleTypeMap.find(*rule_type_str);
        if (rule_type_it != kRuleTypeMap.end()) {
          rule_it->rule_type = rule_type_it->second;
        }
      }
      rule_it->attribute = *attribute;
      if (sub_selector) {
        rule_it->sub_selector = *sub_selector;
      }
      rule_it = std::next(rule_it);
    }

    rule_group_it = std::next(rule_group_it);
  }
  return result;
}

std::optional<std::vector<PatternsURLDetails>> ParsePatternsURLDetails(
    const base::Value::Dict* root_dict) {
  auto* url_patterns_list = root_dict->FindList(kUrlPatternsKey);
  auto* search_engines_list = root_dict->FindList(kSearchEnginesKey);
  auto* scrape_dict = root_dict->FindDict(kScrapeRulesKey);
  auto* id_mapping_dict = root_dict->FindDict(kIdMappingKey);
  if (!url_patterns_list || !search_engines_list || !scrape_dict ||
      !id_mapping_dict) {
    VLOG(1)
        << "Missing URL patterns, search engines, scrape rules or id mapping";
    return std::nullopt;
  }

  std::vector<PatternsURLDetails> result(url_patterns_list->size());

  for (size_t i = 0; i < url_patterns_list->size(); i++) {
    auto* url_regex = (*url_patterns_list)[i].GetIfString();
    if (!url_regex) {
      VLOG(1) << "URL pattern is not string";
      return std::nullopt;
    }
    auto& details = result[i];

    details.url_regex = *url_regex;

    std::string i_str = base::NumberToString(i);

    auto* id = id_mapping_dict->FindString(i_str);
    auto* scrape_url_dict = scrape_dict->FindDict(i_str);
    if (!id || !scrape_url_dict) {
      VLOG(1) << "ID or scrape dict missing for pattern";
      return std::nullopt;
    }
    details.id = *id;

    details.is_search_engine =
        base::ranges::find(search_engines_list->begin(),
                           search_engines_list->end(),
                           i_str) != search_engines_list->end();

    auto scrape_rule_groups = ParseScrapeRules(scrape_url_dict);
    if (!scrape_rule_groups) {
      return std::nullopt;
    }
    details.scrape_rule_groups = std::move(*scrape_rule_groups);
  }

  return result;
}

}  // namespace

ScrapeRule::ScrapeRule() = default;
ScrapeRule::~ScrapeRule() = default;

ScrapeRuleGroup::ScrapeRuleGroup() = default;
ScrapeRuleGroup::~ScrapeRuleGroup() = default;

PatternsURLDetails::PatternsURLDetails() = default;
PatternsURLDetails::~PatternsURLDetails() = default;

PatternsGroup::PatternsGroup() = default;
PatternsGroup::~PatternsGroup() = default;

std::unique_ptr<PatternsGroup> ParsePatterns(const std::string& patterns_json) {
  auto result = std::make_unique<PatternsGroup>();
  auto patterns_value = base::JSONReader::Read(patterns_json);
  if (!patterns_value || !patterns_value->is_dict()) {
    VLOG(1) << "Patterns is not JSON or is not dict";
    return nullptr;
  }
  const auto& patterns_dict = patterns_value->GetDict();

  auto* normal_dict = patterns_dict.FindDict(kNormalPatternsKey);
  auto* strict_dict = patterns_dict.FindDict(kStrictPatternsKey);
  if (!normal_dict && !strict_dict) {
    VLOG(1) << "No normal or strict rules in patterns";
    return nullptr;
  }

  if (normal_dict) {
    auto details = ParsePatternsURLDetails(normal_dict);
    if (!details) {
      return nullptr;
    }
    result->normal_patterns = std::move(*details);
  }
  if (strict_dict) {
    auto details = ParsePatternsURLDetails(strict_dict);
    if (!details) {
      return nullptr;
    }
    result->strict_patterns = std::move(*details);
  }
  return result;
}

}  // namespace web_discovery
