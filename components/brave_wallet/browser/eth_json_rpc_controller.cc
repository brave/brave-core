/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"

#include <utility>

#include "base/bind.h"
#include "base/environment.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/eth_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_requests.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/l10n/l10n_util.h"

namespace {

// The domain name should be a-z | A-Z | 0-9 and hyphen(-).
// The domain name should not start or end with hyphen (-).
// The domain name can be a subdomain.
// TLD & TLD-1 must be at least two characters.
constexpr char kDomainPattern[] =
    "(?:[A-Za-z0-9][A-Za-z0-9-]*[A-Za-z0-9]\\.)+[A-Za-z]{2,}$";

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
  SetNetwork(prefs_->GetString(kBraveWalletCurrentChainId),
             base::BindOnce([](bool success) {
               if (!success)
                 LOG(ERROR)
                     << "Could not set netowrk from EthJsonRpcController()";
             }));
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
  RequestInternal(json_payload, auto_retry_on_network_change, network_url_,
                  std::move(callback));
}

void EthJsonRpcController::RequestInternal(const std::string& json_payload,
                                           bool auto_retry_on_network_change,
                                           const GURL& network_url,
                                           RequestCallback callback) {
  DCHECK(network_url.is_valid());

  base::flat_map<std::string, std::string> request_headers;
  std::string id, method, params;
  if (GetEthJsonRequestInfo(json_payload, nullptr, &method, &params)) {
    request_headers["X-Eth-Method"] = method;
    if (method == kEthGetBlockByNumber) {
      std::string cleaned_params;
      base::RemoveChars(params, "\" []", &cleaned_params);
      request_headers["X-eth-get-block"] = cleaned_params;
    } else if (method == kEthBlockNumber) {
      request_headers["X-Eth-Block"] = "true";
    }
  }

  std::unique_ptr<base::Environment> env(base::Environment::Create());
  std::string brave_key(BRAVE_SERVICES_KEY);
  if (env->HasVar("BRAVE_SERVICES_KEY")) {
    env->GetVar("BRAVE_SERVICES_KEY", &brave_key);
  }
  request_headers["x-brave-key"] = brave_key;

  api_request_helper_.Request("POST", network_url, json_payload,
                              "application/json", auto_retry_on_network_change,
                              std::move(callback), request_headers);
}

void EthJsonRpcController::FirePendingRequestCompleted(
    const std::string& chain_id,
    const std::string& error) {
  for (const auto& observer : observers_) {
    observer->OnAddEthereumChainRequestCompleted(chain_id, error);
  }
}

bool EthJsonRpcController::HasRequestFromOrigin(const GURL& origin) const {
  for (const auto& request : add_chain_pending_requests_) {
    if (request.second.origin == origin)
      return true;
  }
  return false;
}

void EthJsonRpcController::GetPendingChainRequests(
    GetPendingChainRequestsCallback callback) {
  std::vector<mojom::EthereumChainPtr> all_chains;
  for (const auto& request : add_chain_pending_requests_) {
    all_chains.push_back(request.second.request.Clone());
  }
  std::move(callback).Run(std::move(all_chains));
}

void EthJsonRpcController::AddEthereumChain(mojom::EthereumChainPtr chain,
                                            const GURL& origin,
                                            AddEthereumChainCallback callback) {
  DCHECK_EQ(origin, origin.DeprecatedGetOriginAsURL());
  if (!origin.is_valid() ||
      add_chain_pending_requests_.contains(chain->chain_id) ||
      HasRequestFromOrigin(origin)) {
    std::move(callback).Run(chain->chain_id, false);
    return;
  }
  auto chain_id = chain->chain_id;
  add_chain_pending_requests_[chain_id] =
      EthereumChainRequest(origin, std::move(*chain));
  std::move(callback).Run(chain_id, true);
}

void EthJsonRpcController::AddEthereumChainRequestCompleted(
    const std::string& chain_id,
    bool approved) {
  if (!add_chain_pending_requests_.contains(chain_id))
    return;
  if (approved) {
    AddCustomNetwork(prefs_,
                     add_chain_pending_requests_.at(chain_id).request.Clone());
  }
  std::string error =
      approved ? std::string()
               : l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST);
  FirePendingRequestCompleted(chain_id, error);
  add_chain_pending_requests_.erase(chain_id);
}

