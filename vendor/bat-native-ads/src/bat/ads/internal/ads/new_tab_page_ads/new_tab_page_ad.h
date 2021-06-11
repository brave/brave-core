/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_H_

#include <string>

#include "bat/ads/internal/ad_events/ad_event_info.h"
#include "bat/ads/internal/ads/new_tab_page_ads/new_tab_page_ad_observer.h"
#include "bat/ads/mojom.h"

namespace ads {

struct NewTabPageAdInfo;

class NewTabPageAd : public NewTabPageAdObserver {
 public:
  NewTabPageAd();

  ~NewTabPageAd() override;

  void AddObserver(NewTabPageAdObserver* observer);
  void RemoveObserver(NewTabPageAdObserver* observer);

  void FireEvent(const std::string& uuid,
                 const std::string& creative_instance_id,
                 const NewTabPageAdEventType event_type);

 private:
  base::ObserverList<NewTabPageAdObserver> observers_;

  void FireEvent(const NewTabPageAdInfo& ad,
                 const std::string& uuid,
                 const std::string& creative_instance_id,
                 const NewTabPageAdEventType event_type);

  void NotifyNewTabPageAdEvent(const NewTabPageAdInfo& ad,
                               const NewTabPageAdEventType event_type) const;

  void NotifyNewTabPageAdServed(const NewTabPageAdInfo& ad) const;
  void NotifyNewTabPageAdViewed(const NewTabPageAdInfo& ad) const;
  void NotifyNewTabPageAdClicked(const NewTabPageAdInfo& ad) const;

  void NotifyNewTabPageAdEventFailed(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const NewTabPageAdEventType event_type) const;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_H_
