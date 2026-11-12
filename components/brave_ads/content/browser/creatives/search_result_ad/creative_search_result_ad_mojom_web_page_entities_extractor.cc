/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_web_page_entities_extractor.h"

#include <algorithm>
#include <cstdint>
#include <iterator>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/numerics/checked_math.h"
#include "base/strings/escape.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_campaign_conversion_parse_result_type.h"
#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "components/schema_org/common/metadata.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

constexpr auto kRequiredCreativeAdPropertyNames =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {kCreativeAdAdvertiserIdPropertyName, kCreativeAdCampaignIdPropertyName,
         kCreativeAdCreativeInstanceIdPropertyName,
         kCreativeAdCreativeSetIdPropertyName,
         kCreativeAdDescriptionPropertyName,
         kCreativeAdHeadlineTextPropertyName,
         kCreativeAdLandingPagePropertyName, kCreativeAdPlacementIdPropertyName,
         kCreativeAdRewardsValuePropertyName});

bool GetRequiredStringValue(
    const schema_org::mojom::PropertyPtr& mojom_property,
    std::string* out_value) {
  CHECK(mojom_property);
  CHECK(out_value);

  if (!mojom_property->values->is_string_values() ||
      mojom_property->values->get_string_values().size() != 1) {
    // Invalid type.
    return false;
  }

  *out_value = mojom_property->values->get_string_values()[0];

  return !out_value->empty();
}

bool GetDoubleValue(const schema_org::mojom::PropertyPtr& mojom_property,
                    double* out_value) {
  CHECK(mojom_property);
  CHECK(out_value);

  if (!mojom_property->values->is_string_values() ||
      mojom_property->values->get_string_values().size() != 1) {
    // Invalid type.
    return false;
  }

  return base::StringToDouble(mojom_property->values->get_string_values()[0],
                              out_value);
}

bool GetUrlValue(const schema_org::mojom::PropertyPtr& mojom_property,
                 GURL* out_value) {
  CHECK(mojom_property);
  CHECK(out_value);

  if (!mojom_property->values->is_string_values() ||
      mojom_property->values->get_string_values().size() != 1) {
    // Invalid type.
    return false;
  }

  const GURL url(mojom_property->values->get_string_values()[0]);
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme)) {
    return false;
  }

  *out_value = url;

  return true;
}

bool ExtractCreativeAdMojomProperty(
    const schema_org::mojom::PropertyPtr& mojom_property,
    mojom::CreativeSearchResultAdInfo* mojom_creative_ad) {
  CHECK(mojom_property);
  CHECK(mojom_creative_ad);

  const std::string& property_name = mojom_property->name;
  if (property_name == kCreativeAdPlacementIdPropertyName) {
    std::string placement_id;
    if (!GetRequiredStringValue(mojom_property, &placement_id)) {
      return false;
    }

    // Escape all characters except alphanumerics and -._~ to make sure that the
    // placement id can be safely passed to JS function and can be compared with
    // an encoded placement id from search result click url.
    mojom_creative_ad->placement_id =
        base::EscapeAllExceptUnreserved(placement_id);
    return true;
  }

  if (property_name == kCreativeAdCreativeInstanceIdPropertyName) {
    return GetRequiredStringValue(mojom_property,
                                  &mojom_creative_ad->creative_instance_id);
  }

  if (property_name == kCreativeAdCreativeSetIdPropertyName) {
    return GetRequiredStringValue(mojom_property,
                                  &mojom_creative_ad->creative_set_id);
  }

  if (property_name == kCreativeAdCampaignIdPropertyName) {
    return GetRequiredStringValue(mojom_property,
                                  &mojom_creative_ad->campaign_id);
  }

  if (property_name == kCreativeAdAdvertiserIdPropertyName) {
    return GetRequiredStringValue(mojom_property,
                                  &mojom_creative_ad->advertiser_id);
  }

  if (property_name == kCreativeAdLandingPagePropertyName) {
    return GetUrlValue(mojom_property, &mojom_creative_ad->target_url);
  }

  if (property_name == kCreativeAdHeadlineTextPropertyName) {
    return GetRequiredStringValue(mojom_property,
                                  &mojom_creative_ad->headline_text);
  }

  if (property_name == kCreativeAdDescriptionPropertyName) {
    return GetRequiredStringValue(mojom_property,
                                  &mojom_creative_ad->description);
  }

  if (property_name == kCreativeAdRewardsValuePropertyName) {
    return GetDoubleValue(mojom_property, &mojom_creative_ad->value);
  }

  return false;
}

