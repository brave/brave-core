/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_RESPONSE_PARSER_H_

#include <string>
#include <vector>

#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_wallet {

struct SolanaSignatureStatus;
struct SolanaAccountInfo;

namespace solana {

bool ParseGetBalance(const std::string& json, uint64_t* balance);
bool ParseGetTokenAccountBalance(const std::string& json,
                                 std::string* amount,
                                 uint8_t* decimals,
                                 std::string* ui_amount_string);
bool ParseSendTransaction(const std::string& json, std::string* tx_id);
bool ParseGetLatestBlockhash(const std::string& json,
                             std::string* hash,
                             uint64_t* last_valid_block_height);
bool ParseGetSignatureStatuses(
    const std::string& json,
    std::vector<absl::optional<SolanaSignatureStatus>>* statuses);
bool ParseGetAccountInfo(const std::string& json,
                         absl::optional<SolanaAccountInfo>* account_info_out);
bool ParseGetFeeForMessage(const std::string& json, uint64_t* fee);
bool ParseGetBlockHeight(const std::string& json, uint64_t* block_height);

}  // namespace solana

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_RESPONSE_PARSER_H_
