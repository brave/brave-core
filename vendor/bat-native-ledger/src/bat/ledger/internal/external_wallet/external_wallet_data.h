/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_EXTERNAL_WALLET_DATA_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_EXTERNAL_WALLET_DATA_H_

#include <iostream>
#include <string>

#include "bat/ledger/internal/core/enum_string.h"
#include "bat/ledger/public/interfaces/ledger.mojom.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ledger {

enum class ExternalWalletProvider { kUphold, kGemini, kBitflyer };

std::string StringifyEnum(ExternalWalletProvider value);

absl::optional<ExternalWalletProvider> ParseEnum(
    const EnumString<ExternalWalletProvider>& s);

struct ExternalWallet {
  ExternalWalletProvider provider;
  std::string address;
  std::string access_token;
};

absl::optional<ExternalWallet> ExternalWalletFromMojoStruct(
    const mojom::ExternalWallet& wallet);

struct ExternalWalletTransferResult {
  ExternalWalletProvider provider;
  std::string transaction_id;
};

}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_EXTERNAL_WALLET_EXTERNAL_WALLET_DATA_H_
