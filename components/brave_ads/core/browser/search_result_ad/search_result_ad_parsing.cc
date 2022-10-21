/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/search_result_ad/search_result_ad_parsing.h"

#include <utility>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_set.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_number_conversions.h"
#include "brave/vendor/bat-native-ads/include/bat/ads/public/interfaces/ads.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/blink/public/mojom/document_metadata/document_metadata.mojom.h"

using ads::mojom::ConversionInfo;
using ads::mojom::ConversionInfoPtr;
using ads::mojom::SearchResultAdInfo;
using ads::mojom::SearchResultAdInfoPtr;

namespace brave_ads {

namespace {

constexpr char kProductType[] = "Product";
constexpr char kSearchResultAdType[] = "SearchResultAd";

constexpr char kContextPropertyName[] = "@context";
constexpr char kTypePropertyName[] = "@type";

constexpr char kDataPlacementId[] = "data-placement-id";
constexpr char kDataCreativeInstanceId[] = "data-creative-instance-id";
constexpr char kDataCreativeSetId[] = "data-creative-set-id";
constexpr char kDataCampaignId[] = "data-campaign-id";
constexpr char kDataAdvertiserId[] = "data-advertiser-id";
constexpr char kDataLandingPage[] = "data-landing-page";
constexpr char kDataHeadlineText[] = "data-headline-text";
constexpr char kDataDescription[] = "data-description";
constexpr char kDataRewardsValue[] = "data-rewards-value";
constexpr char kDataConversionTypeValue[] = "data-conversion-type-value";
constexpr char kDataConversionUrlPatternValue[] =
    "data-conversion-url-pattern-value";
constexpr char kDataConversionAdvertiserPublicKeyValue[] =
    "data-conversion-advertiser-public-key-value";
constexpr char kDataConversionObservationWindowValue[] =
    "data-conversion-observation-window-value";

constexpr auto kSearchResultAdAttributes =
    base::MakeFixedFlatSet<base::StringPiece>(
        {kDataPlacementId, kDataCreativeInstanceId, kDataCreativeSetId,
         kDataCampaignId, kDataAdvertiserId, kDataLandingPage,
         kDataHeadlineText, kDataDescription, kDataRewardsValue,
         kDataConversionTypeValue, kDataConversionUrlPatternValue,
         kDataConversionAdvertiserPublicKeyValue,
         kDataConversionObservationWindowValue});

bool GetStringValue(const schema_org::mojom::PropertyPtr& ad_property,
                    std::string* out_value) {
  DCHECK(ad_property);
  DCHECK(out_value);

  // Wrong attribute type
  if (!ad_property->values->is_string_values() ||
      ad_property->values->get_string_values().size() != 1) {
    return false;
  }

  *out_value = ad_property->values->get_string_values().front();

  return true;
}

bool GetIntValue(const schema_org::mojom::PropertyPtr& ad_property,
                 int32_t* out_value) {
  DCHECK(ad_property);
  DCHECK(out_value);

  // Wrong attribute type
  if (!ad_property->values->is_long_values() ||
      ad_property->values->get_long_values().size() != 1) {
    return false;
  }

  *out_value =
      static_cast<int32_t>(ad_property->values->get_long_values().front());

  return true;
}

bool GetDoubleValue(const schema_org::mojom::PropertyPtr& ad_property,
                    double* out_value) {
  DCHECK(ad_property);
  DCHECK(out_value);

  // Wrong attribute type
  if (!ad_property->values->is_string_values() ||
      ad_property->values->get_string_values().size() != 1) {
    return false;
  }

  const std::string& value = ad_property->values->get_string_values().front();
  return base::StringToDouble(value, out_value);
}

bool GetUrlValue(const schema_org::mojom::PropertyPtr& ad_property,
                 GURL* out_value) {
  DCHECK(ad_property);
  DCHECK(out_value);

  // Wrong attribute type
  if (!ad_property->values->is_string_values() ||
      ad_property->values->get_string_values().size() != 1) {
    return false;
  }

  const std::string& value = ad_property->values->get_string_values().front();
  const GURL url(value);
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme)) {
    return false;
  }

  *out_value = url;
  return true;
}

bool SetSearchAdProperty(const schema_org::mojom::PropertyPtr& ad_property,
                         SearchResultAdInfo* search_result_ad) {
  DCHECK(ad_property);
  DCHECK(search_result_ad);
  DCHECK(search_result_ad->conversion);

  const std::string& name = ad_property->name;
  if (name == kDataPlacementId) {
    return GetStringValue(ad_property, &search_result_ad->placement_id);
  }

  if (name == kDataCreativeInstanceId) {
    return GetStringValue(ad_property, &search_result_ad->creative_instance_id);
  }

  if (name == kDataCreativeSetId) {
    return GetStringValue(ad_property, &search_result_ad->creative_set_id);
  }

  if (name == kDataCampaignId) {
    return GetStringValue(ad_property, &search_result_ad->campaign_id);
  }

  if (name == kDataAdvertiserId) {
    return GetStringValue(ad_property, &search_result_ad->advertiser_id);
  }

  if (name == kDataLandingPage) {
    return GetUrlValue(ad_property, &search_result_ad->target_url);
  }

  if (name == kDataHeadlineText) {
    return GetStringValue(ad_property, &search_result_ad->headline_text);
  }

  if (name == kDataDescription) {
    return GetStringValue(ad_property, &search_result_ad->description);
  }

  if (name == kDataRewardsValue) {
    return GetDoubleValue(ad_property, &search_result_ad->value);
  }

  if (name == kDataConversionTypeValue) {
    return GetStringValue(ad_property, &search_result_ad->conversion->type);
  }

  if (name == kDataConversionUrlPatternValue) {
    return GetStringValue(ad_property,
                          &search_result_ad->conversion->url_pattern);
  }

  if (name == kDataConversionAdvertiserPublicKeyValue) {
    return GetStringValue(ad_property,
                          &search_result_ad->conversion->advertiser_public_key);
  }

  if (name == kDataConversionObservationWindowValue) {
    return GetIntValue(ad_property,
                       &search_result_ad->conversion->observation_window);
  }

  NOTREACHED();

  return false;
}