// Returns the single entity carried by `mojom_property`'s values, or null if
// `mojom_property` does not hold exactly one entity value.
const schema_org::mojom::EntityPtr* GetNestedEntity(
    const schema_org::mojom::PropertyPtr& mojom_property) {
  CHECK(mojom_property);

  if (!mojom_property->values->is_entity_values() ||
      mojom_property->values->get_entity_values().size() != 1) {
    return nullptr;
  }
  return &mojom_property->values->get_entity_values()[0];
}

// Returns the sub-property named `name` directly under `mojom_entity`, or
// null if not present.
const schema_org::mojom::PropertyPtr* FindNestedProperty(
    const schema_org::mojom::EntityPtr& mojom_entity,
    std::string_view name) {
  CHECK(mojom_entity);

  for (const auto& mojom_sub_property : mojom_entity->properties) {
    if (mojom_sub_property && mojom_sub_property->name == name) {
      return &mojom_sub_property;
    }
  }
  return nullptr;
}

std::optional<bool> GetBoolValue(
    const schema_org::mojom::PropertyPtr& mojom_property) {
  if (!mojom_property->values->is_bool_values() ||
      mojom_property->values->get_bool_values().size() != 1) {
    return std::nullopt;
  }
  return mojom_property->values->get_bool_values()[0];
}

std::optional<std::string> GetStringValue(
    const schema_org::mojom::PropertyPtr& mojom_property) {
  if (!mojom_property->values->is_string_values() ||
      mojom_property->values->get_string_values().size() != 1) {
    return std::nullopt;
  }
  return mojom_property->values->get_string_values()[0];
}

std::optional<int32_t> GetIntValue(
    const schema_org::mojom::PropertyPtr& mojom_property) {
  if (!mojom_property->values->is_long_values() ||
      mojom_property->values->get_long_values().size() != 1) {
    return std::nullopt;
  }

  int32_t out_value;
  if (!base::CheckedNumeric<int32_t>(
           mojom_property->values->get_long_values()[0])
           .AssignIfValid(&out_value)) {
    return std::nullopt;
  }
  return out_value;
}

std::optional<bool> GetNestedBoolValue(
    const schema_org::mojom::EntityPtr& mojom_entity,
    std::string_view name) {
  const schema_org::mojom::PropertyPtr* const mojom_sub_property =
      FindNestedProperty(mojom_entity, name);
  return mojom_sub_property ? GetBoolValue(*mojom_sub_property) : std::nullopt;
}

std::optional<std::string> GetNestedStringValue(
    const schema_org::mojom::EntityPtr& mojom_entity,
    std::string_view name) {
  const schema_org::mojom::PropertyPtr* const mojom_sub_property =
      FindNestedProperty(mojom_entity, name);
  return mojom_sub_property ? GetStringValue(*mojom_sub_property)
                            : std::nullopt;
}

