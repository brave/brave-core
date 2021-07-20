/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"

#include <utility>

#include "base/environment.h"
#include "base/strings/stringprintf.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_call_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_requests.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

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

EthJsonRpcController::EthJsonRpcController(
    Network network,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      network_(network),
      observers_(new base::ObserverListThreadSafe<
                 BraveWalletProviderEventsObserver>()),
      weak_ptr_factory_(this) {
  SetNetwork(network);
}

EthJsonRpcController::~EthJsonRpcController() {}

void EthJsonRpcController::AddObserver(
    BraveWalletProviderEventsObserver* observer) {
  observers_->AddObserver(observer);
}

void EthJsonRpcController::RemoveObserver(
    BraveWalletProviderEventsObserver* observer) {
  observers_->RemoveObserver(observer);
}

void EthJsonRpcController::Request(const std::string& json_payload,
                                   URLRequestCallback callback,
                                   bool auto_retry_on_network_change) {
  api_request_helper_.Request("POST", network_url_, json_payload,
                              "application/json", auto_retry_on_network_change,
                              std::move(callback));
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

  observers_->Notify(FROM_HERE,
                     &BraveWalletProviderEventsObserver::ChainChangedEvent,
                     GetChainIDFromNetwork(network_));
}

void EthJsonRpcController::SetCustomNetwork(const GURL& network_url) {
  network_ = Network::kCustom;
  network_url_ = network_url;
}

void EthJsonRpcController::GetBalance(
    const std::string& address,
    EthJsonRpcController::GetBallanceCallback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth_getBalance(address, "latest"),
                 std::move(internal_callback), true);
}

void EthJsonRpcController::OnGetBalance(
    GetBallanceCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }
  std::string balance;
  if (!ParseEthGetBalance(body, &balance)) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, balance);
}

void EthJsonRpcController::GetTransactionCount(const std::string& address,
                                               GetTxCountCallback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetTransactionCount,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth_getTransactionCount(address, "latest"),
                 std::move(internal_callback), true);
}

void EthJsonRpcController::OnGetTransactionCount(
    GetTxCountCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, 0);
    return;
  }
  uint256_t count;
  if (!ParseEthGetTransactionCount(body, &count)) {
    std::move(callback).Run(false, 0);
    return;
  }

  std::move(callback).Run(true, count);
}

void EthJsonRpcController::GetTransactionReceipt(
    const std::string& tx_hash,
    GetTxReceiptCallback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetTransactionReceipt,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth_getTransactionReceipt(tx_hash),
                 std::move(internal_callback), true);
}

void EthJsonRpcController::OnGetTransactionReceipt(
    GetTxReceiptCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  TransactionReceipt receipt;
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, receipt);
    return;
  }
  if (!ParseEthGetTransactionReceipt(body, &receipt)) {
    std::move(callback).Run(false, receipt);
    return;
  }

  std::move(callback).Run(true, receipt);
}

void EthJsonRpcController::SendRawTransaction(const std::string& signed_tx,
                                              SendRawTxCallback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnSendRawTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth_sendRawTransaction(signed_tx),
                 std::move(internal_callback), true);
}

void EthJsonRpcController::OnSendRawTransaction(
    SendRawTxCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }
  std::string tx_hash;
  if (!ParseEthSendRawTransaction(body, &tx_hash)) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, tx_hash);
}

bool EthJsonRpcController::GetERC20TokenBalance(
    const std::string& contract,
    const std::string& address,
    EthJsonRpcController::GetBallanceCallback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetERC20TokenBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  std::string data;
  if (!erc20::BalanceOf(address, &data)) {
    return false;
  }
  Request(eth_call("", address, "", "", "", data, ""),
          std::move(internal_callback), true);
  return true;
}

void EthJsonRpcController::OnGetERC20TokenBalance(
    GetERC20TokenBalanceCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }
  std::string result;
  if (!ParseEthCall(body, &result)) {
    std::move(callback).Run(false, "");
    return;
  }
  std::move(callback).Run(true, result);
}

