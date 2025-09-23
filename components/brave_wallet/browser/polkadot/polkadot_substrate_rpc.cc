/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_substrate_rpc.h"

#include "base/functional/bind.h"
#include "base/json/json_writer.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_responses.h"
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

base::DictValue PolkadotSubstrateRpc::MakeRpcRequestJson(
    std::string_view method,
    base::ListValue params) {
  base::DictValue req;
  req.Set("id", 1);
  req.Set("jsonrpc", "2.0");
  req.Set("method", method);
  req.Set("params", std::move(params));
  return req;
}

void PolkadotSubstrateRpc::GetChainName(const std::string& chain_id,
                                        GetChainNameCallback callback) {
  auto url = GetNetworkURL(chain_id);

  std::string method = net::HttpRequestHeaders::kPostMethod;
  std::string payload_content_type = "application/json";
  auto payload =
      base::WriteJson(MakeRpcRequestJson("system_chain", base::ListValue()));

  api_request_helper_.Request(
      method, url, *payload, payload_content_type,
      base::BindOnce(&PolkadotSubstrateRpc::OnGetChainName,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void PolkadotSubstrateRpc::OnGetChainName(GetChainNameCallback callback,
                                          APIRequestResult api_result) {
  if (!api_result.Is2XXResponseCode()) {
    return std::move(callback).Run(std::nullopt, WalletInternalErrorMessage());
  }

  auto res = json_rpc_responses::PolkadotSystemChainResponse::FromValue(
      api_result.value_body());

  if (!res) {
    return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
  }

  if (res->error) {
    if (res->error->message) {
      return std::move(callback).Run(std::nullopt, res->error->message.value());
    }
    return std::move(callback).Run(std::nullopt,
                                   WalletUserRejectedRequestErrorMessage());
  }

  if (res->result) {
    return std::move(callback).Run(*res->result, std::nullopt);
  }

  return std::move(callback).Run(std::nullopt, WalletParsingErrorMessage());
}

GURL PolkadotSubstrateRpc::GetNetworkURL(const std::string& chain_id) {
  return network_manager_->GetNetworkURL(chain_id, mojom::CoinType::DOT);
}

}  // namespace brave_wallet
