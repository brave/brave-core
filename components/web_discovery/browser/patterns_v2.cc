/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/patterns_v2.h"

#include <utility>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_util.h"

namespace web_discovery {

namespace {

// Selection type constants
constexpr char kSelectionTypeFirst[] = "first";
constexpr char kSelectionTypeAll[] = "all";

// Dictionary keys used in JSON parsing
constexpr char kInputKey[] = "input";
constexpr char kOutputKey[] = "output";
constexpr char kSelectKey[] = "select";
constexpr char kAttrKey[] = "attr";
constexpr char kTransformKey[] = "transform";
constexpr char kKey[] = "key";
constexpr char kSourceKey[] = "source";
constexpr char kRequiredKeysKey[] = "requiredKeys";
constexpr char kOptionalKey[] = "optional";
constexpr char kFieldsKey[] = "fields";
constexpr char kFirstMatchKey[] = "firstMatch";

// Parses an extraction rule object
std::optional<V2ExtractionRule> ParseExtractionRule(
    const base::Value::Dict& rule_dict) {
  V2ExtractionRule rule;

  // Parse attribute (required)
  const auto* attr = rule_dict.FindString(kAttrKey);
  if (!attr) {
    VLOG(1) << "Extraction rule missing required 'attr' field";
    return std::nullopt;
  }
  rule.attribute = *attr;

  // Parse sub-selector if present
  if (const auto* select = rule_dict.FindString(kSelectKey)) {
    rule.sub_selector = *select;
  }

  // Parse transforms if present
  if (const auto* transforms = rule_dict.FindList(kTransformKey)) {
    for (const auto& transform_value : *transforms) {
      if (!transform_value.is_list()) {
        VLOG(1) << "Transform value is not a list";
        return std::nullopt;
      }
      auto transform = CreateValueTransform(transform_value.GetList());
      if (!transform) {
        VLOG(1) << "Failed to create value transform";
        return std::nullopt;
      }
      rule.transforms.emplace_back(std::move(transform));
    }
  }

  return rule;
}

// Parses extraction rules for a single field, handling both single rules and
// firstMatch arrays
std::optional<std::vector<V2ExtractionRule>> ParseExtractionRules(
    const base::Value::Dict& field_dict) {
  std::vector<const base::Value::Dict*> rule_dicts;

  if (const auto* first_match_list = field_dict.FindList(kFirstMatchKey)) {
    rule_dicts.reserve(first_match_list->size());
    for (const auto& rule_value : *first_match_list) {
      const auto* rule_dict = rule_value.GetIfDict();
      if (!rule_dict) {
        return std::nullopt;
      }
      rule_dicts.push_back(rule_dict);
    }
  } else {
    // Single rule - use the field_dict itself
    rule_dicts.push_back(&field_dict);
  }

  // Parse all collected rule dictionaries
  std::vector<V2ExtractionRule> field_rules;
  field_rules.reserve(rule_dicts.size());

  for (const auto* rule_dict : rule_dicts) {
    auto extraction_rule = ParseExtractionRule(*rule_dict);
    if (!extraction_rule) {
      return std::nullopt;
    }
    field_rules.emplace_back(std::move(*extraction_rule));
  }

  return field_rules;
}

// Parses an input group (input section)
std::optional<V2InputGroup> ParseInputGroup(
    const base::Value::Dict& group_dict) {
  V2InputGroup input_group;

  if (group_dict.size() != 1) {
    VLOG(1) << "Input group must have exactly one key";
    return std::nullopt;
  }

  auto group_dict_it = group_dict.begin();
  const std::string& selection_type = group_dict_it->first;

  // Set select_all based on the selection type
  if (selection_type == kSelectionTypeAll) {
    input_group.select_all = true;
  } else if (selection_type == kSelectionTypeFirst) {
    input_group.select_all = false;
  } else {
    VLOG(1) << "Unknown input selection type: " << selection_type;
    return std::nullopt;
  }

  const auto* rules_dict = group_dict_it->second.GetIfDict();
  if (!rules_dict) {
    VLOG(1) << "Rules dictionary is not a dictionary";
    return std::nullopt;
  }

  // Parse extraction rules
  for (auto it = rules_dict->begin(); it != rules_dict->end(); ++it) {
    const auto& field_name = it->first;
    const auto& field_value = it->second;
    const auto* field_dict = field_value.GetIfDict();
    if (!field_dict) {
      VLOG(1) << "Field value is not a dictionary";
      return std::nullopt;
    }

    VLOG(2) << "Parsing extraction rules for field: " << field_name;
    auto extraction_rules = ParseExtractionRules(*field_dict);
    if (!extraction_rules) {
      return std::nullopt;
    }

    input_group.extraction_rules.emplace(field_name,
                                         std::move(*extraction_rules));
  }

  return input_group;
}

// Parses an output field
std::optional<V2OutputField> ParseOutputField(
    const base::Value::Dict& field_dict) {
  V2OutputField field;

  // Parse key (required)
  const auto* key = field_dict.FindString(kKey);
  if (!key) {
    VLOG(1) << "Output field missing required 'key'";
    return std::nullopt;
  }
  field.key = *key;

  // Parse source (optional)
  if (const auto* source = field_dict.FindString(kSourceKey)) {
    field.source_selector = *source;
  }

  // Parse required keys (optional)
  const auto* required_keys = field_dict.FindList(kRequiredKeysKey);
  if (required_keys) {
    for (const auto& key_value : *required_keys) {
      const auto* key_string = key_value.GetIfString();
      if (!key_string) {
        VLOG(1) << "Required key is not a string";
        return std::nullopt;
      }
      field.required_keys.push_back(*key_string);
    }
  }

  // Parse optional flag
  if (const auto optional = field_dict.FindBool(kOptionalKey)) {
    field.optional = *optional;
  }

  return field;
}

// Parses an output group
std::optional<V2OutputGroup> ParseOutputGroup(
    const std::string& group_name,
    const base::Value::Dict& group_dict) {
  V2OutputGroup output_group;
  output_group.action = group_name;

  // Parse fields
  const auto* fields = group_dict.FindList(kFieldsKey);
  if (!fields) {
    VLOG(1) << "Output group missing 'fields' array";
    return std::nullopt;
  }

  for (const auto& field_value : *fields) {
    const auto* field_dict = field_value.GetIfDict();
    if (!field_dict) {
      VLOG(1) << "Field value is not a dictionary";
      return std::nullopt;
    }
    auto field = ParseOutputField(*field_dict);
    if (!field) {
      return std::nullopt;
    }
    output_group.fields.emplace_back(std::move(*field));
  }

  return output_group;
}

// Parses a site pattern
std::optional<V2SitePattern> ParseSitePattern(
    const base::Value::Dict& site_dict) {
  V2SitePattern site_pattern;

  // Parse input section
  const auto* input_dict = site_dict.FindDict(kInputKey);
  const auto* output_dict = site_dict.FindDict(kOutputKey);

  if (!input_dict || !output_dict) {
    VLOG(1) << "Input or output section is missing";
    return std::nullopt;
  }

  for (auto it = input_dict->begin(); it != input_dict->end(); ++it) {
    const auto& selector = it->first;
    const auto* selector_value = it->second.GetIfDict();
    if (!selector_value) {
      VLOG(1) << "Input value is not a dictionary";
      return std::nullopt;
    }
    VLOG(2) << "Parsing input group for selector: " << selector;
    auto input_group = ParseInputGroup(*selector_value);
    if (!input_group) {
      return std::nullopt;
    }
    site_pattern.input_groups.emplace(selector, std::move(*input_group));
  }

  for (auto it = output_dict->begin(); it != output_dict->end(); ++it) {
    const auto& group_name = it->first;
    const auto* group_value = it->second.GetIfDict();
    if (!group_value) {
      VLOG(1) << "Output value is not a dictionary";
      return std::nullopt;
    }
    auto output_group = ParseOutputGroup(group_name, *group_value);
    if (!output_group) {
      return std::nullopt;
    }
    site_pattern.output_groups.emplace_back(std::move(*output_group));
  }

  return site_pattern;
}

}  // namespace

// PatternsV2ExtractionRule implementation
V2ExtractionRule::V2ExtractionRule() = default;
V2ExtractionRule::~V2ExtractionRule() = default;
V2ExtractionRule::V2ExtractionRule(V2ExtractionRule&&) = default;
V2ExtractionRule& V2ExtractionRule::operator=(V2ExtractionRule&&) = default;

// PatternsV2InputGroup implementation
V2InputGroup::V2InputGroup() = default;
V2InputGroup::~V2InputGroup() = default;
V2InputGroup::V2InputGroup(V2InputGroup&&) = default;
V2InputGroup& V2InputGroup::operator=(V2InputGroup&&) = default;

// PatternsV2OutputField implementation
V2OutputField::V2OutputField() = default;
V2OutputField::~V2OutputField() = default;
V2OutputField::V2OutputField(V2OutputField&&) = default;
V2OutputField& V2OutputField::operator=(V2OutputField&&) = default;

// PatternsV2OutputGroup implementation
V2OutputGroup::V2OutputGroup() = default;
V2OutputGroup::~V2OutputGroup() = default;
V2OutputGroup::V2OutputGroup(V2OutputGroup&&) = default;
V2OutputGroup& V2OutputGroup::operator=(V2OutputGroup&&) = default;

// PatternsV2SitePattern implementation
V2SitePattern::V2SitePattern() = default;
V2SitePattern::~V2SitePattern() = default;
V2SitePattern::V2SitePattern(V2SitePattern&&) = default;
V2SitePattern& V2SitePattern::operator=(V2SitePattern&&) = default;

// PatternsV2PatternsGroup implementation
V2PatternsGroup::V2PatternsGroup() = default;
V2PatternsGroup::~V2PatternsGroup() = default;
V2PatternsGroup::V2PatternsGroup(V2PatternsGroup&&) = default;
V2PatternsGroup& V2PatternsGroup::operator=(V2PatternsGroup&&) = default;

std::unique_ptr<V2PatternsGroup> ParseV2Patterns(
    std::string_view patterns_json) {
  auto json_value = base::JSONReader::Read(patterns_json);
  if (!json_value.has_value() || !json_value->is_dict()) {
    VLOG(1) << "Failed to parse v2 patterns JSON";
    return nullptr;
  }

  auto patterns_group = std::make_unique<V2PatternsGroup>();
  const auto& root_dict = json_value->GetDict();

  // Parse each site pattern
  for (auto it = root_dict.begin(); it != root_dict.end(); ++it) {
    const auto& site_id = it->first;
    const auto* site_value = it->second.GetIfDict();
    if (!site_value) {
      VLOG(1) << "Site value is not a dictionary";
      return nullptr;
    }
    // Convert site_id string to RelevantSite enum using existing function
    auto relevant_site = RelevantSiteFromID(site_id);
    if (!relevant_site) {
      VLOG(1) << "Unknown site ID: " << site_id;
      continue;  // Skip unknown sites instead of failing
    }

    auto site_pattern = ParseSitePattern(*site_value);
    if (!site_pattern) {
      return nullptr;
    }
    patterns_group->site_patterns.emplace(*relevant_site,
                                          std::move(*site_pattern));
  }

  return patterns_group;
}

}  // namespace web_discovery