// A legacy flat `url`/`observationWindow` pair also exists for older
// browsers, but only the nested `data-conversion` object is read.
CampaignConversionParseResultType ExtractCampaignConversionMojomProperty(
    const schema_org::mojom::PropertyPtr& mojom_property,
    mojom::CampaignConversionInfo* mojom_campaign_conversion) {
  CHECK(mojom_property);
  CHECK(mojom_campaign_conversion);

  const schema_org::mojom::EntityPtr* const mojom_data_entity =
      GetNestedEntity(mojom_property);
  if (!mojom_data_entity) {
    return CampaignConversionParseResultType::kMalformed;
  }

  bool has_required_properties = true;

  if (const schema_org::mojom::PropertyPtr* const mojom_url_pattern_property =
          FindNestedProperty(*mojom_data_entity,
                             kCampaignConversionDataUrlPatternPropertyName)) {
    const std::optional<std::string> thank_you_url_pattern =
        GetStringValue(*mojom_url_pattern_property);
    if (!thank_you_url_pattern) {
      return CampaignConversionParseResultType::kMalformed;
    }
    if (!thank_you_url_pattern->empty()) {
      mojom_campaign_conversion->thank_you_url_pattern = thank_you_url_pattern;
    }
  } else {
    has_required_properties = false;
  }

  if (const schema_org::mojom::PropertyPtr* const
          mojom_observation_window_property = FindNestedProperty(
              *mojom_data_entity,
              kCampaignConversionDataObservationWindowPropertyName)) {
    const std::optional<int32_t> observation_window =
        GetIntValue(*mojom_observation_window_property);
    if (!observation_window) {
      return CampaignConversionParseResultType::kMalformed;
    }
    mojom_campaign_conversion->observation_window =
        base::Days(*observation_window);
  } else {
    has_required_properties = false;
  }

  if (const schema_org::mojom::PropertyPtr* const mojom_order_property =
          FindNestedProperty(*mojom_data_entity,
                             kCampaignConversionDataOrderPropertyName)) {
    if (const schema_org::mojom::EntityPtr* const mojom_order_entity =
            GetNestedEntity(*mojom_order_property)) {
      mojom_campaign_conversion->extract_id =
          GetNestedBoolValue(*mojom_order_entity,
                             kCampaignConversionDataOrderExtractIdPropertyName)
              .value_or(false);
      mojom_campaign_conversion->extract_value =
          GetNestedBoolValue(
              *mojom_order_entity,
              kCampaignConversionDataOrderExtractValuePropertyName)
              .value_or(false);
      mojom_campaign_conversion->checkout_url_pattern = GetNestedStringValue(
          *mojom_order_entity,
          kCampaignConversionDataOrderCheckoutUrlPatternPropertyName);
    }
  }

  return has_required_properties
             ? CampaignConversionParseResultType::kComplete
             : CampaignConversionParseResultType::kIncomplete;
}

mojom::CreativeSearchResultAdInfoPtr ExtractMojomEntity(
    const schema_org::mojom::EntityPtr& mojom_entity) {
  if (!mojom_entity ||
      mojom_entity->type != kCreativeSearchResultAdMojomEntityType) {
    // Unsupported type.
    return {};
  }

  mojom::CreativeSearchResultAdInfoPtr mojom_creative_ad =
      mojom::CreativeSearchResultAdInfo::New();

  mojom::CampaignConversionInfoPtr mojom_campaign_conversion =
      mojom::CampaignConversionInfo::New();

  base::flat_set<std::string_view> creative_ad_property_names;

  bool has_campaign_conversion_data = false;

  for (const auto& mojom_property : mojom_entity->properties) {
    if (!mojom_property) {
      return {};
    }

    if (kRequiredCreativeAdPropertyNames.contains(mojom_property->name)) {
      if (!ExtractCreativeAdMojomProperty(mojom_property,
                                          mojom_creative_ad.get())) {
        VLOG(6) << "Failed to extract creative search result ad property "
                << mojom_property->name;
        return {};
      }

      creative_ad_property_names.insert(mojom_property->name);
    } else if (mojom_property->name == kCampaignConversionDataPropertyName) {
      switch (ExtractCampaignConversionMojomProperty(
          mojom_property, mojom_campaign_conversion.get())) {
        case CampaignConversionParseResultType::kMalformed:
          VLOG(6) << "Failed to extract campaign conversion property "
                  << mojom_property->name;
          return {};
        case CampaignConversionParseResultType::kIncomplete:
          VLOG(6) << "Campaign conversion required properties are missing";
          break;
        case CampaignConversionParseResultType::kComplete:
          has_campaign_conversion_data = true;
          break;
      }
    }
  }

  std::vector<std::string_view> missing_creative_ad_property_names;
  std::ranges::set_difference(
      kRequiredCreativeAdPropertyNames.cbegin(),
      kRequiredCreativeAdPropertyNames.cend(),
      creative_ad_property_names.cbegin(), creative_ad_property_names.cend(),
      std::back_inserter(missing_creative_ad_property_names));

  if (!missing_creative_ad_property_names.empty()) {
    VLOG(6) << base::JoinString(missing_creative_ad_property_names, ", ")
            << " creative search result ad required properties are missing";
    return {};
  }

  if (has_campaign_conversion_data) {
    if (mojom_campaign_conversion->thank_you_url_pattern &&
        !mojom_campaign_conversion->thank_you_url_pattern->empty()) {
      mojom_creative_ad->campaign_conversion =
          std::move(mojom_campaign_conversion);
    } else {
      VLOG(6) << kCampaignConversionDataUrlPatternPropertyName
              << " campaign conversion required property is missing";
    }
  }

  if (mojom_creative_ad->placement_id.empty()) {
    // Invalid placement id.
    return {};
  }

  mojom_creative_ad->type = mojom::AdType::kSearchResultAd;

  return mojom_creative_ad;
}

