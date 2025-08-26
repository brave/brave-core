/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

#include "base/functional/bind.h"
#include "brave/components/brave_wallet/browser/network_manager.h"

namespace brave_wallet {

using APIRequestResult = api_request_helper::APIRequestResult;

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("polkadot_substrate_rpc", R"(
      semantics {
        sender: "Polkadot Substrate RPC"
        description:
          "This service is used to communicate with Polkadot Substrate nodes "
          "on behalf of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Polkadot Substrate JSON RPC response bodies."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable this feature on chrome://flags."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

}  // namespace

PolkadotSubstrateRpc::PolkadotSubstrateRpc(
    NetworkManager& network_manager,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : network_manager_(network_manager),
      api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

PolkadotSubstrateRpc::~PolkadotSubstrateRpc() = default;

void PolkadotSubstrateRpc::GetChainName(const std::string& chain_id,
                                        GetChainNameCallback callback) {
  std::string method = "POST";
  auto url = GetNetworkURL(chain_id);
  std::string payload =
      R"({"id":1, "jsonrpc":"2.0", "method": "system_chain", "params":[]})";

  std::string payload_content_type = "application/json";

  api_request_helper_.Request(
      method, url, payload, payload_content_type,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetChainName,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetChainName(GetChainNameCallback callback,
                                          APIRequestResult res) {
  if (!res.Is2XXResponseCode()) {
    auto err_str = res.SerializeBodyToString();
    if (err_str.empty()) {
      err_str = "PolkadotSubstrateRpc error";
    }
    return std::move(callback).Run(std::nullopt, err_str);
  }

  const auto& body = res.value_body();
  const auto* result = body.GetDict().Find("result");
  if (!result) {
    return std::move(callback).Run(std::nullopt,
                                   "Missing result field in RPC response");
  }

  const auto* chain_name = result->GetIfString();
  if (!chain_name) {
    return std::move(callback).Run(std::nullopt,
                                   "result field was not a string");
  }

  std::move(callback).Run(*chain_name, std::nullopt);
}

GURL PolkadotSubstrateRpc::GetNetworkURL(const std::string& chain_id) {
  return network_manager_->GetNetworkURL(chain_id, mojom::CoinType::DOT);
}

}  // namespace brave_wallet
