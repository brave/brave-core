/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_H_

#include <string>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/creatives/promoted_content_ads/creative_promoted_content_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/promoted_content_ads/promoted_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

using FirePromotedContentAdEventHandlerCallback = base::OnceCallback<void(
    bool success,
    const std::string& placement_id,
    const mojom::PromotedContentAdEventType event_tmojom_ad_event_typeype)>;

struct CreativePromotedContentAdInfo;
struct PromotedContentAdInfo;

class PromotedContentAdEventHandler final
    : public PromotedContentAdEventHandlerDelegate {
 public:
  PromotedContentAdEventHandler();

  PromotedContentAdEventHandler(const PromotedContentAdEventHandler&) = delete;
  PromotedContentAdEventHandler& operator=(
      const PromotedContentAdEventHandler&) = delete;

  PromotedContentAdEventHandler(PromotedContentAdEventHandler&&) noexcept =
      delete;
  PromotedContentAdEventHandler& operator=(
      PromotedContentAdEventHandler&&) noexcept = delete;

  ~PromotedContentAdEventHandler() override;

  void SetDelegate(PromotedContentAdEventHandlerDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 mojom::PromotedContentAdEventType mojom_ad_event_type,
                 FirePromotedContentAdEventHandlerCallback callback);

 private:
  void GetForCreativeInstanceIdCallback(
      const std::string& placement_id,
      mojom::PromotedContentAdEventType mojom_ad_event_type,
      FirePromotedContentAdEventHandlerCallback callback,
      bool success,
      const std::string& creative_instance_id,
      const CreativePromotedContentAdInfo& creative_ad);
  void GetForTypeCallback(const PromotedContentAdInfo& ad,
                          mojom::PromotedContentAdEventType mojom_ad_event_type,
                          FirePromotedContentAdEventHandlerCallback callback,
                          bool success,
                          const AdEventList& ad_events);
  void FireEventCallback(const PromotedContentAdInfo& ad,
                         mojom::PromotedContentAdEventType mojom_ad_event_type,
                         FirePromotedContentAdEventHandlerCallback callback,
                         bool success) const;

  void SuccessfullyFiredEvent(
      const PromotedContentAdInfo& ad,
      mojom::PromotedContentAdEventType mojom_ad_event_type,
      FirePromotedContentAdEventHandlerCallback callback) const;
  void FailedToFireEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType mojom_ad_event_type,
      FirePromotedContentAdEventHandlerCallback callback) const;

  void NotifyDidFirePromotedContentAdEvent(
      const PromotedContentAdInfo& ad,
      mojom::PromotedContentAdEventType mojom_ad_event_type) const;
  void NotifyFailedToFirePromotedContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::PromotedContentAdEventType mojom_ad_event_type) const;

  raw_ptr<PromotedContentAdEventHandlerDelegate> delegate_ =
      nullptr;  // Not owned.

  const database::table::CreativePromotedContentAds
      creative_ads_database_table_;

  const database::table::AdEvents ad_events_database_table_;

  base::WeakPtrFactory<PromotedContentAdEventHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_PROMOTED_CONTENT_ADS_PROMOTED_CONTENT_AD_EVENT_HANDLER_H_
