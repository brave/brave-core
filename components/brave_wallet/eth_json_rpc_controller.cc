/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/eth_json_rpc_controller.h"

#include <utility>

#include "base/environment.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("eth_json_rpc_controller", R"(
      semantics {
        sender: "ETH JSON RPC Controller"
        description:
          "This controller is used to communicate with Ethereum nodes "
          "on behalf of the user interacting with the native Brave wallet."
        trigger:
          "Triggered by uses of the native Brave wallet."
        data:
          "Ethereum JSON RPC response bodies."
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

const unsigned int kRetriesCountOnNetworkChange = 1;

std::string GetInfuraProjectID() {
  std::string project_id(BRAVE_INFURA_PROJECT_ID);
  std::unique_ptr<base::Environment> env(base::Environment::Create());

  if (env->HasVar("BRAVE_INFURA_PROJECT_ID")) {
    env->GetVar("BRAVE_INFURA_PROJECT_ID", &project_id);
  }

  return project_id;
}

bool GetUseStagingInfuraEndpoint() {
  std::string project_id(BRAVE_INFURA_PROJECT_ID);
  std::unique_ptr<base::Environment> env(base::Environment::Create());
  return env->HasVar("BRAVE_INFURA_STAGING");
}

}  // namespace

namespace brave_wallet {

EthJsonRpcController::EthJsonRpcController(content::BrowserContext* context,
                                           Network network)
    : context_(context), network_(network) {
  SetNetwork(network);
}

EthJsonRpcController::~EthJsonRpcController() {}

void EthJsonRpcController::Request(const std::string& json_payload,
                                   URLRequestCallback callback,
                                   bool auto_retry_on_network_change) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = network_url_;
  request->load_flags = net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->load_flags |= net::LOAD_DO_NOT_SAVE_COOKIES;
  request->method = "POST";

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTag());
  if (!json_payload.empty()) {
    url_loader->AttachStringForUpload(json_payload, "application/json");
  }
  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      auto_retry_on_network_change
          ? network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE
          : network::SimpleURLLoader::RetryMode::RETRY_NEVER);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));

  auto* default_storage_partition =
      content::BrowserContext::GetDefaultStoragePartition(context_);
  auto* url_loader_factory =
      default_storage_partition->GetURLLoaderFactoryForBrowserProcess().get();

  iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory,
      base::BindOnce(&EthJsonRpcController::OnURLLoaderComplete,
                     base::Unretained(this), std::move(iter),
                     std::move(callback)));
}

void EthJsonRpcController::OnURLLoaderComplete(
    SimpleURLLoaderList::iterator iter,
    URLRequestCallback callback,
    const std::unique_ptr<std::string> response_body) {
  auto* loader = iter->get();
  auto response_code = -1;
  std::map<std::string, std::string> headers;
  if (loader->ResponseInfo()) {
    auto headers_list = loader->ResponseInfo()->headers;
    if (headers_list) {
      response_code = headers_list->response_code();
      size_t iter = 0;
      std::string key;
      std::string value;
      while (headers_list->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        headers[key] = value;
      }
    }
  }
  url_loaders_.erase(iter);
  std::move(callback).Run(response_code, response_body ? *response_body : "",
                          headers);
}

Network EthJsonRpcController::GetNetwork() const {
  return network_;
}

GURL EthJsonRpcController::GetNetworkURL() const {
  return network_url_;
}

void EthJsonRpcController::SetNetwork(Network network) {
  std::string subdomain;
  network_ = network;
  switch (network) {
    case Network::kMainnet:
      subdomain = "mainnet";
      break;
    case Network::kRinkeby:
      subdomain = "rinkeby";
      break;
    case Network::kRopsten:
      subdomain = "ropsten";
      break;
    case Network::kGoerli:
      subdomain = "goerli";
      break;
    case Network::kKovan:
      subdomain = "kovan";
      break;
    case Network::kLocalhost:
      network_url_ = GURL("http://localhost:8545");
      return;
    case Network::kCustom:
      return;
  }

  const std::string spec =
      base::StringPrintf(GetUseStagingInfuraEndpoint()
                             ? "https://%s-staging-infura.bravesoftware.com/%s"
                             : "https://%s-infura.brave.com/%s",
                         subdomain.c_str(), GetInfuraProjectID().c_str());
  network_url_ = GURL(spec);
}

void EthJsonRpcController::SetCustomNetwork(const GURL& network_url) {
  network_ = Network::kCustom;
  network_url_ = network_url;
}

}  // namespace brave_wallet
