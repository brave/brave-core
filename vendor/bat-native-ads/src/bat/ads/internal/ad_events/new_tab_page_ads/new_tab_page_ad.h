/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/internal/ad_events/new_tab_page_ads/new_tab_page_ad_observer.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

struct NewTabPageAdInfo;

class NewTabPageAd final : public NewTabPageAdObserver {
 public:
  NewTabPageAd();
  ~NewTabPageAd() override;
  NewTabPageAd(const NewTabPageAd&) = delete;
  NewTabPageAd& operator=(const NewTabPageAd&) = delete;

  void AddObserver(NewTabPageAdObserver* observer);
  void RemoveObserver(NewTabPageAdObserver* observer);

  void FireEvent(const std::string& placement_id,
                 const std::string& creative_instance_id,
                 const mojom::NewTabPageAdEventType event_type);

 private:
  void FireEvent(const NewTabPageAdInfo& ad,
                 const std::string& placement_id,
                 const std::string& creative_instance_id,
                 const mojom::NewTabPageAdEventType event_type);

  void NotifyNewTabPageAdEvent(
      const NewTabPageAdInfo& ad,
      const mojom::NewTabPageAdEventType event_type) const;
  void NotifyNewTabPageAdServed(const NewTabPageAdInfo& ad) const;
  void NotifyNewTabPageAdViewed(const NewTabPageAdInfo& ad) const;
  void NotifyNewTabPageAdClicked(const NewTabPageAdInfo& ad) const;

  void NotifyNewTabPageAdEventFailed(
      const std::string& placement_id,
      const std::string& creative_instance_id,
      const mojom::NewTabPageAdEventType event_type) const;

  base::ObserverList<NewTabPageAdObserver> observers_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_EVENTS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_H_
