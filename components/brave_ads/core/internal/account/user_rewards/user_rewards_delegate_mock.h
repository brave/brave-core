/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_REWARDS_USER_REWARDS_DELEGATE_MOCK_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_REWARDS_USER_REWARDS_DELEGATE_MOCK_H_

#include "brave/components/brave_ads/core/internal/account/user_rewards/user_rewards_delegate.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_ads {

class UserRewardsDelegateMock : public UserRewardsDelegate {
 public:
  UserRewardsDelegateMock();

  UserRewardsDelegateMock(const UserRewardsDelegateMock&) = delete;
  UserRewardsDelegateMock& operator=(const UserRewardsDelegateMock&) = delete;

  UserRewardsDelegateMock(UserRewardsDelegateMock&&) noexcept = delete;
  UserRewardsDelegateMock& operator=(UserRewardsDelegateMock&&) noexcept =
      delete;

  ~UserRewardsDelegateMock() override;

  MOCK_METHOD(void, OnDidMigrateVerifiedRewardsUser, ());
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_REWARDS_USER_REWARDS_DELEGATE_MOCK_H_
