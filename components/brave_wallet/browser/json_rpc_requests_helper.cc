/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"

#include <memory>
#include <string_view>
#include <utility>

#include "base/environment.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/constants/brave_services_key.h"
#include "net/http/http_util.h"

namespace brave_wallet {

namespace internal {

base::Value::Dict ComposeRpcDict(std::string_view method) {
  base::Value::Dict dict;
  dict.Set("jsonrpc", "2.0");
  dict.Set("method", method);
  // I don't think we need to use this param, but it is required,
  // so always set it to 1 for now..
  dict.Set("id", 1);
  return dict;
}

}  // namespace internal

std::string GetJSON(base::ValueView dict) {
  std::string json;
  base::JSONWriter::Write(dict, &json);
  return json;
}

void AddKeyIfNotEmpty(base::Value::Dict* dict,
                      std::string_view name,
                      std::string_view val) {
  if (!val.empty()) {
    dict->Set(name, val);
  }
}

base::flat_map<std::string, std::string> MakeCommonJsonRpcHeaders(
    const std::string& json_payload) {
  base::flat_map<std::string, std::string> request_headers;
  std::string id, method, params;
  if (GetEthJsonRequestInfo(json_payload, nullptr, &method, &params)) {
    if (net::HttpUtil::IsValidHeaderValue(method)) {
      request_headers["X-Eth-Method"] = method;
    }
    if (method == kEthGetBlockByNumber) {
      std::string cleaned_params;
      base::RemoveChars(params, "\" []", &cleaned_params);
      if (net::HttpUtil::IsValidHeaderValue(cleaned_params)) {
        request_headers["X-eth-get-block"] = cleaned_params;
      }
    } else if (method == kEthBlockNumber) {
      request_headers["X-Eth-Block"] = "true";
    }
  }

  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string brave_key(BUILDFLAG(BRAVE_SERVICES_KEY));
  if (env->HasVar("BRAVE_SERVICES_KEY")) {
    env->GetVar("BRAVE_SERVICES_KEY", &brave_key);
  }
  request_headers[kBraveServicesKeyHeader] = std::move(brave_key);

  return request_headers;
}

base::flat_map<std::string, std::string> MakeBraveServicesKeyHeaders() {
  return {
      {kBraveServicesKeyHeader, BUILDFLAG(BRAVE_SERVICES_KEY)},
  };
}

std::string EncodeAnkrGetAccountBalanceParams(
    const std::string& address,
    std::vector<std::string> blockchains) {
  base::Value::Dict dict = internal::ComposeRpcDict("ankr_getAccountBalance");

  base::Value::Dict params;
  params.Set("nativeFirst", true);
  params.Set("walletAddress", address);

  base::Value::List blockchains_list;
  for (const auto& blockchain : blockchains) {
    blockchains_list.Append(blockchain);
  }
  params.Set("blockchains", std::move(blockchains_list));

  dict.Set("params", std::move(params));
  return GetJSON(dict);
}

}  // namespace brave_wallet
