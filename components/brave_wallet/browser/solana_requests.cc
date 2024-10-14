/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/solana_requests.h"

#include <optional>
#include <utility>

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/json/json_helper.h"

namespace brave_wallet::solana {

std::string getBalance(const std::string& pubkey) {
  return GetJsonRpcString("getBalance", pubkey);
}

std::string getTokenAccountBalance(const std::string& pubkey) {
  return GetJsonRpcString("getTokenAccountBalance", pubkey);
}

std::string sendTransaction(
    const std::string& signed_tx,
    std::optional<SolanaTransaction::SendOptions> options) {
  base::Value::List params;
  params.Append(signed_tx);

  // Set encoding to base64 because the document says base58 is currently the
  // default value but is slow and deprecated.
  base::Value::Dict configuration;
  configuration.Set("encoding", "base64");

  if (options) {
    if (options->max_retries) {
      configuration.Set("maxRetries",
                        base::NumberToString(*options->max_retries));
    }
    if (options->preflight_commitment) {
      configuration.Set("preflightCommitment", *options->preflight_commitment);
    }
    if (options->skip_preflight) {
      configuration.Set("skipPreflight", *options->skip_preflight);
    }
  }

  params.Append(std::move(configuration));

  base::Value::Dict dictionary =
      GetJsonRpcDictionary("sendTransaction", std::move(params));
  return json::convert_string_value_to_uint64("/params/1/maxRetries",
                                              GetJSON(dictionary), true);
}

std::string getLatestBlockhash() {
  return GetJsonRpcString("getLatestBlockhash");
}

std::string getSignatureStatuses(
    const std::vector<std::string>& tx_signatures) {
  base::Value::List params;
  base::Value::List tx_signatures_value;
  for (const auto& tx_signature : tx_signatures) {
    tx_signatures_value.Append(tx_signature);
  }
  params.Append(std::move(tx_signatures_value));

  // Solana node will search its ledger cache for any signatures not found in
  // the recent status cache. Enable this since we may try to update a pending
  // transaction sitting for a while.
  base::Value::Dict configuration;
  configuration.Set("searchTransactionHistory", true);
  params.Append(std::move(configuration));

  base::Value::Dict dictionary =
      GetJsonRpcDictionary("getSignatureStatuses", std::move(params));
  return GetJSON(dictionary);
}

std::string getAccountInfo(const std::string& pubkey) {
  base::Value::List params;
  params.Append(pubkey);

  // Set encoding to base64 because the document says base58 is currently the
  // default value but is slow and deprecated.
  base::Value::Dict configuration;
  configuration.Set("encoding", "base64");
  params.Append(std::move(configuration));

  base::Value::Dict dictionary =
      GetJsonRpcDictionary("getAccountInfo", std::move(params));
  return GetJSON(dictionary);
}

std::string getFeeForMessage(const std::string& message) {
  base::Value::List params;
  params.Append(message);

  base::Value::Dict configuration;
  // dApps may supply a blockhash with a confirmed commitment level,
  // so fetching a fee for those transactions requires us using
  // a confirmed commitment level.
  configuration.Set("commitment", "confirmed");
  params.Append(std::move(configuration));

  base::Value::Dict dictionary =
      GetJsonRpcDictionary("getFeeForMessage", std::move(params));
  return GetJSON(dictionary);
}

std::string getBlockHeight() {
  return GetJsonRpcString("getBlockHeight");
}

// Returns all SPL Token accounts by token owner.
//
// "base58" as encoding is slow and deprecated. Prefer using "base64" instead.
std::string getTokenAccountsByOwner(const std::string& pubkey,
                                    const std::string& encoding,
                                    const std::string& program_id) {
  CHECK(IsValidEncodingString(encoding));

  base::Value::List params;
  params.Append(pubkey);

  base::Value::Dict program;
  base::Value::Dict encoding_dict;
  program.Set("programId", program_id);
  encoding_dict.Set("encoding", encoding);
  params.Append(std::move(program));
  params.Append(std::move(encoding_dict));

  base::Value::Dict dictionary =
      GetJsonRpcDictionary("getTokenAccountsByOwner", std::move(params));
  return GetJSON(dictionary);
}

std::string isBlockhashValid(const std::string& blockhash,
                             const std::optional<std::string>& commitment) {
  CHECK(!commitment || IsValidCommitmentString(*commitment));
  base::Value::List params;
  params.Append(blockhash);

  base::Value::Dict configuration;
  configuration.Set("commitment", commitment ? *commitment : "processed");
  params.Append(std::move(configuration));

  base::Value::Dict dictionary =
      GetJsonRpcDictionary("isBlockhashValid", std::move(params));
  return GetJSON(dictionary);
}

std::string simulateTransaction(const std::string& unsigned_tx) {
  base::Value::List params;
  params.Append(unsigned_tx);

  base::Value::Dict configuration;
  configuration.Set("encoding", "base64");
  // dApps may supply a blockhash with a confirmed commitment level,
  // so simulating that transaction requires us using
  // a confirmed commitment level.
  configuration.Set("commitment", "confirmed");
  params.Append(std::move(configuration));

  base::Value::Dict dictionary =
      GetJsonRpcDictionary("simulateTransaction", std::move(params));
  return GetJSON(dictionary);
}

std::string getRecentPrioritizationFees() {
  return GetJsonRpcString("getRecentPrioritizationFees");
}

}  // namespace brave_wallet::solana
