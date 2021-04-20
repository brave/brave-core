/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_CONSTANTS_H_

namespace brave_rewards {

constexpr char kBatPaymentMethod[] = "bat";

namespace errors {

constexpr char kBatTransactionFailed[] = "BAT transaction failed";
constexpr char kBraveRewardsNotEnabled[] =
   "Brave rewards should be enabled to use Pay with BAT";
constexpr char kInsufficientBalance[] = "Insufficient Balance";
constexpr char kInvalidData[] = "Invalid data in payment request";
constexpr char kInvalidPublisher[] = "Unverified publisher";
constexpr char kInvalidRenderer[] = "Renderer not found";
constexpr char kRewardsNotInitialized[] = "Brave rewards is not initialized";
constexpr char kTransactionCancelled[] = "Transaction cancelled";
constexpr char kUnverifiedUserWallet[]  = "Unverified user wallet";

}  // namespace errors

}  // namespace brave_rewards

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_COMMON_CONSTANTS_H_
