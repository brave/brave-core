/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_AD_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "bat/ads/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler_observer.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads {

namespace promoted_content_ads {
class EventHandler;
}  // namespace promoted_content_ads

class Account;
class Transfer;
struct PromotedContentAdInfo;

class PromotedContentAd final
    : public promoted_content_ads::EventHandlerObserver {
 public:
  PromotedContentAd(Account* account, Transfer* transfer);

  PromotedContentAd(const PromotedContentAd& other) = delete;
  PromotedContentAd& operator=(const PromotedContentAd& other) = delete;

  PromotedContentAd(PromotedContentAd&& other) noexcept = delete;
  PromotedContentAd& operator=(PromotedContentAd&& other) noexcept = delete;

  ~PromotedContentAd() override;

  void TriggerEvent(const std::string& placement_id,
                    const std::string& creative_instance_id,
                    mojom::PromotedContentAdEventType event_type);

 private:
  // promoted_content_ads::EventHandlerObserver:
  void OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) override;
  void OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) override;

  std::unique_ptr<promoted_content_ads::EventHandler> event_handler_;

  const raw_ptr<Account> account_ = nullptr;    // NOT OWNED
  const raw_ptr<Transfer> transfer_ = nullptr;  // NOT OWNED
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_PROMOTED_CONTENT_AD_H_
