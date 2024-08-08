/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_INLINE_CONTENT_AD_INLINE_CONTENT_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_INLINE_CONTENT_AD_INLINE_CONTENT_AD_HANDLER_H_

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/tabs/tab_manager_observer.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/inline_content_ads/inline_content_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/inline_content_ads/inline_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class AntiTargetingResource;
class SiteVisit;
class SubdivisionTargeting;
struct InlineContentAdInfo;
struct TabInfo;

class InlineContentAdHandler final : public InlineContentAdEventHandlerDelegate,
                                     public InlineContentAdServingDelegate,
                                     public TabManagerObserver {
 public:
  InlineContentAdHandler(SiteVisit& site_visit,
                         const SubdivisionTargeting& subdivision_targeting,
                         const AntiTargetingResource& anti_targeting_resource);

  InlineContentAdHandler(const InlineContentAdHandler&) = delete;
  InlineContentAdHandler& operator=(const InlineContentAdHandler&) = delete;

  InlineContentAdHandler(InlineContentAdHandler&&) noexcept = delete;
  InlineContentAdHandler& operator=(InlineContentAdHandler&&) noexcept = delete;

  ~InlineContentAdHandler() override;

  void MaybeServe(const std::string& dimensions,
                  MaybeServeInlineContentAdCallback callback);

  void TriggerEvent(const std::string& placement_id,
                    const std::string& creative_instance_id,
                    mojom::InlineContentAdEventType event_type,
                    TriggerAdEventCallback callback);

 private:
  void MaybeServeCallback(MaybeServeInlineContentAdCallback callback,
                          const std::string& dimensions,
                          const std::optional<InlineContentAdInfo>& ad);

  void CacheAdPlacement(int32_t tab_id, const InlineContentAdInfo& ad);
  void PurgeOrphanedCachedAdPlacements(int32_t tab_id);

  // InlineContentAdServingDelegate:
  void OnOpportunityAroseToServeInlineContentAd() override;
  void OnDidServeInlineContentAd(int32_t tab_id,
                                 const InlineContentAdInfo& ad) override;

  // InlineContentAdEventHandlerDelegate:
  void OnDidFireInlineContentAdServedEvent(
      const InlineContentAdInfo& ad) override;
  void OnDidFireInlineContentAdViewedEvent(
      const InlineContentAdInfo& ad) override;
  void OnDidFireInlineContentAdClickedEvent(
      const InlineContentAdInfo& ad) override;

  // TabManagerObserver:
  void OnTabDidChange(const TabInfo& tab) override;
  void OnDidCloseTab(int32_t tab_id) override;

  InlineContentAdEventHandler event_handler_;

  const raw_ref<SiteVisit> site_visit_;

  InlineContentAdServing serving_;

  std::map</*tab_id*/ int32_t, std::vector<std::string>> placement_ids_;

  base::WeakPtrFactory<InlineContentAdHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_INLINE_CONTENT_AD_INLINE_CONTENT_AD_HANDLER_H_
