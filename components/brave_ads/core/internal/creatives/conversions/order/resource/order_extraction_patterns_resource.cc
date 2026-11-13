/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_resource.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_feature.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_json_reader.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_default.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

OrderExtractionPatternsResource::OrderExtractionPatternsResource() {
  ads_client_observation_.Observe(&GetAdsClient());

  LoadFromPref();
}

OrderExtractionPatternsResource::~OrderExtractionPatternsResource() = default;

std::optional<std::string> OrderExtractionPatternsResource::GetManifestVersion()
    const {
  if (!resource_ || !resource_->version) {
    return std::nullopt;
  }

  return base::NumberToString(*resource_->version);
}

const OrderExtractionPatternsInfo&
OrderExtractionPatternsResource::GetPatterns() const {
  return resource_ ? resource_->patterns : GetDefaultOrderExtractionPatterns();
}

///////////////////////////////////////////////////////////////////////////////

void OrderExtractionPatternsResource::LoadFromPref() {
  std::optional<base::Value> pref_value =
      GetAdsClient().GetProfilePref(prefs::kOrderExtractionPatternsConfig);
  if (!pref_value || !pref_value->is_dict()) {
    return;
  }

  std::optional<OrderExtractionPatternsResourceInfo> resource =
      json::reader::MaybeReadOrderExtractionPatternsResource(
          pref_value->GetDict());
  if (!resource || !resource->version ||
      *resource->version != kOrderExtractionPatternsResourceVersion.Get()) {
    // The pref's version does not match the expected resource version, so it is
    // ignored.
    return;
  }

  load_state_type_ = ResourceLoadStateType::kLoaded;
  resource_ = std::move(resource);
}

void OrderExtractionPatternsResource::ResetResourceAndPref() {
  GetAdsClient().ClearProfilePref(prefs::kOrderExtractionPatternsConfig);
  resource_.reset();
  load_state_type_ = ResourceLoadStateType::kNotLoaded;
}

void OrderExtractionPatternsResource::
    OnNotifySponsoredContentComponentDidChange(
        const std::optional<base::DictValue>& value) {
  if (!value) {
    // The sponsored content component no longer provides this resource, so it
    // is no longer available.
    ResetResourceAndPref();
    return BLOG(1, "Order extraction patterns resource is unavailable");
  }

  std::optional<OrderExtractionPatternsResourceInfo> resource =
      json::reader::MaybeReadOrderExtractionPatternsResource(*value);
  if (!resource) {
    resource_.reset();
    load_state_type_ = ResourceLoadStateType::kFailedToLoad;
    return BLOG(0, "Failed to parse order extraction patterns resource");
  }

  if (!resource->version ||
      *resource->version != kOrderExtractionPatternsResourceVersion.Get()) {
    resource_.reset();
    load_state_type_ = ResourceLoadStateType::kNotLoaded;
    return BLOG(1, "Order extraction patterns resource is unavailable");
  }

  // Persist the resource to the profile pref so it can be loaded on future
  // browser startups before the component is ready again.
  GetAdsClient().SetProfilePref(prefs::kOrderExtractionPatternsConfig,
                                base::Value(value->Clone()));

  load_state_type_ = ResourceLoadStateType::kLoaded;
  resource_ = std::move(resource);

  BLOG(1, "Successfully parsed order extraction patterns resource version "
              << kOrderExtractionPatternsResourceVersion.Get());
}

void OrderExtractionPatternsResource::
    OnNotifyDidUnregisterSponsoredContentComponent() {
  ResetResourceAndPref();
}

}  // namespace brave_ads
