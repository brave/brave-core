/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_GEMINI_GEMINI_UTIL_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_GEMINI_GEMINI_UTIL_H_

#include <string>

#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace gemini {

const char kFeeAddressStaging[] = "622b9018-f26a-44bf-9a45-3bf3bf3c95e9";
const char kFeeAddressProduction[] = "6116ad51-b50d-4e54-bb59-9de559beffdd";

std::string GetClientId();

std::string GetClientSecret();

std::string GetUrl();

std::string GetFeeAddress();

std::string GetLoginUrl(const std::string& state);

std::string GetAccountUrl();

std::string GetActivityUrl();

mojom::ExternalWalletPtr GenerateLinks(mojom::ExternalWalletPtr);

}  // namespace gemini
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_GEMINI_GEMINI_UTIL_H_
