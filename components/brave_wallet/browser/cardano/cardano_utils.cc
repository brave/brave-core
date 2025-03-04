/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/cardano/cardano_utils.h"

#include "base/strings/string_number_conversions.h"

namespace brave_wallet {

namespace {
constexpr char kNativeLovelaceToken[] = "lovelace";
}

std::optional<uint64_t> GetLovelaceAmountFromUtxo(
    const cardano_rpc::UnspentOutput& utxo) {
  for (auto& token : utxo.amount) {
    if (token.unit == kNativeLovelaceToken) {
      uint64_t quantity = 0;
      if (base::StringToUint64(token.quantity, &quantity)) {
        return quantity;
      }
    }
  }

  return std::nullopt;
}

}  // namespace brave_wallet
