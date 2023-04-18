/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_PROMOTED_CONTENT_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_PROMOTED_CONTENT_AD_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/promoted_content_ads/promoted_content_ad_event_handler_delegate.h"

namespace brave_ads {

class Account;
class Transfer;
struct PromotedContentAdInfo;

class PromotedContentAd final
    : public promoted_content_ads::EventHandlerDelegate {
 public:
  PromotedContentAd(Account* account, Transfer* transfer);

  PromotedContentAd(const PromotedContentAd&) = delete;
  PromotedContentAd& operator=(const PromotedContentAd&) = delete;

  PromotedContentAd(PromotedContentAd&&) noexcept = delete;
  PromotedContentAd& operator=(PromotedContentAd&&) noexcept = delete;

  ~PromotedContentAd() override;

  void TriggerEvent(const std::string& placement_id,
                    const std::string& creative_instance_id,
                    mojom::PromotedContentAdEventType event_type);

 private:
  // promoted_content_ads::EventHandlerDelegate:
  void OnPromotedContentAdViewed(const PromotedContentAdInfo& ad) override;
  void OnPromotedContentAdClicked(const PromotedContentAdInfo& ad) override;

  promoted_content_ads::EventHandler event_handler_;

  const raw_ptr<Account> account_ = nullptr;    // NOT OWNED
  const raw_ptr<Transfer> transfer_ = nullptr;  // NOT OWNED
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_PROMOTED_CONTENT_AD_HANDLER_H_
