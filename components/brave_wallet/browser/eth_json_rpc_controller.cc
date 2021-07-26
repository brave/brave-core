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
    mojom::Network network,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      network_(network),
      weak_ptr_factory_(this) {
  SetNetwork(network);
}

EthJsonRpcController::~EthJsonRpcController() {}

mojo::PendingRemote<mojom::EthJsonRpcController>
EthJsonRpcController::MakeRemote() {
  mojo::PendingRemote<mojom::EthJsonRpcController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void EthJsonRpcController::AddObserver(
    ::mojo::PendingRemote<mojom::EthJsonRpcControllerObserver> observer) {
  observers_.Add(std::move(observer));
}

void EthJsonRpcController::Request(const std::string& json_payload,
                                   bool auto_retry_on_network_change,
                                   RequestCallback callback) {
  api_request_helper_.Request("POST", network_url_, json_payload,
                              "application/json", auto_retry_on_network_change,
                              std::move(callback));
}

void EthJsonRpcController::GetNetwork(
    mojom::EthJsonRpcController::GetNetworkCallback callback) {
  std::move(callback).Run(network_);
}

void EthJsonRpcController::SetNetwork(mojom::Network network) {
  std::string subdomain;
  network_ = network;
  switch (network) {
    case brave_wallet::mojom::Network::Mainnet:
      subdomain = "mainnet";
      break;
    case brave_wallet::mojom::Network::Rinkeby:
      subdomain = "rinkeby";
      break;
    case brave_wallet::mojom::Network::Ropsten:
      subdomain = "ropsten";
      break;
    case brave_wallet::mojom::Network::Goerli:
      subdomain = "goerli";
      break;
    case brave_wallet::mojom::Network::Kovan:
      subdomain = "kovan";
      break;
    case brave_wallet::mojom::Network::Localhost:
      network_url_ = GURL("http://localhost:8545");
      return;
    case brave_wallet::mojom::Network::Custom:
      return;
  }

  const std::string spec =
      base::StringPrintf(GetUseStagingInfuraEndpoint()
                             ? "https://%s-staging-infura.bravesoftware.com/%s"
                             : "https://%s-infura.brave.com/%s",
                         subdomain.c_str(), GetInfuraProjectID().c_str());
  network_url_ = GURL(spec);

  for (const auto& observer : observers_) {
    observer->ChainChangedEvent(GetChainIdFromNetwork(network_));
  }
}

void EthJsonRpcController::GetChainId(
    mojom::EthJsonRpcController::GetChainIdCallback callback) {
  std::move(callback).Run(GetChainIdFromNetwork(network_));
}

void EthJsonRpcController::GetBlockTrackerUrl(
    mojom::EthJsonRpcController::GetBlockTrackerUrlCallback callback) {
  std::move(callback).Run(GetBlockTrackerUrlFromNetwork(network_).spec());
}

void EthJsonRpcController::GetNetworkUrl(
    mojom::EthJsonRpcController::GetNetworkUrlCallback callback) {
  std::move(callback).Run(network_url_.spec());
}

void EthJsonRpcController::SetCustomNetwork(const GURL& network_url) {
  network_ = brave_wallet::mojom::Network::Custom;
  network_url_ = network_url;
}

void EthJsonRpcController::GetBlockNumber(GetBlockNumberCallback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetBlockNumber,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth_blockNumber(), true, std::move(internal_callback));
}

void EthJsonRpcController::OnGetBlockNumber(
    GetBlockNumberCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, 0);
    return;
  }
  uint256_t block_number;
  if (!ParseEthGetBlockNumber(body, &block_number)) {
    std::move(callback).Run(false, 0);
    return;
  }

  std::move(callback).Run(true, block_number);
}

void EthJsonRpcController::GetBalance(
    const std::string& address,
    EthJsonRpcController::GetBalanceCallback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth_getBalance(address, "latest"), true,
                 std::move(internal_callback));
}

void EthJsonRpcController::OnGetBalance(
    GetBalanceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
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
  return Request(eth_getTransactionCount(address, "latest"), true,
                 std::move(internal_callback));
}

void EthJsonRpcController::OnGetTransactionCount(
    GetTxCountCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
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
  return Request(eth_getTransactionReceipt(tx_hash), true,
                 std::move(internal_callback));
}

void EthJsonRpcController::OnGetTransactionReceipt(
    GetTxReceiptCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
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
  return Request(eth_sendRawTransaction(signed_tx), true,
                 std::move(internal_callback));
}

void EthJsonRpcController::OnSendRawTransaction(
    SendRawTxCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
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

void EthJsonRpcController::GetERC20TokenBalance(
    const std::string& contract,
    const std::string& address,
    EthJsonRpcController::GetERC20TokenBalanceCallback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetERC20TokenBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  std::string data;
  if (!erc20::BalanceOf(address, &data)) {
    std::move(callback).Run(false, "");
    return;
  }
  Request(eth_call("", contract, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void EthJsonRpcController::OnGetERC20TokenBalance(
    GetERC20TokenBalanceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
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

  Request(eth_call("", contract_address, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void EthJsonRpcController::OnEnsProxyReaderGetResolverAddress(
    UnstoppableDomainsProxyReaderGetManyCallback callback,
    const std::string& domain,
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
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

  Request(eth_call("", contract_address, "", "", "", data, "latest"), true,
          std::move(internal_callback));
  return true;
}

void EthJsonRpcController::OnEnsProxyReaderResolveAddress(
    UnstoppableDomainsProxyReaderGetManyCallback callback,
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
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

  Request(eth_call("", contract_address, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void EthJsonRpcController::OnUnstoppableDomainsProxyReaderGetMany(
    UnstoppableDomainsProxyReaderGetManyCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
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
std::string EthJsonRpcController::GetChainIdFromNetwork(
    mojom::Network network) {
  std::string chain_id;
  switch (network) {
    case brave_wallet::mojom::Network::Mainnet:
      chain_id = "0x1";
      break;
    case brave_wallet::mojom::Network::Rinkeby:
      chain_id = "0x4";
      break;
    case brave_wallet::mojom::Network::Ropsten:
      chain_id = "0x3";
      break;
    case brave_wallet::mojom::Network::Goerli:
      chain_id = "0x5";
      break;
    case brave_wallet::mojom::Network::Kovan:
      chain_id = "0x2a";
      break;
    case brave_wallet::mojom::Network::Localhost:
      break;
    case brave_wallet::mojom::Network::Custom:
      break;
  }
  return chain_id;
}

GURL EthJsonRpcController::GetBlockTrackerUrlFromNetwork(
    mojom::Network network) {
  GURL url;
  switch (network) {
    case brave_wallet::mojom::Network::Mainnet:
      url = GURL("https://etherscan.io");
      break;
    case brave_wallet::mojom::Network::Rinkeby:
      url = GURL("https://rinkeby.etherscan.io");
      break;
    case brave_wallet::mojom::Network::Ropsten:
      url = GURL("https://ropsten.etherscan.io");
      break;
    case brave_wallet::mojom::Network::Goerli:
      url = GURL("https://goerli.etherscan.io");
      break;
    case brave_wallet::mojom::Network::Kovan:
      url = GURL("https://kovan.etherscan.io");
      break;
    case brave_wallet::mojom::Network::Localhost:
      break;
    case brave_wallet::mojom::Network::Custom:
      break;
  }
  return url;
}

}  // namespace brave_wallet
