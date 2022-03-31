/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_OBSERVER_H_

#include "base/observer_list.h"
#include "base/observer_list_types.h"

namespace brave_ads {

class AdsServiceObserver : public base::CheckedObserver {
 public:
  virtual void OnAdRewardsChanged() = 0;

 protected:
  ~AdsServiceObserver() override = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_BROWSER_ADS_SERVICE_OBSERVER_H_
