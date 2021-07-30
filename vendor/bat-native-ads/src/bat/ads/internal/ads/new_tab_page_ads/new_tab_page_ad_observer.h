/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_OBSERVER_H_

#include <string>

#include "base/observer_list.h"
#include "bat/ads/mojom.h"

namespace ads {

struct NewTabPageAdInfo;

class NewTabPageAdObserver : public base::CheckedObserver {
 public:
  // Invoked when a new tab page ad is served
  virtual void OnNewTabPageAdServed(const NewTabPageAdInfo& ad) {}

  // Invoked when a new tab page ad is viewed
  virtual void OnNewTabPageAdViewed(const NewTabPageAdInfo& ad) {}

  // Invoked when a new tab page ad is clicked
  virtual void OnNewTabPageAdClicked(const NewTabPageAdInfo& ad) {}

  // Invoked when a new tab page ad event fails
  virtual void OnNewTabPageAdEventFailed(
      const std::string& uuid,
      const std::string& creative_instance_id,
      const NewTabPageAdEventType event_type) {}

 protected:
  ~NewTabPageAdObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_OBSERVER_H_
