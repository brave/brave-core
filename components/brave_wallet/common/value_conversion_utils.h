/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_VALUE_CONVERSION_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_VALUE_CONVERSION_UTILS_H_

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/origin.h"

namespace brave_wallet {

std::optional<std::string> ExtractChainIdFromValue(
    const base::Value::Dict* dict);
base::Value::Dict NetworkInfoToValue(const mojom::NetworkInfo& info);
mojom::NetworkInfoPtr ValueToNetworkInfo(const base::Value& value);
mojom::NetworkInfoPtr ParseEip3085Payload(const base::Value& value);
base::Value::List PermissionRequestResponseToValue(
    const url::Origin& origin,
    const std::vector<std::string>& accounts);

mojom::BlockchainTokenPtr ValueToBlockchainToken(
    const base::Value::Dict& value);
base::Value::Dict BlockchainTokenToValue(
    const mojom::BlockchainTokenPtr& token);

// Returns index of the first URL to use that:
// 1. Has no variables in it like ${INFURA_API_KEY}
// 2. Is HTTP or HTTPS
// Otherwise returns 0.
int GetFirstValidChainURLIndex(const std::vector<GURL>& chain_urls);

bool ReadUint32StringTo(const base::Value::Dict& dict,
                        std::string_view key,
                        uint32_t& to);

template <class T>
bool ReadDictTo(const base::Value::Dict& dict, std::string_view key, T& to) {
  auto* key_dict = dict.FindDict(key);
  if (!key_dict) {
    return false;
  }
  auto t_opt = T::FromValue(*key_dict);
  if (!t_opt) {
    return false;
  }
  to = std::move(*t_opt);
  return true;
}

template <size_t const T>
bool ReadHexByteArrayTo(const base::Value::Dict& dict,
                        std::string_view key,
                        std::array<uint8_t, T>& to) {
  auto* str = dict.FindString(key);
  if (!str) {
    return false;
  }
  if (str->empty()) {
    return false;
  }
  std::vector<uint8_t> output;

  if (!base::HexStringToBytes(*str, &output)) {
    return false;
  }

  if (output.size() != T) {
    return false;
  }

  base::ranges::copy_n(output.begin(), to.size(), to.begin());
  return true;
}

bool ReadStringTo(const base::Value::Dict& dict,
                  std::string_view key,
                  std::string& to);

bool ReadUint64StringTo(const base::Value::Dict& dict,
                        std::string_view key,
                        uint64_t& to);

bool ReadHexByteArrayTo(const base::Value::Dict& dict,
                        std::string_view key,
                        std::vector<uint8_t>& to);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_VALUE_CONVERSION_UTILS_H_