bool EthJsonRpcController::SetNetwork(const std::string& chain_id) {
  auto network_url = GetNetworkURL(prefs_, chain_id);
  if (!network_url.is_valid()) {
    return false;
  }

  chain_id_ = chain_id;
  network_url_ = network_url;
  prefs_->SetString(kBraveWalletCurrentChainId, chain_id);

  FireNetworkChanged();
  MaybeUpdateIsEip1559(chain_id);
  return true;
}

void EthJsonRpcController::SetNetwork(const std::string& chain_id,
                                      SetNetworkCallback callback) {
  if (!SetNetwork(chain_id))
    std::move(callback).Run(false);
  else
    std::move(callback).Run(true);
}

void EthJsonRpcController::MaybeUpdateIsEip1559(const std::string& chain_id) {
  // Only try to update is_eip1559 for localhost or custom chains.
  auto chain = GetKnownChain(prefs_, chain_id);
  if (chain && chain_id != brave_wallet::mojom::kLocalhostChainId)
    return;

  GetIsEip1559(base::BindOnce(&EthJsonRpcController::UpdateIsEip1559,
                              weak_ptr_factory_.GetWeakPtr(), chain_id));
}

void EthJsonRpcController::UpdateIsEip1559(const std::string& chain_id,
                                           bool success,
                                           bool is_eip1559) {
  if (!success)
    return;

  bool changed = false;
  if (chain_id == brave_wallet::mojom::kLocalhostChainId) {
    changed = prefs_->GetBoolean(kSupportEip1559OnLocalhostChain) != is_eip1559;
    prefs_->SetBoolean(kSupportEip1559OnLocalhostChain, is_eip1559);
  } else {
    ListPrefUpdate update(prefs_, kBraveWalletCustomNetworks);
    for (base::Value& custom_network : update.Get()->GetList()) {
      if (!custom_network.is_dict())
        continue;

      const std::string* id = custom_network.FindStringKey("chainId");
      if (!id || *id != chain_id)
        continue;

      changed = custom_network.FindBoolKey("is_eip1559").value_or(false) !=
                is_eip1559;
      custom_network.SetBoolKey("is_eip1559", is_eip1559);
      // Break the loop cuz we don't expect multiple entries with the same
      // chainId in the list.
      break;
    }
  }

  if (!changed)
    return;

  for (const auto& observer : observers_) {
    observer->OnIsEip1559Changed(chain_id, is_eip1559);
  }
}

void EthJsonRpcController::FireNetworkChanged() {
  for (const auto& observer : observers_) {
    observer->ChainChangedEvent(GetChainId());
  }
}

std::string EthJsonRpcController::GetChainId() const {
  return chain_id_;
}

void EthJsonRpcController::GetChainId(
    mojom::EthJsonRpcController::GetChainIdCallback callback) {
  std::move(callback).Run(GetChainId());
}

void EthJsonRpcController::GetBlockTrackerUrl(
    mojom::EthJsonRpcController::GetBlockTrackerUrlCallback callback) {
  std::move(callback).Run(GetBlockTrackerUrlFromNetwork(GetChainId()).spec());
}

void EthJsonRpcController::GetAllNetworks(GetAllNetworksCallback callback) {
  std::vector<mojom::EthereumChainPtr> all_chains;
  brave_wallet::GetAllChains(prefs_, &all_chains);
  std::move(callback).Run(std::move(all_chains));
}

std::string EthJsonRpcController::GetNetworkUrl() const {
  return network_url_.spec();
}

