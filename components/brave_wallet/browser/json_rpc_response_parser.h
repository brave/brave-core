/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_RESPONSE_PARSER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_RESPONSE_PARSER_H_

#include <string>
#include <vector>

#include "base/json/json_reader.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/json_rpc_responses.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "components/grit/brave_components_strings.h"
#include "ui/base/l10n/l10n_util.h"

// Common JSON RPC response parsing functions across different blockchains.
namespace brave_wallet {

absl::optional<std::string> ParseSingleStringResult(
    const base::Value& json_value);
absl::optional<base::Value> ParseResultValue(const base::Value& json_value);

absl::optional<std::vector<uint8_t>> ParseDecodedBytesResult(
    const base::Value& json_value);

template <typename Error>
void ParseErrorResult(const base::Value& json_value,
                      Error* error,
                      std::string* error_message) {
  DCHECK(error);
  DCHECK(error_message);

  // This error is defined in https://www.jsonrpc.org/specification#error_object
  // and the same for Ethereum, Solana, and other JSON RPC implementations.
  *error = Error::kParsingError;
  *error_message = l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR);

  auto response =
      json_rpc_responses::RPCResponse::FromValueDeprecated(json_value);
  if (!response || !response->error) {
    return;
  }
  *error = static_cast<Error>(response->error->code);
  if (!mojom::IsKnownEnumValue(*error)) {
    *error = Error::kUnknown;
  }
  if (response->error->message) {
    *error_message = *response->error->message;
  } else {
    error_message->clear();
  }
}

absl::optional<base::Value::Dict> ParseResultDict(
    const base::Value& json_value);
absl::optional<base::Value::List> ParseResultList(
    const base::Value& json_value);
absl::optional<bool> ParseBoolResult(const base::Value& json_value);

absl::optional<std::string> ConvertInt64ToString(const std::string& path,
                                                 const std::string& json);

absl::optional<std::string> ConvertUint64ToString(const std::string& path,
                                                  const std::string& json);

absl::optional<std::string> ConvertMultiUint64ToString(
    const std::vector<std::string>& paths,
    const std::string& json);

absl::optional<std::string> ConvertMultiUint64InObjectArrayToString(
    const std::string& path_to_list,
    const std::string& path_to_object,
    const std::vector<std::string>& keys,
    const std::string& json);

namespace ankr {
absl::optional<std::vector<mojom::AnkrAssetBalancePtr>>
ParseGetAccountBalanceResponse(const base::Value& json_value);
}  // namespace ankr

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_RESPONSE_PARSER_H_
