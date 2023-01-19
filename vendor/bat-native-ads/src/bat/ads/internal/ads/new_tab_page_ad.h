/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_AD_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "bat/ads/ads_callback.h"
#include "bat/ads/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_observer.h"
#include "bat/ads/internal/ads/serving/new_tab_page_ad_serving_observer.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace new_tab_page_ads {
class EventHandler;
class Serving;
}  // namespace new_tab_page_ads

namespace resource {
class AntiTargeting;
}  // namespace resource

class Account;
class Transfer;
struct NewTabPageAdInfo;

class NewTabPageAd final : public new_tab_page_ads::EventHandlerObserver,
                           public new_tab_page_ads::ServingObserver {
 public:
  NewTabPageAd(Account* account,
               Transfer* transfer,
               geographic::SubdivisionTargeting* subdivision_targeting,
               resource::AntiTargeting* anti_targeting_resource);

  NewTabPageAd(const NewTabPageAd& other) = delete;
  NewTabPageAd& operator=(const NewTabPageAd& other) = delete;

  NewTabPageAd(NewTabPageAd&& other) noexcept = delete;
  NewTabPageAd& operator=(NewTabPageAd&& other) noexcept = delete;

  ~NewTabPageAd() override;

  void MaybeServe(MaybeServeNewTabPageAdCallback callback);

  void TriggerEvent(const std::string& placement_id,
                    const std::string& creative_instance_id,
                    mojom::NewTabPageAdEventType event_type);

 private:
  // new_tab_page_ads::ServingObserver:
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

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_AD_H_
