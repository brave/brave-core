/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/patterns_v2.h"

#include <utility>

#include "base/containers/fixed_flat_map.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/strings/string_util.h"

namespace web_discovery {

namespace {

constexpr auto kTransformFunctionMap =
    base::MakeFixedFlatMap<std::string_view, PatternsV2TransformFunction>({
        {"trySplit", PatternsV2TransformFunction::kTrySplit},
        {"decodeURIComponent",
         PatternsV2TransformFunction::kDecodeURIComponent},
        {"filterExact", PatternsV2TransformFunction::kFilterExact},
        {"tryDecodeURIComponent",
         PatternsV2TransformFunction::kTryDecodeURIComponent},
        {"removeParams", PatternsV2TransformFunction::kRemoveParams},
        {"maskU", PatternsV2TransformFunction::kMaskU},
        {"split", PatternsV2TransformFunction::kSplit},
        {"trim", PatternsV2TransformFunction::kTrim},
        {"relaxedMaskU", PatternsV2TransformFunction::kRelaxedMaskU},
        {"json", PatternsV2TransformFunction::kJson},
        {"queryParam", PatternsV2TransformFunction::kQueryParam},
        {"requireURL", PatternsV2TransformFunction::kRequireURL},
    });

constexpr auto kSelectorTypeMap =
    base::MakeFixedFlatMap<std::string_view, PatternsV2InputGroup::Type>({
        {"first", PatternsV2InputGroup::Type::kFirst},
        {"all", PatternsV2InputGroup::Type::kAll},
    });

// Dictionary keys used in JSON parsing
constexpr std::string_view kInputKey = "input";
constexpr std::string_view kOutputKey = "output";
constexpr std::string_view kSelectKey = "select";
constexpr std::string_view kAttrKey = "attr";
constexpr std::string_view kTransformKey = "transform";
constexpr std::string_view kKey = "key";
constexpr std::string_view kSourceKey = "source";
constexpr std::string_view kRequiredKeysKey = "requiredKeys";
constexpr std::string_view kOptionalKey = "optional";
constexpr std::string_view kFieldsKey = "fields";

// Parses a transformation array
std::optional<PatternsV2Transform> ParseTransform(
    const base::Value::List& transform_list) {
  if (transform_list.empty()) {
    return std::nullopt;
  }

  PatternsV2Transform transform;

  auto part_it = transform_list.begin();
  // First element should be the function name
  if (!part_it->is_string()) {
    VLOG(1) << "Transform function name must be a string";
    return std::nullopt;
  }
  const auto& function_name = part_it->GetString();
  auto function_it = kTransformFunctionMap.find(function_name);
  if (function_it == kTransformFunctionMap.end()) {
    VLOG(1) << "Unknown transform function: " << function_name;
    return std::nullopt;
  }

  transform.function = function_it->second;

  for (part_it++; part_it != transform_list.end(); part_it++) {
    transform.arguments.Append(part_it->Clone());
  }

  return transform;
}

// Parses an extraction rule object
std::optional<PatternsV2ExtractionRule> ParseExtractionRule(
    const base::Value::Dict& rule_dict) {
  PatternsV2ExtractionRule rule;

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
      auto transform = ParseTransform(transform_value.GetList());
      if (!transform) {
        return std::nullopt;
      }
      rule.transforms.emplace_back(std::move(*transform));
    }
  }

  return rule;
}

// Parses an input group (input section)
std::optional<PatternsV2InputGroup> ParseInputGroup(
    const std::string& selector,
    const base::Value::Dict& group_dict) {
  PatternsV2InputGroup input_group;
  input_group.selector = selector;

  if (group_dict.size() != 1) {
    VLOG(1) << "Input group must have exactly one key";
    return std::nullopt;
  }

  auto group_dict_it = group_dict.begin();
  const auto type_it = kSelectorTypeMap.find(group_dict_it->first);
  if (type_it == kSelectorTypeMap.end()) {
    VLOG(1) << "Unknown input type: " << group_dict_it->first;
    return std::nullopt;
  }
  input_group.type = type_it->second;

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
    auto extraction_rule = ParseExtractionRule(*field_dict);
    if (!extraction_rule) {
      return std::nullopt;
    }
    input_group.extraction_rules[field_name].emplace_back(
        std::move(*extraction_rule));
  }

  return input_group;
}

