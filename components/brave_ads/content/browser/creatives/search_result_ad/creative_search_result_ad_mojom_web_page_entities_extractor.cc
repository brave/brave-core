/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/creatives/search_result_ad/creative_search_result_ad_mojom_web_page_entities_extractor.h"

#include <iterator>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_set.h"
#include "base/logging.h"
#include "base/ranges/algorithm.h"
#include "base/strings/escape.h"
#include "base/strings/string_number_conversions.h"
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

constexpr auto kRequiredCreativeSetConversionPropertyNames =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {kCreativeSetConversionObservationWindowPropertyName,
         kCreativeSetConversionUrlPatternPropertyName});

constexpr auto kCreativeSetConversionPropertyNames =
    base::MakeFixedFlatSet<std::string_view>(
        base::sorted_unique,
        {kCreativeSetConversionAdvertiserPublicKeyPropertyName,
         kCreativeSetConversionObservationWindowPropertyName,
         kCreativeSetConversionUrlPatternPropertyName});

bool GetStringValue(const schema_org::mojom::PropertyPtr& mojom_property,
                    std::string* out_value) {
  CHECK(mojom_property);
  CHECK(out_value);

  if (!mojom_property->values->is_string_values() ||
      mojom_property->values->get_string_values().size() != 1) {
    // Invalid type.
    return false;
  }

  *out_value = mojom_property->values->get_string_values()[0];

  return true;
}

bool GetRequiredStringValue(
    const schema_org::mojom::PropertyPtr& mojom_property,
    std::string* out_value) {
  CHECK(mojom_property);
  CHECK(out_value);

  if (!GetStringValue(mojom_property, out_value)) {
    return false;
  }

  if (out_value->empty()) {
    return false;
  }

  return true;
}

