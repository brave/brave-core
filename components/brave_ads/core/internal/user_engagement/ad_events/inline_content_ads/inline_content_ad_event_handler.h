/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_H_

#include <string>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/creatives/inline_content_ads/creative_inline_content_ads_database_table.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/inline_content_ads/inline_content_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

using FireInlineContentAdEventHandlerCallback = base::OnceCallback<void(
    bool success,
    const std::string& placement_id,
    mojom::InlineContentAdEventType mojom_ad_event_type)>;

struct CreativeInlineContentAdInfo;
struct InlineContentAdInfo;

class InlineContentAdEventHandler final
    : public InlineContentAdEventHandlerDelegate {
 public:
  InlineContentAdEventHandler();

  InlineContentAdEventHandler(const InlineContentAdEventHandler&) = delete;
  InlineContentAdEventHandler& operator=(const InlineContentAdEventHandler&) =
      delete;

  ~InlineContentAdEventHandler() override;

  void SetDelegate(InlineContentAdEventHandlerDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 mojom::InlineContentAdEventType mojom_ad_event_type,
                 FireInlineContentAdEventHandlerCallback callback);

 private:
  void GetForCreativeInstanceIdCallback(
      const std::string& placement_id,
      mojom::InlineContentAdEventType mojom_ad_event_type,
      FireInlineContentAdEventHandlerCallback callback,
      bool success,
      const std::string& creative_instance_id,
      const CreativeInlineContentAdInfo& creative_ad);
  void GetForTypeCallback(const InlineContentAdInfo& ad,
                          mojom::InlineContentAdEventType mojom_ad_event_type,
                          FireInlineContentAdEventHandlerCallback callback,
                          bool success,
                          const AdEventList& ad_events);
  void FireViewedEventCallback(
      const InlineContentAdInfo& ad,
      mojom::InlineContentAdEventType mojom_ad_event_type,
      FireInlineContentAdEventHandlerCallback callback,
      bool success);
  void FireEventCallback(const InlineContentAdInfo& ad,
                         mojom::InlineContentAdEventType mojom_ad_event_type,
                         FireInlineContentAdEventHandlerCallback callback,
                         bool success) const;

  void SuccessfullyFiredEvent(
      const InlineContentAdInfo& ad,
      mojom::InlineContentAdEventType mojom_ad_event_type,
      FireInlineContentAdEventHandlerCallback callback) const;
  void FailedToFireEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType mojom_ad_event_type,
      FireInlineContentAdEventHandlerCallback callback) const;

  void NotifyDidFireInlineContentAdEvent(
      const InlineContentAdInfo& ad,
      mojom::InlineContentAdEventType mojom_ad_event_type) const;
  void NotifyFailedToFireInlineContentAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::InlineContentAdEventType mojom_ad_event_type) const;

  raw_ptr<InlineContentAdEventHandlerDelegate> delegate_ =
      nullptr;  // Not owned.

  const database::table::CreativeInlineContentAds creative_ads_database_table_;

  const database::table::AdEvents ad_events_database_table_;

  base::WeakPtrFactory<InlineContentAdEventHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_INLINE_CONTENT_ADS_INLINE_CONTENT_AD_EVENT_HANDLER_H_
