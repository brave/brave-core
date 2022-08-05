/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/search_result_ad/search_result_ad_parsing.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/containers/fixed_flat_set.h"
#include "base/containers/flat_set.h"
#include "base/strings/string_number_conversions.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using ads::mojom::Conversion;
using ads::mojom::ConversionPtr;
using ads::mojom::SearchResultAd;
using ads::mojom::SearchResultAdPtr;

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

  *out_value = ad_property->values->get_long_values().front();

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

  std::string value = ad_property->values->get_string_values().front();
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

  std::string value = ad_property->values->get_string_values().front();
  GURL url(value);
  if (!url.is_valid() || !url.SchemeIs(url::kHttpsScheme)) {
    return false;
  }

  *out_value = url;
  return true;
}

bool SetSearchAdProperty(const schema_org::mojom::PropertyPtr& ad_property,
                         SearchResultAd* search_result_ad) {
  DCHECK(ad_property);
  DCHECK(search_result_ad);
  DCHECK(search_result_ad->conversion);

  const std::string& name = ad_property->name;
  if (name == kDataPlacementId) {
    return GetStringValue(ad_property, &search_result_ad->placement_id);
  } else if (name == kDataCreativeInstanceId) {
    return GetStringValue(ad_property, &search_result_ad->creative_instance_id);
  } else if (name == kDataCreativeSetId) {
    return GetStringValue(ad_property, &search_result_ad->creative_set_id);
  } else if (name == kDataCampaignId) {
    return GetStringValue(ad_property, &search_result_ad->campaign_id);
  } else if (name == kDataAdvertiserId) {
    return GetStringValue(ad_property, &search_result_ad->advertiser_id);
  } else if (name == kDataLandingPage) {
    return GetUrlValue(ad_property, &search_result_ad->target_url);
  } else if (name == kDataHeadlineText) {
    return GetStringValue(ad_property, &search_result_ad->headline_text);
  } else if (name == kDataDescription) {
    return GetStringValue(ad_property, &search_result_ad->description);
  } else if (name == kDataRewardsValue) {
    return GetDoubleValue(ad_property, &search_result_ad->value);
  } else if (name == kDataConversionTypeValue) {
    return GetStringValue(ad_property, &search_result_ad->conversion->type);
  } else if (name == kDataConversionUrlPatternValue) {
    return GetStringValue(ad_property,
                          &search_result_ad->conversion->url_pattern);
  } else if (name == kDataConversionAdvertiserPublicKeyValue) {
    return GetStringValue(ad_property,
                          &search_result_ad->conversion->advertiser_public_key);
  } else if (name == kDataConversionObservationWindowValue) {
    return GetIntValue(ad_property,
                       &search_result_ad->conversion->observation_window);
  }

  NOTREACHED();

  return false;
}