bool GetIntValue(const schema_org::mojom::PropertyPtr& mojom_property,
                 int32_t* out_value) {
  CHECK(mojom_property);
  CHECK(out_value);

  if (!mojom_property->values->is_long_values() ||
      mojom_property->values->get_long_values().size() != 1) {
    // Invalid type.
    return false;
  }

  *out_value =
      static_cast<int32_t>(mojom_property->values->get_long_values()[0]);

  return true;
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

bool ExtractCreativeSetConversionMojomProperty(
    const schema_org::mojom::PropertyPtr& mojom_property,
    mojom::CreativeSetConversionInfo* mojom_creative_set_conversion) {
  CHECK(mojom_property);
  CHECK(mojom_creative_set_conversion);

  const std::string& property_name = mojom_property->name;

  if (property_name == kCreativeSetConversionUrlPatternPropertyName) {
    return GetRequiredStringValue(mojom_property,
                                  &mojom_creative_set_conversion->url_pattern);
  }

  if (property_name == kCreativeSetConversionAdvertiserPublicKeyPropertyName) {
    std::string verifiable_advertiser_public_key_base64;
    const bool success = GetStringValue(
        mojom_property, &verifiable_advertiser_public_key_base64);
    if (success && !verifiable_advertiser_public_key_base64.empty()) {
      mojom_creative_set_conversion->verifiable_advertiser_public_key_base64 =
          verifiable_advertiser_public_key_base64;
    }

    return success;
  }

  if (property_name == kCreativeSetConversionObservationWindowPropertyName) {
    int32_t observation_window;
    const bool success = GetIntValue(mojom_property, &observation_window);
    if (success) {
      mojom_creative_set_conversion->observation_window =
          base::Days(observation_window);
    }

    return success;
  }

  return false;
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

  mojom::CreativeSetConversionInfoPtr mojom_creative_set_conversion =
      mojom::CreativeSetConversionInfo::New();

  base::flat_set<std::string_view> creative_ad_property_names;

  base::flat_set<std::string_view> creative_set_conversion_property_names;

  for (const auto& mojom_property : mojom_entity->properties) {
    if (!mojom_property) {
      return {};
    }

    const std::string_view property_name = mojom_property->name;
    if (base::Contains(kRequiredCreativeAdPropertyNames, property_name)) {
      if (!ExtractCreativeAdMojomProperty(mojom_property,
                                          mojom_creative_ad.get())) {
        VLOG(6) << "Failed to extract creative search result ad property "
                << property_name;
        return {};
      }

      creative_ad_property_names.insert(property_name);
    } else if (base::Contains(kCreativeSetConversionPropertyNames,
                              property_name)) {
      if (!ExtractCreativeSetConversionMojomProperty(
              mojom_property, mojom_creative_set_conversion.get())) {
        VLOG(6) << "Failed to extract creative set conversion property "
                << property_name;
        return {};
      }

      creative_set_conversion_property_names.insert(property_name);
    }
  }

  std::vector<std::string_view> missing_creative_ad_property_names;
  base::ranges::set_difference(
      kRequiredCreativeAdPropertyNames.cbegin(),
      kRequiredCreativeAdPropertyNames.cend(),
      creative_ad_property_names.cbegin(), creative_ad_property_names.cend(),
      std::back_inserter(missing_creative_ad_property_names));

  if (!missing_creative_ad_property_names.empty()) {
    VLOG(6) << base::JoinString(missing_creative_ad_property_names, ", ")
            << " creative search result ad required properties are missing";
    return {};
  }

  if (!creative_set_conversion_property_names.empty()) {
    std::vector<std::string_view>
        missing_creative_set_conversion_property_names;
    base::ranges::set_difference(
        kRequiredCreativeSetConversionPropertyNames.cbegin(),
        kRequiredCreativeSetConversionPropertyNames.cend(),
        creative_set_conversion_property_names.cbegin(),
        creative_set_conversion_property_names.cend(),
        std::back_inserter(missing_creative_set_conversion_property_names));

    if (missing_creative_set_conversion_property_names.empty()) {
      mojom_creative_ad->creative_set_conversion =
          std::move(mojom_creative_set_conversion);
    } else {
      VLOG(6) << base::JoinString(
                     missing_creative_set_conversion_property_names, ", ")
              << " creative set conversion required properties are missing";
    }
  }

  if (mojom_creative_ad->placement_id.empty()) {
    // Invalid placement id.
    return {};
  }

  mojom_creative_ad->type = mojom::AdType::kSearchResultAd;

  return mojom_creative_ad;
}

std::vector<mojom::CreativeSearchResultAdInfoPtr> ExtractMojomProperty(
    const schema_org::mojom::PropertyPtr& mojom_property) {
  std::vector<mojom::CreativeSearchResultAdInfoPtr> creative_search_result_ads;

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

void Log(const std::vector<mojom::CreativeSearchResultAdInfoPtr>&
             creative_search_result_ads) {
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

    if (!mojom_creative_ad->creative_set_conversion) {
      return;
    }

    VLOG(6) << "Creative set conversion properties:\n"
            << kCreativeSetConversionUrlPatternPropertyName << ": "
            << mojom_creative_ad->creative_set_conversion->url_pattern << "\n"
            << kCreativeSetConversionAdvertiserPublicKeyPropertyName << ": "
            << mojom_creative_ad->creative_set_conversion
                   ->verifiable_advertiser_public_key_base64.value_or("")
            << "\n"
            << kCreativeSetConversionObservationWindowPropertyName << ": "
            << mojom_creative_ad->creative_set_conversion->observation_window;
  }
}

}  // namespace

std::vector<mojom::CreativeSearchResultAdInfoPtr>
ExtractCreativeSearchResultAdsFromMojomWebPageEntities(
    const std::vector<schema_org::mojom::EntityPtr>& mojom_entities) {
  std::vector<mojom::CreativeSearchResultAdInfoPtr> creative_search_result_ads;

  for (const auto& mojom_entity : mojom_entities) {
    if (!mojom_entity ||
        mojom_entity->type != kCreativeSearchResultAdsProductMojomEntityType) {
      continue;
    }

    for (const auto& mojom_property : mojom_entity->properties) {
      base::ranges::move(ExtractMojomProperty(mojom_property),
                         std::back_inserter(creative_search_result_ads));
    }
  }

  Log(creative_search_result_ads);

  return creative_search_result_ads;
}

}  // namespace brave_ads
