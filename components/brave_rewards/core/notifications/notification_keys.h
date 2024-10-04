/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_NOTIFICATIONS_NOTIFICATION_KEYS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_NOTIFICATIONS_NOTIFICATION_KEYS_H_

namespace brave_rewards::internal {
namespace notifications {

inline constexpr char kUpholdBATNotAllowed[] = "uphold_bat_not_allowed";
inline constexpr char kUpholdInsufficientCapabilities[] =
    "uphold_insufficient_capabilities";
inline constexpr char kWalletDisconnected[] = "wallet_disconnected";
inline constexpr char kSelfCustodyAvailable[] = "self_custody_available";

}  // namespace notifications
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_NOTIFICATIONS_NOTIFICATION_KEYS_H_
