/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_ORDER_EXTRACTION_PATTERNS_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_ORDER_EXTRACTION_PATTERNS_RESOURCE_H_

#include <optional>
#include <string>

#include "base/scoped_observation.h"
#include "base/types/optional_ref.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_load_state_types.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/resource/order_extraction_patterns_resource_info.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace base {
class DictValue;
}  // namespace base

namespace brave_ads {

class AdsClient;

// Manages the lifecycle of the `conversions.order` extraction patterns
// resource, including loading from persisted preferences and observing updates.
class OrderExtractionPatternsResource final : public AdsClientNotifierObserver {
 public:
  OrderExtractionPatternsResource();

  OrderExtractionPatternsResource(const OrderExtractionPatternsResource&) =
      delete;
  OrderExtractionPatternsResource& operator=(
      const OrderExtractionPatternsResource&) = delete;

  ~OrderExtractionPatternsResource() override;

  ResourceLoadStateType GetLoadState() const { return load_state_type_; }

  // Returns the loaded resource's version, or `std::nullopt` if no resource
  // is loaded.
  std::optional<std::string> GetManifestVersion() const;

  // Returns the loaded patterns, or the built-in default patterns if no
  // resource is loaded.
  const OrderExtractionPatternsInfo& GetPatterns() const;

 private:
  // Loads the resource from the last pref-persisted delivery, if any.
  void LoadFromPref();

  // Resets the resource and clears the pref, since the content it was persisted
  // for is no longer available.
  void ResetResourceAndPref();

  // AdsClientNotifierObserver:
  void OnNotifySponsoredContentComponentDidChange(
      base::optional_ref<const base::DictValue> value) override;
  void OnNotifyDidUnregisterSponsoredContentComponent() override;

  std::optional<OrderExtractionPatternsResourceInfo> resource_;

  ResourceLoadStateType load_state_type_ = ResourceLoadStateType::kNotLoaded;

  base::ScopedObservation<AdsClient, AdsClientNotifierObserver>
      ads_client_observation_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_RESOURCE_ORDER_EXTRACTION_PATTERNS_RESOURCE_H_
