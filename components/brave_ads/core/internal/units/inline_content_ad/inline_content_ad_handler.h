/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_UNITS_INLINE_CONTENT_AD_INLINE_CONTENT_AD_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_UNITS_INLINE_CONTENT_AD_INLINE_CONTENT_AD_HANDLER_H_

#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving.h"
#include "brave/components/brave_ads/core/internal/serving/inline_content_ad_serving_delegate.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/inline_content_ads/inline_content_ad_event_handler.h"
#include "brave/components/brave_ads/core/internal/user/user_interaction/ad_events/inline_content_ads/inline_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"
#include "brave/components/brave_ads/core/public/ads_callback.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

class Account;
class AntiTargetingResource;
class SubdivisionTargeting;
class Transfer;
struct InlineContentAdInfo;

class InlineContentAdHandler final : public InlineContentAdEventHandlerDelegate,
                                     public InlineContentAdServingDelegate {
 public:
  InlineContentAdHandler(Account& account,
                         Transfer& transfer,
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
                          const absl::optional<InlineContentAdInfo>& ad);

  // InlineContentAdServingDelegate:
  void OnOpportunityAroseToServeInlineContentAd() override;
  void OnDidServeInlineContentAd(const InlineContentAdInfo& ad) override;

  // InlineContentAdEventHandlerDelegate:
  void OnDidFireInlineContentAdServedEvent(
      const InlineContentAdInfo& ad) override;
  void OnDidFireInlineContentAdViewedEvent(
      const InlineContentAdInfo& ad) override;
  void OnDidFireInlineContentAdClickedEvent(
      const InlineContentAdInfo& ad) override;

  InlineContentAdEventHandler event_handler_;

  const raw_ref<Account> account_;
  const raw_ref<Transfer> transfer_;

  InlineContentAdServing serving_;

  base::WeakPtrFactory<InlineContentAdHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_UNITS_INLINE_CONTENT_AD_INLINE_CONTENT_AD_HANDLER_H_
