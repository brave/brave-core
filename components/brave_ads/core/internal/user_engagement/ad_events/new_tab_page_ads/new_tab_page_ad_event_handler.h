/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_H_

#include <string>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-shared.h"

namespace brave_ads {

using FireNewTabPageAdEventHandlerCallback =
    base::OnceCallback<void(bool success,
                            const std::string& placement_id,
                            const mojom::NewTabPageAdEventType event_type)>;

struct CreativeNewTabPageAdInfo;
struct NewTabPageAdInfo;

class NewTabPageAdEventHandler final : public NewTabPageAdEventHandlerDelegate {
 public:
  NewTabPageAdEventHandler();

  NewTabPageAdEventHandler(const NewTabPageAdEventHandler&) = delete;
  NewTabPageAdEventHandler& operator=(const NewTabPageAdEventHandler&) = delete;

  NewTabPageAdEventHandler(NewTabPageAdEventHandler&&) noexcept = delete;
  NewTabPageAdEventHandler& operator=(NewTabPageAdEventHandler&&) noexcept =
      delete;

  ~NewTabPageAdEventHandler() override;

  void SetDelegate(NewTabPageAdEventHandlerDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 mojom::NewTabPageAdEventType event_type,
                 FireNewTabPageAdEventHandlerCallback callback);

 private:
  void GetForCreativeInstanceIdCallback(
      const std::string& placement_id,
      mojom::NewTabPageAdEventType event_type,
      FireNewTabPageAdEventHandlerCallback callback,
      bool success,
      const std::string& creative_instance_id,
      const CreativeNewTabPageAdInfo& creative_ad);
  void GetForTypeCallback(const NewTabPageAdInfo& ad,
                          mojom::NewTabPageAdEventType event_type,
                          FireNewTabPageAdEventHandlerCallback callback,
                          bool success,
                          const AdEventList& ad_events);
  void FireEventCallback(const NewTabPageAdInfo& ad,
                         mojom::NewTabPageAdEventType event_type,
                         FireNewTabPageAdEventHandlerCallback callback,
                         bool success) const;

  void SuccessfullyFiredEvent(
      const NewTabPageAdInfo& ad,
      mojom::NewTabPageAdEventType event_type,
      FireNewTabPageAdEventHandlerCallback callback) const;
  void FailedToFireEvent(const std::string& placement_id,
                         const std::string& creative_instance_id,
                         mojom::NewTabPageAdEventType event_type,
                         FireNewTabPageAdEventHandlerCallback callback) const;

  void NotifyDidFireNewTabPageAdEvent(
      const NewTabPageAdInfo& ad,
      mojom::NewTabPageAdEventType event_type) const;
  void NotifyFailedToFireNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      mojom::NewTabPageAdEventType event_type) const;

  raw_ptr<NewTabPageAdEventHandlerDelegate> delegate_ = nullptr;

  base::WeakPtrFactory<NewTabPageAdEventHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_H_
