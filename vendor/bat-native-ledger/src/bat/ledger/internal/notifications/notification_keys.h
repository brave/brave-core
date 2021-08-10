/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_NOTIFICATIONS_NOTIFICATION_KEYS_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_NOTIFICATIONS_NOTIFICATION_KEYS_H_

namespace ledger {
namespace notifications {

const char kWalletDeviceLimitReached[] = "wallet_device_limit_reached";
const char kWalletMismatchedProviderAccounts[] =
    "wallet_mismatched_provider_accounts";
const char kWalletDisconnected[] = "wallet_disconnected";
const char kWalletNewVerified[] = "wallet_new_verified";

}  // namespace notifications
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_NOTIFICATIONS_NOTIFICATION_KEYS_H_
