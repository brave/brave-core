/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_H_

#include <string>

#include "base/check_op.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/ad_event_info.h"
#include "brave/components/brave_ads/core/internal/ads/ad_events/new_tab_page_ads/new_tab_page_ad_event_handler_delegate.h"

namespace brave_ads {

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
    DCHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 mojom::NewTabPageAdEventType event_type);

 private:
  void OnGetForCreativeInstanceId(const std::string& placement_id,
                                  mojom::NewTabPageAdEventType event_type,
                                  bool success,
                                  const std::string& creative_instance_id,
                                  const CreativeNewTabPageAdInfo& creative_ad);

  void FireEvent(const NewTabPageAdInfo& ad,
                 mojom::NewTabPageAdEventType event_type);
  void OnGetAdEvents(const NewTabPageAdInfo& ad,
                     mojom::NewTabPageAdEventType event_type,
                     bool success,
                     const AdEventList& ad_events);

  void SuccessfullyFiredEvent(const NewTabPageAdInfo& ad,
                              mojom::NewTabPageAdEventType event_type) const;
  void FailedToFireEvent(const std::string& placement_id,
                         const std::string& creative_instance_id,
                         mojom::NewTabPageAdEventType event_type) const;

  raw_ptr<NewTabPageAdEventHandlerDelegate> delegate_ = nullptr;

  base::WeakPtrFactory<NewTabPageAdEventHandler> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_H_
