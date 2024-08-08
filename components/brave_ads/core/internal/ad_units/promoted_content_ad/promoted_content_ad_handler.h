/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_PROMOTED_CONTENT_AD_PROMOTED_CONTENT_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_PROMOTED_CONTENT_AD_PROMOTED_CONTENT_AD_HANDLER_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/promoted_content_ads/promoted_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class SiteVisit;
struct PromotedContentAdInfo;

class PromotedContentAdHandler final
    : public PromotedContentAdEventHandlerDelegate {
 public:
  explicit PromotedContentAdHandler(SiteVisit& site_visit);

  PromotedContentAdHandler(const PromotedContentAdHandler&) = delete;
  PromotedContentAdHandler& operator=(const PromotedContentAdHandler&) = delete;

  PromotedContentAdHandler(PromotedContentAdHandler&&) noexcept = delete;
  PromotedContentAdHandler& operator=(PromotedContentAdHandler&&) noexcept =
      delete;

  ~PromotedContentAdHandler() override;

  void TriggerEvent(const std::string& placement_id,
                    const std::string& creative_instance_id,
                    mojom::PromotedContentAdEventType event_type,
                    TriggerAdEventCallback callback);

 private:
  void TriggerServedEventCallback(const std::string& creative_instance_id,
                                  TriggerAdEventCallback callback,
                                  bool success,
                                  const std::string& placement_id,
                                  mojom::PromotedContentAdEventType event_type);

  // PromotedContentAdEventHandlerDelegate:
  void OnDidFirePromotedContentAdServedEvent(
      const PromotedContentAdInfo& ad) override;
  void OnDidFirePromotedContentAdViewedEvent(
      const PromotedContentAdInfo& ad) override;
  void OnDidFirePromotedContentAdClickedEvent(
      const PromotedContentAdInfo& ad) override;

  const raw_ref<SiteVisit> site_visit_;

  PromotedContentAdEventHandler event_handler_;

  base::WeakPtrFactory<PromotedContentAdHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_PROMOTED_CONTENT_AD_PROMOTED_CONTENT_AD_HANDLER_H_
