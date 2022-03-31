/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_SERVING_OBSERVER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_SERVING_OBSERVER_H_

#include "base/observer_list_types.h"

namespace ads {

struct NewTabPageAdInfo;

class NewTabPageAdServingObserver : public base::CheckedObserver {
 public:
  // Invoked when a new tab page ad is served
  virtual void OnDidServeNewTabPageAd(const NewTabPageAdInfo& ad) {}

  // Invoked when a new tab page ad fails to serve
  virtual void OnFailedToServeNewTabPageAd() {}

 protected:
  ~NewTabPageAdServingObserver() override = default;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_SERVING_NEW_TAB_PAGE_ADS_NEW_TAB_PAGE_AD_SERVING_OBSERVER_H_
