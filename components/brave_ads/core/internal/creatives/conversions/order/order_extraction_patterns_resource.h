/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_RESOURCE_H_

#include <optional>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_ads/core/internal/common/resources/resource_load_state_types.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_info.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/order/order_extraction_patterns_resource_info.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client_notifier_observer.h"

namespace brave_ads {

class AdsClient;

// Loads and caches the `conversions.order` patterns from
// `conversion-config.json`, delivered in the existing resources component
// alongside `campaigns.json`/`sites.json`; see
// `kOrderExtractionPatternsResourceId`.
class OrderExtractionPatternsResource final : public AdsClientNotifierObserver {
 public:
  OrderExtractionPatternsResource();

  OrderExtractionPatternsResource(const OrderExtractionPatternsResource&) =
      delete;
  OrderExtractionPatternsResource& operator=(
      const OrderExtractionPatternsResource&) = delete;

  ~OrderExtractionPatternsResource() override;

  ResourceLoadStateType GetLoadState() const { return load_state_type_; }

  // Never null; returns the compiled-in English/USD-only default until the
  // component delivers a real set, see `GetLoadState()` to tell which one
  // the caller is getting.
  const OrderExtractionPatternsInfo* GetPatterns() const;

 private:
  void Load();
  void LoadCallback(std::optional<OrderExtractionPatternsResourceInfo> resource,
                    bool exists);

  void Unload();

  // AdsClientNotifierObserver:
  void OnNotifyResourceComponentDidChange(const std::string& manifest_version,
                                          const std::string& id) override;
  void OnNotifyDidUnregisterResourceComponent(const std::string& id) override;

  std::optional<std::string> manifest_version_;

  std::optional<OrderExtractionPatternsResourceInfo> resource_;

  ResourceLoadStateType load_state_type_ = ResourceLoadStateType::kNotLoaded;

  base::ScopedObservation<AdsClient, AdsClientNotifierObserver>
      ads_client_observation_{this};

  base::WeakPtrFactory<OrderExtractionPatternsResource> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_CREATIVES_CONVERSIONS_ORDER_ORDER_EXTRACTION_PATTERNS_RESOURCE_H_
