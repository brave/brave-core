/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_REWARDS_USER_REWARDS_DELEGATE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_REWARDS_USER_REWARDS_DELEGATE_H_

namespace brave_ads {

class UserRewardsDelegate {
 public:
  // Invoked to tell the delegate that migration of the verified rewards user
  // was successful.
  virtual void OnDidMigrateVerifiedRewardsUser() {}

 protected:
  virtual ~UserRewardsDelegate() = default;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_USER_REWARDS_USER_REWARDS_DELEGATE_H_
