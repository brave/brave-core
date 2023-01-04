/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_OBSERVER_H_

#include <string>

#include "base/observer_list_types.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"

namespace ads {

struct NewTabPageAdInfo;

namespace new_tab_page_ads {

class EventHandlerObserver : public base::CheckedObserver {
 public:
  // Invoked when the new tab page |ad| is served.
  virtual void OnNewTabPageAdServed(const NewTabPageAdInfo& ad) {}

  // Invoked when the new tab page |ad| is viewed.
  virtual void OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) {}

  // Invoked when the new tab page |ad| is clicked.
  virtual void OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) {}

  // Invoked when the new tab page |ad| event fails for |placement_id|,
  // |creative_instance_id| and |event_type|.
  virtual void OnNewTabPageAdEventFailed(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::NewTabPageAdEventType event_type) {}
};

}  // namespace new_tab_page_ads
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_EVENT_HANDLER_OBSERVER_H_
