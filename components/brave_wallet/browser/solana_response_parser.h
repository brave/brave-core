/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_RESPONSE_PARSER_H_

#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

// TODO(apaymyshev): refactor utility methods to return absl::optional instead
// of bool + out-parameter.

namespace brave_wallet {

struct SolanaSignatureStatus;
struct SolanaAccountInfo;

namespace solana {

bool ParseGetBalance(const base::Value& json_value, uint64_t* balance);
bool ParseGetTokenAccountBalance(const base::Value& json_value,
                                 std::string* amount,
                                 uint8_t* decimals,
                                 std::string* ui_amount_string);
bool ParseSendTransaction(const base::Value& json_value, std::string* tx_id);
bool ParseGetLatestBlockhash(const base::Value& json_value,
                             std::string* hash,
                             uint64_t* last_valid_block_height);
bool ParseGetSignatureStatuses(
    const base::Value& json_value,
    std::vector<absl::optional<SolanaSignatureStatus>>* statuses);
bool ParseGetAccountInfo(const base::Value& json_value,
                         absl::optional<SolanaAccountInfo>* account_info_out);
bool ParseGetAccountInfoPayload(
    const base::Value::Dict& value_dict,
    absl::optional<SolanaAccountInfo>* account_info_out);
bool ParseGetFeeForMessage(const base::Value& json_value, uint64_t* fee);
bool ParseGetBlockHeight(const base::Value& json_value, uint64_t* block_height);
bool ParseGetTokenAccountsByOwner(const base::Value& json_value,
                                  std::vector<SolanaAccountInfo>* accounts);

base::OnceCallback<absl::optional<std::string>(const std::string& raw_response)>
ConverterForGetAccountInfo();
base::OnceCallback<absl::optional<std::string>(const std::string& raw_response)>
ConverterForGetProrgamAccounts();

}  // namespace solana

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SOLANA_RESPONSE_PARSER_H_
