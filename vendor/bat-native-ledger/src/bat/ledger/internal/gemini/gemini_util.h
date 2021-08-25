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

const char kFeeAddressStaging[] = "60edbc1b-eaa7-4aa0-afb8-a87a89634c55";
const char kFeeAddressProduction[] = "6116ad51-b50d-4e54-bb59-9de559beffdd";
const char kACAddressStaging[] = "60e5e863-8c3d-4341-8b54-23e2695a490c";
const char kACAddressProduction[] = "60e5e863-8c3d-4341-8b54-23e2695a490c";

std::string GetClientId();

std::string GetClientSecret();

std::string GetUrl();

std::string GetFeeAddress();

std::string GetACAddress();

std::string GetAuthorizeUrl(const std::string& state);

std::string GetAddUrl();

std::string GetWithdrawUrl();

std::string GetAccountUrl();

type::ExternalWalletPtr GenerateLinks(type::ExternalWalletPtr wallet);

}  // namespace gemini
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_GEMINI_GEMINI_UTIL_H_
