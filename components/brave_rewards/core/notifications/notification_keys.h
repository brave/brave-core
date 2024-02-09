/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_NOTIFICATIONS_NOTIFICATION_KEYS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_NOTIFICATIONS_NOTIFICATION_KEYS_H_

namespace brave_rewards::internal {
namespace notifications {

const char kUpholdBATNotAllowed[] = "uphold_bat_not_allowed";
const char kUpholdInsufficientCapabilities[] =
    "uphold_insufficient_capabilities";
const char kWalletDisconnected[] = "wallet_disconnected";
const char kSelfCustodyAvailable[] = "self_custody_available";

}  // namespace notifications
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_NOTIFICATIONS_NOTIFICATION_KEYS_H_
