/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INLINE_CONTENT_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INLINE_CONTENT_AD_HANDLER_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/ads_callback.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/inline_content_ads/inline_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/internal/ads/serving/inline_content_ad_serving_delegate.h"

namespace brave_ads {

namespace inline_content_ads {
class Serving;
}  // namespace inline_content_ads

namespace resource {
class AntiTargeting;
}  // namespace resource

class Account;
class SubdivisionTargeting;
class Transfer;
struct InlineContentAdInfo;

class InlineContentAdHandler final
    : public inline_content_ads::EventHandlerDelegate,
      public inline_content_ads::ServingDelegate {
 public:
  InlineContentAdHandler(
      Account* account,
      Transfer* transfer,
      const SubdivisionTargeting& subdivision_targeting,
      const resource::AntiTargeting& anti_targeting_resource);

  InlineContentAdHandler(const InlineContentAdHandler&) = delete;
  InlineContentAdHandler& operator=(const InlineContentAdHandler&) = delete;

  InlineContentAdHandler(InlineContentAdHandler&&) noexcept = delete;
  InlineContentAdHandler& operator=(InlineContentAdHandler&&) noexcept = delete;

  ~InlineContentAdHandler() override;

  void MaybeServe(const std::string& dimensions,
                  MaybeServeInlineContentAdCallback callback);

  void TriggerEvent(const std::string& placement_id,
                    const std::string& creative_instance_id,
                    mojom::InlineContentAdEventType event_type);

 private:
  // inline_content_ads::ServingDelegate:
  void OnOpportunityAroseToServeInlineContentAd(
      const SegmentList& segments) override;
  void OnDidServeInlineContentAd(const InlineContentAdInfo& ad) override;

  // inline_content_ads::EventHandlerDelegate:
  void OnInlineContentAdServed(const InlineContentAdInfo& ad) override;
  void OnInlineContentAdViewed(const InlineContentAdInfo& ad) override;
  void OnInlineContentAdClicked(const InlineContentAdInfo& ad) override;

  inline_content_ads::EventHandler event_handler_;

  std::unique_ptr<inline_content_ads::Serving> serving_;

  const raw_ptr<Account> account_ = nullptr;    // NOT OWNED
  const raw_ptr<Transfer> transfer_ = nullptr;  // NOT OWNED
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_INLINE_CONTENT_AD_HANDLER_H_