// Parses an output field
std::optional<PatternsV2OutputField> ParseOutputField(
    const base::Value::Dict& field_dict) {
  PatternsV2OutputField field;

  // Parse key (required)
  const auto* key = field_dict.FindString(kKey);
  if (!key) {
    VLOG(1) << "Output field missing required 'key'";
    return std::nullopt;
  }
  field.key = *key;

  // Parse source (optional)
  if (const auto* source = field_dict.FindString(kSourceKey)) {
    field.source = *source;
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
std::optional<PatternsV2OutputGroup> ParseOutputGroup(
    const std::string& group_name,
    const base::Value::Dict& group_dict) {
  PatternsV2OutputGroup output_group;
  output_group.name = group_name;

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
std::optional<PatternsV2SitePattern> ParseSitePattern(
    const base::Value::Dict& site_dict) {
  PatternsV2SitePattern site_pattern;

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
    auto input_group = ParseInputGroup(selector, *selector_value);
    if (!input_group) {
      return std::nullopt;
    }
    site_pattern.input_groups.emplace_back(std::move(*input_group));
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

// PatternsV2Transform implementation
PatternsV2Transform::PatternsV2Transform() = default;
PatternsV2Transform::~PatternsV2Transform() = default;
PatternsV2Transform::PatternsV2Transform(PatternsV2Transform&&) = default;
PatternsV2Transform& PatternsV2Transform::operator=(PatternsV2Transform&&) =
    default;

// PatternsV2ExtractionRule implementation
PatternsV2ExtractionRule::PatternsV2ExtractionRule() = default;
PatternsV2ExtractionRule::~PatternsV2ExtractionRule() = default;
PatternsV2ExtractionRule::PatternsV2ExtractionRule(PatternsV2ExtractionRule&&) =
    default;
PatternsV2ExtractionRule& PatternsV2ExtractionRule::operator=(
    PatternsV2ExtractionRule&&) = default;

// PatternsV2InputGroup implementation
PatternsV2InputGroup::PatternsV2InputGroup() = default;
PatternsV2InputGroup::~PatternsV2InputGroup() = default;
PatternsV2InputGroup::PatternsV2InputGroup(PatternsV2InputGroup&&) = default;
PatternsV2InputGroup& PatternsV2InputGroup::operator=(PatternsV2InputGroup&&) =
    default;

// PatternsV2OutputField implementation
PatternsV2OutputField::PatternsV2OutputField() = default;
PatternsV2OutputField::~PatternsV2OutputField() = default;
PatternsV2OutputField::PatternsV2OutputField(PatternsV2OutputField&&) = default;
PatternsV2OutputField& PatternsV2OutputField::operator=(
    PatternsV2OutputField&&) = default;

// PatternsV2OutputGroup implementation
PatternsV2OutputGroup::PatternsV2OutputGroup() = default;
PatternsV2OutputGroup::~PatternsV2OutputGroup() = default;
PatternsV2OutputGroup::PatternsV2OutputGroup(PatternsV2OutputGroup&&) = default;
PatternsV2OutputGroup& PatternsV2OutputGroup::operator=(
    PatternsV2OutputGroup&&) = default;

// PatternsV2SitePattern implementation
PatternsV2SitePattern::PatternsV2SitePattern() = default;
PatternsV2SitePattern::~PatternsV2SitePattern() = default;
PatternsV2SitePattern::PatternsV2SitePattern(PatternsV2SitePattern&&) = default;
PatternsV2SitePattern& PatternsV2SitePattern::operator=(
    PatternsV2SitePattern&&) = default;

// PatternsV2PatternsGroup implementation
PatternsV2PatternsGroup::PatternsV2PatternsGroup() = default;
PatternsV2PatternsGroup::~PatternsV2PatternsGroup() = default;
PatternsV2PatternsGroup::PatternsV2PatternsGroup(PatternsV2PatternsGroup&&) =
    default;
PatternsV2PatternsGroup& PatternsV2PatternsGroup::operator=(
    PatternsV2PatternsGroup&&) = default;

std::unique_ptr<PatternsV2PatternsGroup> ParseV2Patterns(
    std::string_view patterns_json) {
  auto json_value = base::JSONReader::Read(patterns_json);
  if (!json_value.has_value() || !json_value->is_dict()) {
    VLOG(1) << "Failed to parse v2 patterns JSON";
    return nullptr;
  }

  auto patterns_group = std::make_unique<PatternsV2PatternsGroup>();
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
