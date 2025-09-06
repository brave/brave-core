/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/polkadot/polkadot_rpc.h"

#include "base/functional/bind.h"

namespace brave_wallet {

using APIRequestResult = api_request_helper::APIRequestResult;

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("polkadot_rpc", R"(
      semantics {
        sender: "Polkadot RPC"
        description:
          "This service is used to communicate with Polkadot Substrate nodes "
          "on behalf of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Polkadot JSON RPC response bodies."
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
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(),
                          url_loader_factory) {}

PolkadotSubstrateRpc::~PolkadotSubstrateRpc() = default;

void PolkadotSubstrateRpc::GetChainName(
    base::OnceCallback<void(const std::string&)> callback) {
  std::string method = "POST";
  GURL url("https://westend-rpc.polkadot.io");
  std::string payload =
      R"({"id":1, "jsonrpc":"2.0", "method": "system_chain", "params":[]})";

  std::string payload_content_type = "application/json";

  api_request_helper_.Request(
      method, url, payload, payload_content_type,
      base::BindOnce(
          [](base::OnceCallback<void(const std::string&)> callback,
             APIRequestResult res) {
            const auto& body = res.value_body();
            const auto* chain_name = body.GetDict().Find("result");

            std::move(callback).Run(chain_name->GetString());
          },
          std::move(callback)));
}

}  // namespace brave_wallet
