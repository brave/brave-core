/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_REQUESTS_HELPER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_REQUESTS_HELPER_H_

#include <string>
#include <string_view>
#include <utility>

#include "base/containers/flat_map.h"
#include "base/values.h"

// Helper functions for building out JSON RPC requests across all blockchains.
namespace brave_wallet {

namespace internal {

base::Value::Dict ComposeRpcDict(std::string_view method);

}  // namespace internal

template <typename T>
base::Value::Dict GetJsonRpcDictionary(std::string_view method, T&& params) {
  auto dict = internal::ComposeRpcDict(method);
  dict.Set("params", std::move(params));
  return dict;
}

std::string GetJSON(base::ValueView dict);

template <typename... Args>
std::string GetJsonRpcString(std::string_view method, Args&&... args) {
  base::Value::List params;
  (params.Append(std::forward<Args&&>(args)), ...);
  return GetJSON(GetJsonRpcDictionary(method, std::move(params)));
}

void AddKeyIfNotEmpty(base::Value::Dict* dict,
                      std::string_view name,
                      std::string_view val);

base::flat_map<std::string, std::string> MakeCommonJsonRpcHeaders(
    const std::string& json_payload);
base::flat_map<std::string, std::string> MakeBraveServicesKeyHeaders();

std::string EncodeAnkrGetAccountBalanceParams(
    const std::string& address,
    std::vector<std::string> blockchains);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_REQUESTS_HELPER_H_