absl::optional<SearchResultAdMap> ParseSearchResultAdMapEntityProperties(
    const schema_org::mojom::EntityPtr& entity) {
  DCHECK(entity);
  DCHECK_EQ(entity->type, kProductType);

  SearchResultAdMap search_result_ads;

  for (const auto& property : entity->properties) {
    if (!property || property->name == kContextPropertyName ||
        property->name == kTypePropertyName) {
      continue;
    }

    // Search result ads list product could have only "@context" and "creatives"
    // properties.
    if (property->name != "creatives") {
      return absl::nullopt;
    }

    if (!property->values->is_entity_values() ||
        property->values->get_entity_values().empty()) {
      LOG(ERROR) << "Search result ad attributes list is empty";
      return SearchResultAdMap();
    }

    for (const auto& ad_entity : property->values->get_entity_values()) {
      if (!ad_entity || ad_entity->type != kSearchResultAdType) {
        LOG(ERROR) << "Wrong search result ad type specified: "
                   << ad_entity->type;
        return SearchResultAdMap();
      }

      if (property->name == kTypePropertyName) {
        continue;
      }

      SearchResultAdPtr search_result_ad = SearchResultAd::New();
      search_result_ad->conversion = Conversion::New();

      base::flat_set<base::StringPiece> found_attributes;
      for (const auto& ad_property : ad_entity->properties) {
        // Wrong attribute name
        const auto* it = std::find(kSearchResultAdAttributes.begin(),
                                   kSearchResultAdAttributes.end(),
                                   base::StringPiece(ad_property->name));

        if (it == kSearchResultAdAttributes.end()) {
          LOG(ERROR) << "Wrong search result ad attribute specified: "
                     << ad_property->name;
          return SearchResultAdMap();
        }
        found_attributes.insert(*it);

        if (!SetSearchAdProperty(ad_property, search_result_ad.get())) {
          LOG(ERROR) << "Cannot read search result ad attribute value: "
                     << ad_property->name;
          return SearchResultAdMap();
        }
      }

      // Not all of attributes were specified.
      if (found_attributes.size() != kSearchResultAdAttributes.size()) {
        std::vector<base::StringPiece> absent_attributes;
        std::set_difference(kSearchResultAdAttributes.begin(),
                            kSearchResultAdAttributes.end(),
                            found_attributes.begin(), found_attributes.end(),
                            std::back_inserter(absent_attributes));

        LOG(ERROR) << "Some of search result ad attributes were not specified: "
                   << base::JoinString(absent_attributes, ", ");

        return SearchResultAdMap();
      }

      const std::string creative_instance_id =
          search_result_ad->creative_instance_id;
      search_result_ads.emplace(creative_instance_id,
                                std::move(search_result_ad));
    }

    // Creatives has been parsed.
    break;
  }

  return search_result_ads;
}

void LogSearchResultAdMap(const SearchResultAdMap& search_result_ads) {
  if (!VLOG_IS_ON(1)) {
    return;
  }

  if (search_result_ads.empty()) {
    VLOG(1) << "Parsed search result ads list is empty.";
    return;
  }

  VLOG(1) << "Parsed search result ads list:";
  for (const auto& search_result_ad_pair : search_result_ads) {
    const auto& search_result_ad = search_result_ad_pair.second;
    VLOG(1) << "Ad with \"" << kDataPlacementId
            << "\": " << search_result_ad->placement_id;
    VLOG(1) << "  \"" << kDataCreativeInstanceId
            << "\": " << search_result_ad->creative_instance_id;
    VLOG(1) << "  \"" << kDataCreativeSetId
            << "\": " << search_result_ad->creative_set_id;
    VLOG(1) << "  \"" << kDataCampaignId
            << "\": " << search_result_ad->campaign_id;
    VLOG(1) << "  \"" << kDataAdvertiserId
            << "\": " << search_result_ad->advertiser_id;
    VLOG(1) << "  \"" << kDataLandingPage
            << "\": " << search_result_ad->target_url;
    VLOG(1) << "  \"" << kDataHeadlineText
            << "\": " << search_result_ad->headline_text;
    VLOG(1) << "  \"" << kDataDescription
            << "\": " << search_result_ad->description;
    VLOG(1) << "  \"" << kDataRewardsValue << "\": " << search_result_ad->value;
    VLOG(1) << "  \"" << kDataConversionTypeValue
            << "\": " << search_result_ad->conversion->type;
    VLOG(1) << "  \"" << kDataConversionUrlPatternValue
            << "\": " << search_result_ad->conversion->url_pattern;
    VLOG(1) << "  \"" << kDataConversionAdvertiserPublicKeyValue
            << "\": " << search_result_ad->conversion->advertiser_public_key;
    VLOG(1) << "  \"" << kDataConversionObservationWindowValue
            << "\": " << search_result_ad->conversion->observation_window;
  }
}

}  // namespace

SearchResultAdMap ParseWebPageEntities(blink::mojom::WebPagePtr web_page) {
  for (const auto& entity : web_page->entities) {
    if (entity->type != kProductType) {
      continue;
    }

    absl::optional<SearchResultAdMap> search_result_ads =
        ParseSearchResultAdMapEntityProperties(entity);

    if (search_result_ads) {
      LogSearchResultAdMap(*search_result_ads);
      return std::move(*search_result_ads);
    }
  }

  VLOG(1) << "No search result ad found.";

  return SearchResultAdMap();
}

}  // namespace brave_ads
