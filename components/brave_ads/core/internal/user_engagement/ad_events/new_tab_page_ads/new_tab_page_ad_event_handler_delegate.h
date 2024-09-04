/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_DELEGATE_H_

#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct NewTabPageAdInfo;

class NewTabPageAdEventHandlerDelegate {
 public:
  // Invoked when the new tab page `ad` is served.
  virtual void OnDidFireNewTabPageAdServedEvent(const NewTabPageAdInfo& ad) {}

  // Invoked when the new tab page `ad` is viewed.
  virtual void OnDidFireNewTabPageAdViewedEvent(const NewTabPageAdInfo& ad) {}

  // Invoked when the new tab page `ad` is clicked.
  virtual void OnDidFireNewTabPageAdClickedEvent(const NewTabPageAdInfo& ad) {}

  // Invoked when the new tab page video `ad` started playing.
  virtual void OnDidFireNewTabPageAdMediaPlayEvent(const NewTabPageAdInfo& ad) {
  }

  // Invoked after playing 25% of the new tab page video `ad`.
  virtual void OnDidFireNewTabPageAdMedia25Event(const NewTabPageAdInfo& ad) {}

  // Invoked after playing 100% of the new tab page video `ad`.
  virtual void OnDidFireNewTabPageAdMedia100Event(const NewTabPageAdInfo& ad) {}

  // Invoked when the new tab page ad event fails for `placement_id`,
  // `creative_instance_id` and `mojom_ad_event_type`.
  virtual void OnFailedToFireNewTabPageAdEvent(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::NewTabPageAdEventType mojom_ad_event_type) {}

 protected:
  virtual ~NewTabPageAdEventHandlerDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_USER_ENGAGEMENT_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_DELEGATE_H_