void EthJsonRpcController::EnsProxyReaderGetResolverAddress(
    const std::string& contract_address,
    const std::string& domain,
    UnstoppableDomainsProxyReaderGetManyCallback callback) {
  auto internal_callback = base::BindOnce(
      &EthJsonRpcController::OnEnsProxyReaderGetResolverAddress,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback), domain);
  std::string data;
  if (!ens::GetResolverAddress(domain, &data)) {
    std::move(callback).Run(false, "");
  }

  Request(eth_call("", contract_address, "", "", "", data, "latest"),
          std::move(internal_callback), true);
}

void EthJsonRpcController::OnEnsProxyReaderGetResolverAddress(
    UnstoppableDomainsProxyReaderGetManyCallback callback,
    const std::string& domain,
    int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  DCHECK(callback);
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }
  std::string result;
  if (!ParseEthCall(body, &result) || result.empty()) {
    std::move(callback).Run(false, "");
    return;
  }
  size_t offset = 2 /* len of "0x" */ + 24 /* len of offset to array */;
  if (offset >= result.size()) {
    std::move(callback).Run(false, "");
    return;
  }
  std::string contenthash = "0x" + result.substr(offset);
  EnsProxyReaderResolveAddress(contenthash, domain, std::move(callback));
}

bool EthJsonRpcController::EnsProxyReaderResolveAddress(
    const std::string& contract_address,
    const std::string& domain,
    UnstoppableDomainsProxyReaderGetManyCallback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnEnsProxyReaderResolveAddress,
                     base::Unretained(this), std::move(callback));
  std::string data;
  if (!ens::GetContentHashAddress(domain, &data)) {
    return false;
  }

  Request(eth_call("", contract_address, "", "", "", data, "latest"),
          std::move(internal_callback), true);
  return true;
}

void EthJsonRpcController::OnEnsProxyReaderResolveAddress(
    UnstoppableDomainsProxyReaderGetManyCallback callback,
    int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  DCHECK(callback);
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }
  std::string result;
  if (!ParseEthCall(body, &result)) {
    std::move(callback).Run(false, "");
    return;
  }
  std::move(callback).Run(true, result);
}

void EthJsonRpcController::UnstoppableDomainsProxyReaderGetMany(
    const std::string& contract_address,
    const std::string& domain,
    const std::vector<std::string>& keys,
    UnstoppableDomainsProxyReaderGetManyCallback callback) {
  auto internal_callback = base::BindOnce(
      &EthJsonRpcController::OnUnstoppableDomainsProxyReaderGetMany,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  std::string data;
  if (!unstoppable_domains::GetMany(keys, domain, &data)) {
    std::move(callback).Run(false, "");
  }

  Request(eth_call("", contract_address, "", "", "", data, "latest"),
          std::move(internal_callback), true);
}

void EthJsonRpcController::OnUnstoppableDomainsProxyReaderGetMany(
    UnstoppableDomainsProxyReaderGetManyCallback callback,
    const int status,
    const std::string& body,
    const std::map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }
  std::string result;
  if (!ParseEthCall(body, &result)) {
    std::move(callback).Run(false, "");
    return;
  }
  std::move(callback).Run(true, result);
}

// [static]
std::string EthJsonRpcController::GetChainIDFromNetwork(Network network) {
  std::string chain_id;
  switch (network) {
    case Network::kMainnet:
      chain_id = "0x1";
      break;
    case Network::kRinkeby:
      chain_id = "0x4";
      break;
    case Network::kRopsten:
      chain_id = "0x3";
      break;
    case Network::kGoerli:
      chain_id = "0x5";
      break;
    case Network::kKovan:
      chain_id = "0x2a";
      break;
    case Network::kLocalhost:
      break;
    case Network::kCustom:
      break;
  }
  return chain_id;
}

GURL EthJsonRpcController::GetBlockTrackerURLFromNetwork(Network network) {
  GURL url;
  switch (network) {
    case Network::kMainnet:
      url = GURL("https://etherscan.io");
      break;
    case Network::kRinkeby:
      url = GURL("https://rinkeby.etherscan.io");
      break;
    case Network::kRopsten:
      url = GURL("https://ropsten.etherscan.io");
      break;
    case Network::kGoerli:
      url = GURL("https://goerli.etherscan.io");
      break;
    case Network::kKovan:
      url = GURL("https://kovan.etherscan.io");
      break;
    case Network::kLocalhost:
      break;
    case Network::kCustom:
      break;
  }
  return url;
}

}  // namespace brave_wallet