void EthJsonRpcController::GetNetworkUrl(
    mojom::EthJsonRpcController::GetNetworkUrlCallback callback) {
  std::move(callback).Run(GetNetworkUrl());
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
  std::string data;
  if (!erc20::BalanceOf(address, &data)) {
    std::move(callback).Run(false, "");
    return;
  }

  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetERC20TokenBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
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

void EthJsonRpcController::GetERC20TokenAllowance(
    const std::string& contract_address,
    const std::string& owner_address,
    const std::string& spender_address,
    EthJsonRpcController::GetERC20TokenAllowanceCallback callback) {
  std::string data;
  if (!erc20::Allowance(owner_address, spender_address, &data)) {
    std::move(callback).Run(false, "");
    return;
  }

  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetERC20TokenAllowance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  Request(eth_call("", contract_address, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void EthJsonRpcController::OnGetERC20TokenAllowance(
    GetERC20TokenAllowanceCallback callback,
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

void EthJsonRpcController::EnsRegistryGetResolver(
    const std::string& chain_id,
    const std::string& domain,
    StringResultCallback callback) {
  const std::string contract_address = GetEnsRegistryContractAddress(chain_id);
  if (contract_address.empty()) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string data;
  if (!ens::Resolver(domain, &data)) {
    std::move(callback).Run(false, "");
    return;
  }

  GURL network_url = GetNetworkURL(prefs_, chain_id);
  if (!network_url.is_valid()) {
    std::move(callback).Run(false, "");
    return;
  }

  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnEnsRegistryGetResolver,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth_call("", contract_address, "", "", "", data, "latest"),
                  true, network_url, std::move(internal_callback));
}

void EthJsonRpcController::OnEnsRegistryGetResolver(
    StringResultCallback callback,
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  DCHECK(callback);
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string resolver_address;
  if (!ParseAddressResult(body, &resolver_address) ||
      resolver_address.empty()) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, resolver_address);
}

void EthJsonRpcController::EnsResolverGetContentHash(
    const std::string& chain_id,
    const std::string& domain,
    StringResultCallback callback) {
  auto internal_callback = base::BindOnce(
      &EthJsonRpcController::ContinueEnsResolverGetContentHash,
      weak_ptr_factory_.GetWeakPtr(), chain_id, domain, std::move(callback));
  EnsRegistryGetResolver(chain_id, domain, std::move(internal_callback));
}

void EthJsonRpcController::ContinueEnsResolverGetContentHash(
    const std::string& chain_id,
    const std::string& domain,
    StringResultCallback callback,
    bool success,
    const std::string& resolver_address) {
  if (!success || resolver_address.empty()) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string data;
  if (!ens::ContentHash(domain, &data)) {
    std::move(callback).Run(false, "");
    return;
  }

  GURL network_url = GetNetworkURL(prefs_, chain_id);
  if (!network_url.is_valid()) {
    std::move(callback).Run(false, "");
    return;
  }

  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnEnsResolverGetContentHash,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth_call("", resolver_address, "", "", "", data, "latest"),
                  true, network_url, std::move(internal_callback));
}

void EthJsonRpcController::OnEnsResolverGetContentHash(
    StringResultCallback callback,
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  DCHECK(callback);
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string content_hash;
  if (!ParseEnsResolverContentHash(body, &content_hash) ||
      content_hash.empty()) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, content_hash);
}

void EthJsonRpcController::EnsGetEthAddr(const std::string& domain,
                                         EnsGetEthAddrCallback callback) {
  if (!IsValidDomain(domain)) {
    std::move(callback).Run(false, "");
    return;
  }

  auto internal_callback = base::BindOnce(
      &EthJsonRpcController::ContinueEnsGetEthAddr,
      weak_ptr_factory_.GetWeakPtr(), domain, std::move(callback));
  EnsRegistryGetResolver(chain_id_, domain, std::move(internal_callback));
}

void EthJsonRpcController::ContinueEnsGetEthAddr(
    const std::string& domain,
    StringResultCallback callback,
    bool success,
    const std::string& resolver_address) {
  if (!success || resolver_address.empty()) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string data;
  if (!ens::Addr(domain, &data)) {
    std::move(callback).Run(false, "");
    return;
  }

  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnEnsGetEthAddr,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  Request(eth_call("", resolver_address, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void EthJsonRpcController::OnEnsGetEthAddr(
    StringResultCallback callback,
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  DCHECK(callback);
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string address;
  if (!ParseAddressResult(body, &address) || address.empty()) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, address);
}

void EthJsonRpcController::UnstoppableDomainsProxyReaderGetMany(
    const std::string& chain_id,
    const std::string& domain,
    const std::vector<std::string>& keys,
    UnstoppableDomainsProxyReaderGetManyCallback callback) {
  const std::string contract_address =
      GetUnstoppableDomainsProxyReaderContractAddress(chain_id);
  if (contract_address.empty()) {
    std::move(callback).Run(false, std::vector<std::string>());
    return;
  }

  std::string data;
  if (!unstoppable_domains::GetMany(keys, domain, &data)) {
    std::move(callback).Run(false, std::vector<std::string>());
    return;
  }

  GURL network_url = GetNetworkURL(prefs_, chain_id);
  if (!network_url.is_valid()) {
    std::move(callback).Run(false, std::vector<std::string>());
    return;
  }

  auto internal_callback = base::BindOnce(
      &EthJsonRpcController::OnUnstoppableDomainsProxyReaderGetMany,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth_call("", contract_address, "", "", "", data, "latest"),
                  true, network_url, std::move(internal_callback));
}

void EthJsonRpcController::OnUnstoppableDomainsProxyReaderGetMany(
    UnstoppableDomainsProxyReaderGetManyCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, std::vector<std::string>());
    return;
  }

  std::vector<std::string> values;
  if (!ParseUnstoppableDomainsProxyReaderGetMany(body, &values)) {
    std::move(callback).Run(false, std::vector<std::string>());
    return;
  }

  std::move(callback).Run(true, values);
}

void EthJsonRpcController::UnstoppableDomainsGetEthAddr(
    const std::string& domain,
    UnstoppableDomainsGetEthAddrCallback callback) {
  if (!IsValidDomain(domain)) {
    std::move(callback).Run(false, "");
    return;
  }

  const std::string contract_address =
      GetUnstoppableDomainsProxyReaderContractAddress(chain_id_);
  if (contract_address.empty()) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string data;
  if (!unstoppable_domains::Get(kCryptoEthAddressKey, domain, &data)) {
    std::move(callback).Run(false, "");
    return;
  }

  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnUnstoppableDomainsGetEthAddr,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  Request(eth_call("", contract_address, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void EthJsonRpcController::OnUnstoppableDomainsGetEthAddr(
    UnstoppableDomainsGetEthAddrCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string address;
  if (!ParseUnstoppableDomainsProxyReaderGet(body, &address) ||
      address.empty()) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, address);
}

GURL EthJsonRpcController::GetBlockTrackerUrlFromNetwork(std::string chain_id) {
  std::vector<mojom::EthereumChainPtr> networks;
  brave_wallet::GetAllChains(prefs_, &networks);
  for (const auto& network : networks) {
    if (network->chain_id != chain_id)
      continue;
    if (network->block_explorer_urls.size())
      return GURL(network->block_explorer_urls.front());
  }
  return GURL();
}

void EthJsonRpcController::GetEstimateGas(const std::string& from_address,
                                          const std::string& to_address,
                                          const std::string& gas,
                                          const std::string& gas_price,
                                          const std::string& value,
                                          const std::string& data,
                                          GetEstimateGasCallback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetEstimateGas,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth_estimateGas(from_address, to_address, gas, gas_price,
                                 value, data, "latest"),
                 true, std::move(internal_callback));
}

void EthJsonRpcController::OnGetEstimateGas(
    GetEstimateGasCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string result;
  if (!ParseEthEstimateGas(body, &result)) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, result);
}

void EthJsonRpcController::GetGasPrice(GetGasPriceCallback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetGasPrice,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth_gasPrice(), true, std::move(internal_callback));
}

