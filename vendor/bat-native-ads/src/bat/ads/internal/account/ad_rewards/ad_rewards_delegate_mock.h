/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_AD_REWARDS_DELEGATE_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_AD_REWARDS_DELEGATE_MOCK_H_

#include "bat/ads/internal/account/ad_rewards/ad_rewards_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ads {

class AdRewardsDelegateMock : public AdRewardsDelegate {
 public:
  AdRewardsDelegateMock();
  ~AdRewardsDelegateMock() override;

  AdRewardsDelegateMock(const AdRewardsDelegateMock&) = delete;
  AdRewardsDelegateMock& operator=(const AdRewardsDelegateMock&) = delete;

  MOCK_METHOD(void, OnDidReconcileAdRewards, ());

  MOCK_METHOD(void, OnFailedToReconcileAdRewards, ());

  MOCK_METHOD(void, OnWillRetryToReconcileAdRewards, ());

  MOCK_METHOD(void, OnDidRetryToReconcileAdRewards, ());
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_AD_REWARDS_AD_REWARDS_DELEGATE_MOCK_H_
