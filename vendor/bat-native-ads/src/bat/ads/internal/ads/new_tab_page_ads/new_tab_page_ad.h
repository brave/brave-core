/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_H_
#define BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_H_

#include <string>

#include "bat/ads/mojom.h"

namespace ads {

class AdsImpl;

class NewTabPageAd {
 public:
  NewTabPageAd(
      AdsImpl* ads);

  ~NewTabPageAd();

  void Trigger(
    const std::string& wallpaper_id,
    const std::string& creative_instance_id,
    const NewTabPageAdEventType event_type);

 private:
  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_ADS_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_H_