void EthJsonRpcController::OnGetGasPrice(
    GetGasPriceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string result;
  if (!ParseEthGasPrice(body, &result)) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, result);
}

void EthJsonRpcController::GetIsEip1559(GetIsEip1559Callback callback) {
  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetIsEip1559,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth_getBlockByNumber("latest", false), true,
                 std::move(internal_callback));
}

void EthJsonRpcController::OnGetIsEip1559(
    GetIsEip1559Callback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, false);
    return;
  }

  base::Value result;
  if (!ParseResult(body, &result) || !result.is_dict()) {
    std::move(callback).Run(false, false);
    return;
  }

  const std::string* base_fee = result.FindStringKey("baseFeePerGas");
  std::move(callback).Run(true, base_fee && !base_fee->empty());
}

bool EthJsonRpcController::IsValidDomain(const std::string& domain) {
  static const base::NoDestructor<re2::RE2> kDomainRegex(kDomainPattern);
  return re2::RE2::FullMatch(domain, kDomainPattern);
}

void EthJsonRpcController::GetERC721OwnerOf(const std::string& contract,
                                            const std::string& token_id,
                                            GetERC721OwnerOfCallback callback) {
  uint256_t token_id_uint = 0;
  if (!HexValueToUint256(token_id, &token_id_uint)) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string data;
  if (!erc721::OwnerOf(token_id_uint, &data)) {
    std::move(callback).Run(false, "");
    return;
  }

  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetERC721OwnerOf,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  Request(eth_call("", contract, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void EthJsonRpcController::OnGetERC721OwnerOf(
    GetERC721OwnerOfCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string address;
  if (!ParseAddressResult(body, &address) || address.empty()) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, address);
}

void EthJsonRpcController::GetERC721TokenBalance(
    const std::string& contract_address,
    const std::string& token_id,
    const std::string& account_address,
    GetERC721TokenBalanceCallback callback) {
  const auto eth_account_address = EthAddress::FromHex(account_address);
  if (eth_account_address.IsEmpty()) {
    std::move(callback).Run(false, "");
    return;
  }

  auto internal_callback = base::BindOnce(
      &EthJsonRpcController::ContinueGetERC721TokenBalance,
      weak_ptr_factory_.GetWeakPtr(), eth_account_address.ToChecksumAddress(),
      std::move(callback));
  GetERC721OwnerOf(contract_address, token_id, std::move(internal_callback));
}

void EthJsonRpcController::ContinueGetERC721TokenBalance(
    const std::string& account_address,
    GetERC721TokenBalanceCallback callback,
    bool success,
    const std::string& owner_address) {
  if (!success || owner_address.empty()) {
    std::move(callback).Run(false, "");
    return;
  }

  bool is_owner = owner_address == account_address;
  std::move(callback).Run(true, is_owner ? "0x1" : "0x0");
}

void EthJsonRpcController::GetSupportsInterface(
    const std::string& contract_address,
    const std::string& interface_id,
    GetSupportsInterfaceCallback callback) {
  std::string data;
  if (!erc165::SupportsInterface(interface_id, &data)) {
    std::move(callback).Run(false, false);
  }

  auto internal_callback =
      base::BindOnce(&EthJsonRpcController::OnGetSupportsInterface,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  Request(eth_call("", contract_address, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void EthJsonRpcController::OnGetSupportsInterface(
    GetSupportsInterfaceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(false, false);
    return;
  }

  bool is_supported = false;
  if (!ParseBoolResult(body, &is_supported)) {
    std::move(callback).Run(false, false);
    return;
  }

  std::move(callback).Run(true, is_supported);
}

void EthJsonRpcController::GetPendingSwitchChainRequests(
    GetPendingSwitchChainRequestsCallback callback) {
  std::vector<mojom::SwitchChainRequestPtr> requests;
  for (const auto& request : switch_chain_requests_) {
    requests.push_back(
        mojom::SwitchChainRequest::New(request.first, request.second));
  }
  std::move(callback).Run(std::move(requests));
}

void EthJsonRpcController::NotifySwitchChainRequestProcessed(
    bool approved,
    const GURL& origin) {
  if (!switch_chain_requests_.contains(origin) ||
      !switch_chain_callbacks_.contains(origin)) {
    return;
  }
  if (approved) {
    // We already check chain id validiy in
    // EthJsonRpcController::AddSwitchEthereumChainRequest so this should always
    // be successful unless chain id differs or we add more check other than
    // chain id
    CHECK(SetNetwork(switch_chain_requests_[origin]));
  }
  auto callback = std::move(switch_chain_callbacks_[origin]);
  switch_chain_requests_.erase(origin);
  switch_chain_callbacks_.erase(origin);

  if (approved)
    std::move(callback).Run(0, "");
  else
    std::move(callback).Run(
        static_cast<int>(ProviderErrors::kUserRejectedRequest),
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
}

bool EthJsonRpcController::AddSwitchEthereumChainRequest(
    const std::string& chain_id,
    const GURL& origin,
    SwitchEthereumChainRequestCallback callback) {
  if (!GetNetworkURL(prefs_, chain_id).is_valid()) {
    std::move(callback).Run(
        static_cast<int>(ProviderErrors::kUnknownChain),
        l10n_util::GetStringFUTF8(IDS_WALLET_UNKNOWN_CHAIN,
                                  base::ASCIIToUTF16(chain_id)));
    return false;
  }

  // Already on the chain
  if (GetChainId() == chain_id) {
    std::move(callback).Run(0, "");
    return false;
  }

  // There can be only 1 request per origin
  if (switch_chain_requests_.contains(origin)) {
    std::move(callback).Run(
        static_cast<int>(ProviderErrors::kUserRejectedRequest),
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    return false;
  }
  switch_chain_requests_[origin] = chain_id;
  switch_chain_callbacks_[origin] = std::move(callback);
  return true;
}

}  // namespace brave_wallet
