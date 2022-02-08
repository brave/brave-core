/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_UTILS_H_

#include <string>
#include <vector>

namespace brave_wallet {

// Encode uint16_t value into 1-3 bytes compact-u16.
// See
// https://docs.solana.com/developing/programming-model/transactions#compact-u16-format
void CompactU16Encode(uint16_t u16, std::vector<uint8_t>* compact_u16);

// A bridge function to call DecodeBase58 in bitcoin-core.
bool Base58Decode(const std::string& str,
                  std::vector<uint8_t>* ret,
                  int max_ret_len);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_UTILS_H_
