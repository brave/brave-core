/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_HANDLER_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/types/optional_ref.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving.h"
#include "brave/components/brave_ads/core/internal/serving/new_tab_page_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"

namespace brave_ads {

class AntiTargetingResource;
class SiteVisit;
class SubdivisionTargeting;
struct NewTabPageAdInfo;

class NewTabPageAdHandler final : public NewTabPageAdEventHandlerDelegate,
                                  public NewTabPageAdServingDelegate {
 public:
  NewTabPageAdHandler(SiteVisit& site_visit,
                      const SubdivisionTargeting& subdivision_targeting,
                      const AntiTargetingResource& anti_targeting_resource);

  NewTabPageAdHandler(const NewTabPageAdHandler&) = delete;
  NewTabPageAdHandler& operator=(const NewTabPageAdHandler&) = delete;

  NewTabPageAdHandler(NewTabPageAdHandler&&) noexcept = delete;
  NewTabPageAdHandler& operator=(NewTabPageAdHandler&&) noexcept = delete;

  ~NewTabPageAdHandler() override;

  void MaybeServe(MaybeServeNewTabPageAdCallback callback);

  void TriggerEvent(const std::string& placement_id,
                    const std::string& creative_instance_id,
                    mojom::NewTabPageAdEventType mojom_ad_event_type,
                    TriggerAdEventCallback callback);

 private:
  void MaybeServeCallback(MaybeServeNewTabPageAdCallback callback,
                          base::optional_ref<const NewTabPageAdInfo> ad);

  void TriggerServedEventCallback(
      const std::string& creative_instance_id,
      TriggerAdEventCallback callback,
      bool success,
      const std::string& placement_id,
      mojom::NewTabPageAdEventType mojom_ad_event_type);

  // NewTabPageAdServingDelegate:
  void OnOpportunityAroseToServeNewTabPageAd() override;
  void OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) override;

  // NewTabPageAdEventHandlerDelegate:
  void OnDidFireNewTabPageAdServedEvent(const NewTabPageAdInfo& ad) override;
  void OnDidFireNewTabPageAdViewedEvent(const NewTabPageAdInfo& ad) override;
  void OnDidFireNewTabPageAdClickedEvent(const NewTabPageAdInfo& ad) override;
  void OnDidFireNewTabPageAdMediaPlayEvent(const NewTabPageAdInfo& ad) override;
  void OnDidFireNewTabPageAdMedia25Event(const NewTabPageAdInfo& ad) override;
  void OnDidFireNewTabPageAdMedia100Event(const NewTabPageAdInfo& ad) override;

  NewTabPageAdEventHandler event_handler_;

  const raw_ref<SiteVisit> site_visit_;

  NewTabPageAdServing serving_;

  base::WeakPtrFactory<NewTabPageAdHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_AD_UNITS_NEW_TAB_PAGE_AD_NEW_TAB_PAGE_AD_HANDLER_H_
