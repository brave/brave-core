/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"

#include <utility>

#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_call_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_requests.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "components/user_prefs/user_prefs.h"
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

}  // namespace

namespace brave_wallet {

EthJsonRpcController::EthJsonRpcController(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs)
    : api_request_helper_(GetNetworkTrafficAnnotationTag(), url_loader_factory),
      prefs_(prefs),
      weak_ptr_factory_(this) {
  std::vector<mojom::EthereumChainPtr> networks;
  GetAllKnownChains(&networks);
  DCHECK(!networks.empty());
  SetNetwork(networks.front()->chain_id);
}

EthJsonRpcController::~EthJsonRpcController() {}

mojo::PendingRemote<mojom::EthJsonRpcController>
EthJsonRpcController::MakeRemote() {
  mojo::PendingRemote<mojom::EthJsonRpcController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void EthJsonRpcController::Bind(
    mojo::PendingReceiver<mojom::EthJsonRpcController> receiver) {
  receivers_.Add(this, std::move(receiver));
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

void EthJsonRpcController::SetNetwork(const std::string& chain_id) {
  auto network_url = GetNetworkURL(prefs_, chain_id);
  if (!network_url.is_valid())
    return;
  chain_id_ = chain_id;
  network_url_ = network_url;
  FireNetworkChanged();
}

void EthJsonRpcController::FireNetworkChanged() {
  for (const auto& observer : observers_) {
    observer->ChainChangedEvent(chain_id_);
  }
}

std::string EthJsonRpcController::GetChainId() const {
  return chain_id_;
}

void EthJsonRpcController::GetChainId(
    mojom::EthJsonRpcController::GetChainIdCallback callback) {
  std::move(callback).Run(chain_id_);
}

void EthJsonRpcController::GetBlockTrackerUrl(
    mojom::EthJsonRpcController::GetBlockTrackerUrlCallback callback) {
  std::move(callback).Run(GetBlockTrackerUrlFromNetwork(chain_id_).spec());
}

void EthJsonRpcController::GetAllNetworks(
    mojom::EthJsonRpcController::GetAllNetworksCallback callback) {
  std::vector<mojom::EthereumChainPtr> all_chains;
  brave_wallet::GetAllChains(prefs_, &all_chains);
  std::move(callback).Run(std::move(all_chains));
}

void EthJsonRpcController::GetNetworkUrl(
    mojom::EthJsonRpcController::GetNetworkUrlCallback callback) {
  std::move(callback).Run(network_url_.spec());
}

void EthJsonRpcController::SetCustomNetworkForTesting(
    const std::string& chain_id,
    const GURL& network_url) {
  chain_id_ = chain_id;
  network_url_ = network_url;
  FireNetworkChanged();
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

// static
void EthJsonRpcController::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterListPref(kBraveWalletCustomNetworks);
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

GURL EthJsonRpcController::GetBlockTrackerUrlFromNetwork(std::string chain_id) {
  std::vector<mojom::EthereumChainPtr> networks;
  brave_wallet::GetAllChains(prefs_, &networks);
  for (const auto& network : networks) {
    if (network->chain_id != chain_id)
      continue;
    if (network->block_explorer_urls->size())
      return GURL(network->block_explorer_urls->front());
  }
  return GURL();
}

}  // namespace brave_wallet
