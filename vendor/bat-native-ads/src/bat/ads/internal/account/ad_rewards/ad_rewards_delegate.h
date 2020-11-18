/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_AD_REWARDS_AD_REWARDS_DELEGATE_H_
#define BAT_ADS_INTERNAL_AD_REWARDS_AD_REWARDS_DELEGATE_H_

namespace ads {

class AdRewardsDelegate {
 public:
  virtual ~AdRewardsDelegate() = default;

  virtual void OnDidReconcileAdRewards() = 0;

  virtual void OnFailedToReconcileAdRewards() {}
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_AD_REWARDS_AD_REWARDS_DELEGATE_H_
