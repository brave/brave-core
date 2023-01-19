/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_UTIL_H_

#include <map>
#include <string>
#include <vector>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace uphold {
const char kUrlStaging[] = "https://wallet-sandbox.uphold.com";
const char kUrlProduction[] = "https://uphold.com";
const char kFeeAddressStaging[] = "1b2b466f-5c15-49bf-995e-c91777d3da93";
const char kFeeAddressProduction[] = "b01e8c55-5004-4761-9e4b-01ec13e25c92";

std::string GetClientId();

std::string GetClientSecret();

std::string GetUrl();

std::string GetFeeAddress();

std::string GetLoginUrl(const std::string& state);

std::string GetAccountUrl();

std::string GetActivityUrl(const std::string& address);

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr);

}  // namespace uphold
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_UPHOLD_UPHOLD_UTIL_H_