CreativeSearchResultAdList ExtractMojomProperty(
    const schema_org::mojom::PropertyPtr& mojom_property) {
  CreativeSearchResultAdList creative_search_result_ads;

  if (!mojom_property ||
      mojom_property->name != kCreativeSearchResultAdsMojomPropertyName) {
    return {};
  }

  const schema_org::mojom::ValuesPtr& mojom_values = mojom_property->values;
  if (!mojom_values || !mojom_values->is_entity_values()) {
    return {};
  }

  for (const auto& mojom_entity : mojom_values->get_entity_values()) {
    if (auto creative_search_result_ad = ExtractMojomEntity(mojom_entity)) {
      creative_search_result_ads.push_back(
          std::move(creative_search_result_ad));
    }
  }

  return creative_search_result_ads;
}

void Log(const CreativeSearchResultAdList& creative_search_result_ads) {
  const bool should_log = VLOG_IS_ON(6);
  if (!should_log) {
    return;
  }

  for (const auto& mojom_creative_ad : creative_search_result_ads) {
    VLOG(6) << "Creative search result ad properties:\n"
            << kCreativeAdPlacementIdPropertyName << ": "
            << mojom_creative_ad->placement_id << "\n"
            << kCreativeAdCreativeInstanceIdPropertyName << ": "
            << mojom_creative_ad->creative_instance_id << "\n"
            << kCreativeAdCreativeSetIdPropertyName << ": "
            << mojom_creative_ad->creative_set_id << "\n"
            << kCreativeAdCampaignIdPropertyName << ": "
            << mojom_creative_ad->campaign_id << "\n"
            << kCreativeAdAdvertiserIdPropertyName << ": "
            << mojom_creative_ad->advertiser_id << "\n"
            << kCreativeAdLandingPagePropertyName << ": "
            << mojom_creative_ad->target_url << "\n"
            << kCreativeAdHeadlineTextPropertyName << ": "
            << mojom_creative_ad->headline_text << "\n"
            << kCreativeAdDescriptionPropertyName << ": "
            << mojom_creative_ad->description << "\n"
            << kCreativeAdRewardsValuePropertyName << ": "
            << mojom_creative_ad->value;

    if (!mojom_creative_ad->campaign_conversion) {
      return;
    }

    const mojom::CampaignConversionInfo& campaign_conversion =
        *mojom_creative_ad->campaign_conversion;

    VLOG(6) << "Campaign conversion properties:\n"
            << kCampaignConversionDataObservationWindowPropertyName << ": "
            << campaign_conversion.observation_window;

    if (campaign_conversion.thank_you_url_pattern) {
      VLOG(6) << kCampaignConversionDataUrlPatternPropertyName << ": "
              << *campaign_conversion.thank_you_url_pattern;
    }
  }
}

}  // namespace

CreativeSearchResultAdList
ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
    const std::vector<schema_org::mojom::EntityPtr>& mojom_entities) {
  CreativeSearchResultAdList creative_search_result_ads;

  for (const auto& mojom_entity : mojom_entities) {
    if (!mojom_entity ||
        mojom_entity->type != kCreativeSearchResultAdsProductMojomEntityType) {
      continue;
    }

    for (const auto& mojom_property : mojom_entity->properties) {
      std::ranges::move(ExtractMojomProperty(mojom_property),
                        std::back_inserter(creative_search_result_ads));
    }
  }

  Log(creative_search_result_ads);

  return creative_search_result_ads;
}

}  // namespace brave_ads
