/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NEW_TAB_PAGE_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NEW_TAB_PAGE_AD_HANDLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/ads_callback.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_observer.h"
#include "brave/components/brave_ads/core/internal/ads/serving/new_tab_page_ad_serving_delegate.h"

namespace brave_ads {

namespace new_tab_page_ads {
class EventHandler;
class Serving;
}  // namespace new_tab_page_ads

namespace resource {
class AntiTargeting;
}  // namespace resource

class Account;
class Transfer;
class SubdivisionTargeting;
struct NewTabPageAdInfo;

class NewTabPageAdHandler final : public new_tab_page_ads::EventHandlerObserver,
                                  public new_tab_page_ads::ServingDelegate {
 public:
  NewTabPageAdHandler(Account* account,
                      Transfer* transfer,
                      const SubdivisionTargeting& subdivision_targeting,
                      const resource::AntiTargeting& anti_targeting_resource);

  NewTabPageAdHandler(const NewTabPageAdHandler&) = delete;
  NewTabPageAdHandler& operator=(const NewTabPageAdHandler&) = delete;

  NewTabPageAdHandler(NewTabPageAdHandler&&) noexcept = delete;
  NewTabPageAdHandler& operator=(NewTabPageAdHandler&&) noexcept = delete;

  ~NewTabPageAdHandler() override;

  void MaybeServe(MaybeServeNewTabPageAdCallback callback);

  void TriggerEvent(const std::string& placement_id,
                    const std::string& creative_instance_id,
                    mojom::NewTabPageAdEventType event_type);

 private:
  // new_tab_page_ads::ServingDelegate:
  void OnOpportunityAroseToServeNewTabPageAd(
      const SegmentList& segments) override;
  void OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) override;

  // new_tab_page_ads::EventHandlerObserver:
  void OnNewTabPageAdServed(const NewTabPageAdInfo& ad) override;
  void OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) override;
  void OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) override;

  std::unique_ptr<new_tab_page_ads::EventHandler> event_handler_;

  std::unique_ptr<new_tab_page_ads::Serving> serving_;

  const raw_ptr<Account> account_ = nullptr;    // NOT OWNED
  const raw_ptr<Transfer> transfer_ = nullptr;  // NOT OWNED
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_NEW_TAB_PAGE_AD_HANDLER_H_