// TODO(https://github.com/brave/brave-browser/issues/25971): Reduce cognitive
// complexity.
void ParseSearchResultAdMapEntityProperties(
    const schema_org::mojom::EntityPtr& entity,
    SearchResultAdMap* search_result_ads) {
  DCHECK(entity);
  DCHECK_EQ(entity->type, kProductType);
  DCHECK(search_result_ads);

  for (const auto& property : entity->properties) {
    if (!property || property->name == kContextPropertyName ||
        property->name == kTypePropertyName) {
      continue;
    }

    // Search result ads list product could have only "@context" and "creatives"
    // properties.
    if (property->name != "creatives") {
      return;
    }

    if (!property->values->is_entity_values() ||
        property->values->get_entity_values().empty()) {
      VLOG(1) << "Search result ad attributes list is empty";
      return;
    }

    for (const auto& ad_entity : property->values->get_entity_values()) {
      if (!ad_entity || ad_entity->type != kSearchResultAdType) {
        VLOG(1) << "Wrong search result ad type specified: " << ad_entity->type;
        return;
      }

      if (property->name == kTypePropertyName) {
        continue;
      }

      SearchResultAdInfoPtr search_result_ad = SearchResultAdInfo::New();
      search_result_ad->conversion = ConversionInfo::New();

      base::flat_set<base::StringPiece> found_attributes;
      for (const auto& ad_property : ad_entity->properties) {
        // Wrong attribute name
        const auto* it = base::ranges::find(
            kSearchResultAdAttributes, base::StringPiece(ad_property->name));

        if (it == kSearchResultAdAttributes.end()) {
          VLOG(1) << "Wrong search result ad attribute specified: "
                  << ad_property->name;
          return;
        }
        found_attributes.insert(*it);

        if (!SetSearchAdProperty(ad_property, search_result_ad.get())) {
          VLOG(1) << "Cannot read search result ad attribute value: "
                  << ad_property->name;
          return;
        }
      }

      // Not all of attributes were specified.
      if (found_attributes.size() != kSearchResultAdAttributes.size()) {
        std::vector<base::StringPiece> absent_attributes;
        std::set_difference(kSearchResultAdAttributes.begin(),
                            kSearchResultAdAttributes.end(),
                            found_attributes.begin(), found_attributes.end(),
                            std::back_inserter(absent_attributes));

        VLOG(1) << "Some of search result ad attributes were not specified: "
                << base::JoinString(absent_attributes, ", ");

        return;
      }

      std::string creative_instance_id = search_result_ad->creative_instance_id;
      search_result_ads->emplace(std::move(creative_instance_id),
                                 std::move(search_result_ad));
    }

    // Creatives has been parsed.
    break;
  }
}

void LogSearchResultAdMap(const SearchResultAdMap& search_result_ads) {
  if (!VLOG_IS_ON(2)) {
    return;
  }

  if (search_result_ads.empty()) {
    return;
  }

  VLOG(2) << "Parsed search result ads list:";
  for (const auto& search_result_ad_pair : search_result_ads) {
    const auto& search_result_ad = search_result_ad_pair.second;
    VLOG(2) << "Ad with \"" << kDataPlacementId
            << "\": " << search_result_ad->placement_id;
    VLOG(2) << "  \"" << kDataCreativeInstanceId
            << "\": " << search_result_ad->creative_instance_id;
    VLOG(2) << "  \"" << kDataCreativeSetId
            << "\": " << search_result_ad->creative_set_id;
    VLOG(2) << "  \"" << kDataCampaignId
            << "\": " << search_result_ad->campaign_id;
    VLOG(2) << "  \"" << kDataAdvertiserId
            << "\": " << search_result_ad->advertiser_id;
    VLOG(2) << "  \"" << kDataLandingPage
            << "\": " << search_result_ad->target_url;
    VLOG(2) << "  \"" << kDataHeadlineText
            << "\": " << search_result_ad->headline_text;
    VLOG(2) << "  \"" << kDataDescription
            << "\": " << search_result_ad->description;
    VLOG(2) << "  \"" << kDataRewardsValue << "\": " << search_result_ad->value;
    VLOG(2) << "  \"" << kDataConversionTypeValue
            << "\": " << search_result_ad->conversion->type;
    VLOG(2) << "  \"" << kDataConversionUrlPatternValue
            << "\": " << search_result_ad->conversion->url_pattern;
    VLOG(2) << "  \"" << kDataConversionAdvertiserPublicKeyValue
            << "\": " << search_result_ad->conversion->advertiser_public_key;
    VLOG(2) << "  \"" << kDataConversionObservationWindowValue
            << "\": " << search_result_ad->conversion->observation_window;
  }
}

}  // namespace

SearchResultAdMap ParseWebPageEntities(blink::mojom::WebPagePtr web_page) {
  SearchResultAdMap search_result_ads;
  for (const auto& entity : web_page->entities) {
    if (entity->type != kProductType) {
      continue;
    }

    ParseSearchResultAdMapEntityProperties(entity, &search_result_ads);
  }

  LogSearchResultAdMap(search_result_ads);
  return search_result_ads;
}

}  // namespace brave_ads
