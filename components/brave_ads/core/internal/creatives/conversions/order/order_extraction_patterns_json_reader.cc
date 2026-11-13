/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader.h"

#include <string>
#include <string_view>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/component_order_id_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/component_order_value_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_resource_info.h"

namespace brave_ads::json::reader {

namespace {

constexpr std::string_view kVersionKey = "version";
constexpr std::string_view kConversionsKey = "conversions";
constexpr std::string_view kOrderKey = "order";

constexpr std::string_view kRegexKey = "regex";
constexpr std::string_view kSearchKey = "search";
constexpr std::string_view kValidationKey = "validation";

constexpr std::string_view kOrderIdKey = "id";
constexpr std::string_view kOrderValueKey = "value";
constexpr std::string_view kOrderSitesKey = "sites";

constexpr std::string_view kIdPatternsKey = "patterns";
constexpr std::string_view kIdDefaultMaxCombinedTextLengthInBytesKey =
    "defaultMaxCombinedTextLengthInBytes";
constexpr std::string_view kIdDefaultMaxDescendantDepthKey =
    "defaultMaxDescendantDepth";
constexpr std::string_view kIdDefaultPatternKey = "defaultPattern";

constexpr std::string_view kValueCurrencyKey = "currency";
constexpr std::string_view kValueKeywordKey = "keyword";
constexpr std::string_view kValueDefaultMaxAncestorDepthKey =
    "defaultMaxAncestorDepth";

constexpr std::string_view kSiteIdPatternKey = "pattern";
constexpr std::string_view kSiteValueMaxAncestorDepthKey = "maxAncestorDepth";
constexpr std::string_view kSiteIdMaxDescendantDepthKey = "maxDescendantDepth";
constexpr std::string_view kSiteIdMaxCombinedTextLengthInBytesKey =
    "maxCombinedTextLengthInBytes";

std::optional<base::flat_map<std::string, std::string>> MaybeParseIdRegexes(
    const base::DictValue* const dict) {
  const base::DictValue* const regex_dict =
      dict ? dict->FindDict(kRegexKey) : nullptr;
  const base::Value* const patterns_value =
      regex_dict ? regex_dict->Find(kIdPatternsKey) : nullptr;
  if (!patterns_value) {
    return base::flat_map<std::string, std::string>();
  }
  return MaybeParseCompiledRegexes(*patterns_value);
}

bool ParseIdSearchBounds(const base::DictValue* const dict,
                         ComponentOrderIdExtractionPatternsInfo& id_patterns) {
  const bool should_require_search_bounds = !id_patterns.regexes.empty();
  const base::DictValue* const search_dict =
      dict ? dict->FindDict(kSearchKey) : nullptr;
  if (should_require_search_bounds &&
      (!search_dict ||
       !search_dict->Find(kIdDefaultMaxCombinedTextLengthInBytesKey) ||
       !search_dict->Find(kIdDefaultMaxDescendantDepthKey))) {
    return false;
  }
  if (!search_dict) {
    return true;
  }

  if (const base::Value* const value =
          search_dict->Find(kIdDefaultMaxCombinedTextLengthInBytesKey)) {
    std::optional<int> default_max_combined_text_length =
        MaybeParseNonNegativeInt(*value);
    if (!default_max_combined_text_length) {
      return false;
    }
    id_patterns.default_max_combined_text_length_to_search =
        *default_max_combined_text_length;
  }

  if (const base::Value* const value =
          search_dict->Find(kIdDefaultMaxDescendantDepthKey)) {
    std::optional<int> default_max_descendant_depth =
        MaybeParseNonNegativeInt(*value);
    if (!default_max_descendant_depth) {
      return false;
    }
    id_patterns.default_number_of_descendant_levels_to_search =
        *default_max_descendant_depth;
  }
  return true;
}

bool ParseIdDefaultValidationPattern(
    const base::DictValue* const dict,
    ComponentOrderIdExtractionPatternsInfo& id_patterns) {
  const base::DictValue* const validation_dict =
      dict ? dict->FindDict(kValidationKey) : nullptr;
  const base::Value* const value =
      validation_dict ? validation_dict->Find(kIdDefaultPatternKey) : nullptr;
  if (!value) {
    return true;
  }
  std::optional<std::string> default_pattern =
      MaybeParseValidationPattern(*value);
  if (!default_pattern) {
    return false;
  }
  id_patterns.default_validation_pattern = std::move(*default_pattern);
  return true;
}

std::optional<ComponentOrderIdExtractionPatternsInfo> MaybeParseIdPatterns(
    const base::DictValue* const dict) {
  ComponentOrderIdExtractionPatternsInfo id_patterns;

  std::optional<base::flat_map<std::string, std::string>> regexes =
      MaybeParseIdRegexes(dict);
  if (!regexes) {
    return std::nullopt;
  }
  id_patterns.regexes = std::move(*regexes);

  if (!ParseIdSearchBounds(dict, id_patterns)) {
    return std::nullopt;
  }

  // `validation` is a sibling of `regex`/`search`, since it also
  // re-validates a bare selector match, not just the regex fallback.
  if (!ParseIdDefaultValidationPattern(dict, id_patterns)) {
    return std::nullopt;
  }

  return id_patterns;
}

bool ParseValueRegexes(
    const base::DictValue* const dict,
    ComponentOrderValueExtractionPatternsInfo& value_patterns) {
  const base::DictValue* const regex_dict =
      dict ? dict->FindDict(kRegexKey) : nullptr;
  if (!regex_dict) {
    return true;
  }

  if (const base::Value* const value = regex_dict->Find(kValueCurrencyKey)) {
    std::optional<base::flat_map<std::string, std::string>> currency_regexes =
        MaybeParseCompiledRegexes(*value);
    if (!currency_regexes) {
      return false;
    }
    value_patterns.currency_regexes = std::move(*currency_regexes);
  }

  if (const base::Value* const value = regex_dict->Find(kValueKeywordKey)) {
    std::optional<base::flat_map<std::string, std::string>> keyword_regexes =
        MaybeParseCompiledRegexes(*value);
    if (!keyword_regexes) {
      return false;
    }
    value_patterns.keyword_regexes = std::move(*keyword_regexes);
  }
  return true;
}

bool ParseValueDefaultMaxAncestorDepth(
    const base::DictValue* const dict,
    ComponentOrderValueExtractionPatternsInfo& value_patterns) {
  const bool should_require_search_bounds =
      !value_patterns.currency_regexes.empty() &&
      !value_patterns.keyword_regexes.empty();
  const base::DictValue* const search_dict =
      dict ? dict->FindDict(kSearchKey) : nullptr;
  if (should_require_search_bounds &&
      (!search_dict || !search_dict->Find(kValueDefaultMaxAncestorDepthKey))) {
    return false;
  }
  if (!search_dict) {
    return true;
  }

  const base::Value* const value =
      search_dict->Find(kValueDefaultMaxAncestorDepthKey);
  if (!value) {
    return true;
  }
  std::optional<int> default_max_ancestor_depth =
      MaybeParseNonNegativeInt(*value);
  if (!default_max_ancestor_depth) {
    return false;
  }
  value_patterns.default_number_of_ancestor_levels_to_search =
      *default_max_ancestor_depth;
  return true;
}

std::optional<ComponentOrderValueExtractionPatternsInfo>
MaybeParseValuePatterns(const base::DictValue* const dict) {
  ComponentOrderValueExtractionPatternsInfo value_patterns;

  if (!ParseValueRegexes(dict, value_patterns)) {
    return std::nullopt;
  }

  if (!ParseValueDefaultMaxAncestorDepth(dict, value_patterns)) {
    return std::nullopt;
  }

  return value_patterns;
}

bool ParseSiteValueMaxAncestorDepthOverride(
    const base::DictValue& value_dict,
    const std::string& site,
    ComponentOrderValueExtractionPatternsInfo& value_patterns) {
  const base::DictValue* const search_dict = value_dict.FindDict(kSearchKey);
  const base::Value* const value =
      search_dict ? search_dict->Find(kSiteValueMaxAncestorDepthKey) : nullptr;
  if (!value) {
    return true;
  }
  std::optional<int> max_ancestor_depth = MaybeParseNonNegativeInt(*value);
  if (!max_ancestor_depth) {
    return false;
  }
  value_patterns.number_of_ancestor_levels_to_search_overrides[site] =
      *max_ancestor_depth;
  return true;
}

// Each site bundles its `value`/`id` selectors and search-bound overrides
// together, rather than splitting them across separate site-keyed maps.
bool ParseSiteValuePatterns(
    const base::DictValue& value_dict,
    const std::string& site,
    ComponentOrderValueExtractionPatternsInfo& value_patterns) {
  if (!ParseSiteValueMaxAncestorDepthOverride(value_dict, site,
                                              value_patterns)) {
    return false;
  }

  std::optional<
      base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>
      selectors_by_page_type = MaybeParseSelectorsByPageType(value_dict);
  if (!selectors_by_page_type) {
    return false;
  }
  if (!selectors_by_page_type->empty()) {
    value_patterns.selectors[site] = std::move(*selectors_by_page_type);
  }
  return true;
}

bool ParseSiteIdValidationPatternOverride(
    const base::DictValue& id_dict,
    const std::string& site,
    ComponentOrderIdExtractionPatternsInfo& id_patterns) {
  const base::DictValue* const validation_dict =
      id_dict.FindDict(kValidationKey);
  const base::Value* const value =
      validation_dict ? validation_dict->Find(kSiteIdPatternKey) : nullptr;
  if (!value) {
    return true;
  }
  std::optional<std::string> pattern = MaybeParseValidationPattern(*value);
  if (!pattern) {
    return false;
  }
  id_patterns.validation_pattern_overrides[site] = std::move(*pattern);
  return true;
}
bool ParseSiteIdSearchBoundOverrides(
    const base::DictValue& id_dict,
    const std::string& site,
    ComponentOrderIdExtractionPatternsInfo& id_patterns) {
  const base::DictValue* const search_dict = id_dict.FindDict(kSearchKey);
  if (!search_dict) {
    return true;
  }

  if (const base::Value* const value =
          search_dict->Find(kSiteIdMaxCombinedTextLengthInBytesKey)) {
    std::optional<int> max_combined_text_length =
        MaybeParseNonNegativeInt(*value);
    if (!max_combined_text_length) {
      return false;
    }
    id_patterns.max_combined_text_length_to_search_overrides[site] =
        *max_combined_text_length;
  }

  if (const base::Value* const value =
          search_dict->Find(kSiteIdMaxDescendantDepthKey)) {
    std::optional<int> max_descendant_depth = MaybeParseNonNegativeInt(*value);
    if (!max_descendant_depth) {
      return false;
    }
    id_patterns.number_of_descendant_levels_to_search_overrides[site] =
        *max_descendant_depth;
  }
  return true;
}

bool ParseSiteIdPatterns(const base::DictValue& id_dict,
                         const std::string& site,
                         ComponentOrderIdExtractionPatternsInfo& id_patterns) {
  if (!ParseSiteIdSearchBoundOverrides(id_dict, site, id_patterns)) {
    return false;
  }

  if (!ParseSiteIdValidationPatternOverride(id_dict, site, id_patterns)) {
    return false;
  }

  std::optional<
      base::flat_map<OrderExtractionPageType, OrderExtractionSelectors>>
      selectors_by_page_type = MaybeParseSelectorsByPageType(id_dict);
  if (!selectors_by_page_type) {
    return false;
  }
  if (!selectors_by_page_type->empty()) {
    id_patterns.selectors[site] = std::move(*selectors_by_page_type);
  }
  return true;
}

bool ParseSites(const base::DictValue* const sites_dict,
                ComponentOrderIdExtractionPatternsInfo& id_patterns,
                ComponentOrderValueExtractionPatternsInfo& value_patterns) {
  if (!sites_dict) {
    return true;
  }

  for (const auto [site, site_value] : *sites_dict) {
    const base::DictValue* const site_dict = site_value.GetIfDict();
    if (!site_dict) {
      return false;
    }

    if (const base::DictValue* const value_dict =
            site_dict->FindDict(kOrderValueKey)) {
      if (!ParseSiteValuePatterns(*value_dict, site, value_patterns)) {
        return false;
      }
    }

    if (const base::DictValue* const id_dict =
            site_dict->FindDict(kOrderIdKey)) {
      if (!ParseSiteIdPatterns(*id_dict, site, id_patterns)) {
        return false;
      }
    }
  }
  return true;
}

bool RequiresGlobalIdValidationPattern(
    const ComponentOrderIdExtractionPatternsInfo& id_patterns) {
  return !id_patterns.regexes.empty() || !id_patterns.selectors.empty();
}

}  // namespace

std::optional<OrderExtractionPatternsInfo> MaybeReadOrderPatterns(
    const base::DictValue& order_dict) {
  OrderExtractionPatternsInfo patterns;

  std::optional<ComponentOrderIdExtractionPatternsInfo> id_patterns =
      MaybeParseIdPatterns(order_dict.FindDict(kOrderIdKey));
  if (!id_patterns) {
    return std::nullopt;
  }
  patterns.id = std::move(*id_patterns);

  std::optional<ComponentOrderValueExtractionPatternsInfo> value_patterns =
      MaybeParseValuePatterns(order_dict.FindDict(kOrderValueKey));
  if (!value_patterns) {
    return std::nullopt;
  }
  patterns.value = std::move(*value_patterns);

  if (!ParseSites(order_dict.FindDict(kOrderSitesKey), patterns.id,
                  patterns.value)) {
    return std::nullopt;
  }

  if (RequiresGlobalIdValidationPattern(patterns.id) &&
      patterns.id.default_validation_pattern.empty()) {
    return std::nullopt;
  }

  return patterns;
}

std::optional<OrderExtractionPatternsResourceInfo>
MaybeReadOrderExtractionPatternsResource(const base::DictValue& dict) {
  OrderExtractionPatternsResourceInfo resource;

  // An unsupported version is not malformed content, so it is left for the
  // caller to decide, same as `AntiTargetingResourceInfo::MaybeFromDict`.
  resource.version = dict.FindInt(kVersionKey);

  const base::DictValue* const conversions_dict =
      dict.FindDict(kConversionsKey);
  const base::DictValue* const order_dict =
      conversions_dict ? conversions_dict->FindDict(kOrderKey) : nullptr;
  if (order_dict) {
    std::optional<OrderExtractionPatternsInfo> patterns =
        MaybeReadOrderPatterns(*order_dict);
    if (!patterns) {
      return std::nullopt;
    }
    resource.patterns = std::move(*patterns);
  }

  return resource;
}

}  // namespace brave_ads::json::reader
