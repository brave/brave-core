/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_AD_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "bat/ads/ads_callback.h"
#include "bat/ads/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_handler_observer.h"
#include "bat/ads/internal/ads/serving/inline_content_ad_serving_observer.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads {

namespace geographic {
class SubdivisionTargeting;
}  // namespace geographic

namespace inline_content_ads {
class EventHandler;
class Serving;
}  // namespace inline_content_ads

namespace resource {
class AntiTargeting;
}  // namespace resource

class Account;
class Transfer;
struct InlineContentAdInfo;

class InlineContentAd final : public inline_content_ads::EventHandlerObserver,
                              public inline_content_ads::ServingObserver {
 public:
  InlineContentAd(Account* account,
                  Transfer* transfer,
                  geographic::SubdivisionTargeting* subdivision_targeting,
                  resource::AntiTargeting* anti_targeting_resource);

  InlineContentAd(const InlineContentAd& other) = delete;
  InlineContentAd& operator=(const InlineContentAd& other) = delete;

  InlineContentAd(InlineContentAd&& other) noexcept = delete;
  InlineContentAd& operator=(InlineContentAd&& other) noexcept = delete;

  ~InlineContentAd() override;

  void MaybeServe(const std::string& dimensions,
                  MaybeServeInlineContentAdCallback callback);

  void TriggerEvent(const std::string& placement_id,
                    const std::string& creative_instance_id,
                    mojom::InlineContentAdEventType event_type);

 private:
  // inline_content_ads::ServingObserver:
  void OnOpportunityAroseToServeInlineContentAd(
      const SegmentList& segments) override;
  void OnDidServeInlineContentAd(const InlineContentAdInfo& ad) override;

  // inline_content_ads::EventHandlerObserver:
  void OnInlineContentAdServed(const InlineContentAdInfo& ad) override;
  void OnInlineContentAdViewed(const InlineContentAdInfo& ad) override;
  void OnInlineContentAdClicked(const InlineContentAdInfo& ad) override;

  std::unique_ptr<inline_content_ads::EventHandler> event_handler_;

  std::unique_ptr<inline_content_ads::Serving> serving_;

  const raw_ptr<Account> account_ = nullptr;    // NOT OWNED
  const raw_ptr<Transfer> transfer_ = nullptr;  // NOT OWNED
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_INLINE_CONTENT_AD_H_
