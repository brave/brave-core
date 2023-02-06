/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_UNITTEST_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_UNITTEST_UTIL_H_

#include <string>

namespace ads {

struct WalletInfo;

std::string GetWalletPaymentIdForTesting();
std::string GetWalletRecoverySeedForTesting();
std::string GetInvalidWalletRecoverySeedForTesting();

WalletInfo GetWalletForTesting();

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_WALLET_WALLET_UNITTEST_UTIL_H_
