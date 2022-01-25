/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_service.h"

#include <utility>

#include "base/bind.h"
#include "base/environment.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_requests.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/browser/fil_requests.h"
#include "brave/components/brave_wallet/browser/fil_response_parser.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/solana_requests.h"
#include "brave/components/brave_wallet/browser/solana_response_parser.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

namespace {

// The domain name should be a-z | A-Z | 0-9 and hyphen(-).
// The domain name should not start or end with hyphen (-).
// The domain name can be a subdomain.
// TLD & TLD-1 must be at least two characters.
constexpr char kDomainPattern[] =
    "(?:[A-Za-z0-9][A-Za-z0-9-]*[A-Za-z0-9]\\.)+[A-Za-z]{2,}$";

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("json_rpc_service", R"(
      semantics {
        sender: "JSON RPC Service"
        description:
          "This service is used to communicate with Ethereum nodes "
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

void ChainIdValidationResponse(
    base::OnceCallback<void(bool)> callback,
    const std::string& chain_id,
    const int http_code,
    const std::string& response,
    const base::flat_map<std::string, std::string>& headers) {
  std::string result;
  bool success = (brave_wallet::ParseSingleStringResult(response, &result) &&
                  (result == chain_id));
  std::move(callback).Run(success);
}

bool IsChainExist(PrefService* prefs, const std::string& chain_id) {
  std::vector<::brave_wallet::mojom::EthereumChainPtr> custom_chains;
  brave_wallet::GetAllChains(prefs, &custom_chains);
  for (const auto& it : custom_chains) {
    if (it->chain_id == chain_id) {
      return true;
    }
  }
  return false;
}

}  // namespace

namespace brave_wallet {

JsonRpcService::JsonRpcService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs)
    : api_request_helper_(new api_request_helper::APIRequestHelper(
          GetNetworkTrafficAnnotationTag(),
          url_loader_factory)),
      prefs_(prefs),
      weak_ptr_factory_(this) {
  SetNetwork(prefs_->GetString(kBraveWalletCurrentChainId),
             base::BindOnce([](bool success) {
               if (!success)
                 LOG(ERROR) << "Could not set netowrk from JsonRpcService()";
             }));
}

void JsonRpcService::SetAPIRequestHelperForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_request_helper_.reset(new api_request_helper::APIRequestHelper(
      GetNetworkTrafficAnnotationTag(), url_loader_factory));
}

JsonRpcService::~JsonRpcService() {}

mojo::PendingRemote<mojom::JsonRpcService> JsonRpcService::MakeRemote() {
  mojo::PendingRemote<mojom::JsonRpcService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void JsonRpcService::Bind(
    mojo::PendingReceiver<mojom::JsonRpcService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void JsonRpcService::AddObserver(
    ::mojo::PendingRemote<mojom::JsonRpcServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

void JsonRpcService::Request(const std::string& json_payload,
                             bool auto_retry_on_network_change,
                             RequestCallback callback) {
  RequestInternal(json_payload, auto_retry_on_network_change, network_url_,
                  std::move(callback));
}

void JsonRpcService::RequestInternal(const std::string& json_payload,
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

  api_request_helper_->Request("POST", network_url, json_payload,
                               "application/json", auto_retry_on_network_change,
                               std::move(callback), request_headers);
}

void JsonRpcService::FirePendingRequestCompleted(const std::string& chain_id,
                                                 const std::string& error) {
  for (const auto& observer : observers_) {
    observer->OnAddEthereumChainRequestCompleted(chain_id, error);
  }
}

bool JsonRpcService::HasRequestFromOrigin(const GURL& origin) const {
  for (const auto& request : add_chain_pending_requests_) {
    if (request.second.origin == origin)
      return true;
  }
  return false;
}

void JsonRpcService::GetPendingChainRequests(
    GetPendingChainRequestsCallback callback) {
  std::vector<mojom::EthereumChainPtr> all_chains;
  for (const auto& request : add_chain_pending_requests_) {
    all_chains.push_back(request.second.request.Clone());
  }
  std::move(callback).Run(std::move(all_chains));
}

void JsonRpcService::AddEthereumChain(mojom::EthereumChainPtr chain,
                                      AddEthereumChainCallback callback) {
  auto chain_id = chain->chain_id;
  auto url = chain->rpc_urls.size() ? GURL(chain->rpc_urls.front()) : GURL();
  if (!url.is_valid()) {
    std::move(callback).Run(
        chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                  base::ASCIIToUTF16(url.spec())));
    return;
  }

  if (IsChainExist(prefs_, chain_id)) {
    std::move(callback).Run(
        chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_SETTINGS_WALLET_NETWORKS_EXISTS));
    return;
  }

  auto result = base::BindOnce(&JsonRpcService::OnEthChainIdValidated,
                               weak_ptr_factory_.GetWeakPtr(), std::move(chain),
                               std::move(callback));
  RequestInternal(
      eth::eth_chainId(), true, url,
      base::BindOnce(&ChainIdValidationResponse, std::move(result), chain_id));
}

void JsonRpcService::OnEthChainIdValidated(mojom::EthereumChainPtr chain,
                                           AddEthereumChainCallback callback,
                                           bool success) {
  if (!success) {
    std::move(callback).Run(
        chain->chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                  base::ASCIIToUTF16(chain->rpc_urls.front())));
    return;
  }

  auto chain_id = chain->chain_id;
  AddCustomNetwork(prefs_, std::move(chain));
  std::move(callback).Run(chain_id, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::AddEthereumChainForOrigin(
    mojom::EthereumChainPtr chain,
    const GURL& origin,
    AddEthereumChainForOriginCallback callback) {
  DCHECK_EQ(origin, url::Origin::Create(origin).GetURL());
  auto chain_id = chain->chain_id;
  if (IsChainExist(prefs_, chain_id)) {
    std::move(callback).Run(
        chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_SETTINGS_WALLET_NETWORKS_EXISTS));
    return;
  }
  if (!origin.is_valid() || add_chain_pending_requests_.contains(chain_id) ||
      HasRequestFromOrigin(origin)) {
    std::move(callback).Run(
        chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    return;
  }
  auto url = chain->rpc_urls.size() ? GURL(chain->rpc_urls.front()) : GURL();
  if (!url.is_valid()) {
    std::move(callback).Run(
        chain->chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                  base::ASCIIToUTF16(chain->rpc_urls.front())));
    return;
  }

  auto result = base::BindOnce(&JsonRpcService::OnEthChainIdValidatedForOrigin,
                               weak_ptr_factory_.GetWeakPtr(), std::move(chain),
                               origin, std::move(callback));

  RequestInternal(
      eth::eth_chainId(), true, url,
      base::BindOnce(&ChainIdValidationResponse, std::move(result), chain_id));
}

void JsonRpcService::OnEthChainIdValidatedForOrigin(
    mojom::EthereumChainPtr chain,
    const GURL& origin,
    AddEthereumChainForOriginCallback callback,
    bool success) {
  if (!success) {
    std::move(callback).Run(
        chain->chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                  base::ASCIIToUTF16(chain->rpc_urls.front())));
    return;
  }

  auto chain_id = chain->chain_id;
  add_chain_pending_requests_[chain_id] =
      EthereumChainRequest(origin, std::move(*chain));
  std::move(callback).Run(chain_id, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::AddEthereumChainRequestCompleted(
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

void JsonRpcService::RemoveEthereumChain(const std::string& chain_id,
                                         RemoveEthereumChainCallback callback) {
  RemoveCustomNetwork(prefs_, chain_id);
  std::move(callback).Run(true);
}

bool JsonRpcService::SetNetwork(const std::string& chain_id) {
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

void JsonRpcService::SetNetwork(const std::string& chain_id,
                                SetNetworkCallback callback) {
  if (!SetNetwork(chain_id))
    std::move(callback).Run(false);
  else
    std::move(callback).Run(true);
}

void JsonRpcService::GetNetwork(GetNetworkCallback callback) {
  std::move(callback).Run(GetChain(prefs_, chain_id_));
}

void JsonRpcService::MaybeUpdateIsEip1559(const std::string& chain_id) {
  // Only try to update is_eip1559 for localhost or custom chains.
  auto chain = GetKnownChain(prefs_, chain_id);
  if (chain && chain_id != brave_wallet::mojom::kLocalhostChainId)
    return;

  GetIsEip1559(base::BindOnce(&JsonRpcService::UpdateIsEip1559,
                              weak_ptr_factory_.GetWeakPtr(), chain_id));
}

void JsonRpcService::UpdateIsEip1559(const std::string& chain_id,
                                     bool is_eip1559,
                                     mojom::ProviderError error,
                                     const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess)
    return;

  // Disable EIP-1559 on selected networks, by overwriting is_eip1559 to
  // false.
  //
  // This list needs to be updated until we have a generic EIP-1559 gas
  // oracle. See: https://github.com/brave/brave-browser/issues/20469.
  if (chain_id == "0x13881" ||  // Polygon Mumbai Testnet
      chain_id == "0x89" ||     // Polygon Mainnet
      chain_id == "0xa86a" ||   // Avalanche Mainnet
      chain_id == "0xa869"      // Avalanche Fuji Testnet
  ) {
    is_eip1559 = false;
  }

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

void JsonRpcService::FireNetworkChanged() {
  for (const auto& observer : observers_) {
    observer->ChainChangedEvent(GetChainId());
  }
}

std::string JsonRpcService::GetChainId() const {
  return chain_id_;
}

void JsonRpcService::GetChainId(
    mojom::JsonRpcService::GetChainIdCallback callback) {
  std::move(callback).Run(GetChainId());
}

void JsonRpcService::GetBlockTrackerUrl(
    mojom::JsonRpcService::GetBlockTrackerUrlCallback callback) {
  std::move(callback).Run(GetBlockTrackerUrlFromNetwork(GetChainId()).spec());
}

void JsonRpcService::GetAllNetworks(GetAllNetworksCallback callback) {
  std::vector<mojom::EthereumChainPtr> all_chains;
  brave_wallet::GetAllChains(prefs_, &all_chains);
  std::move(callback).Run(std::move(all_chains));
}

std::string JsonRpcService::GetNetworkUrl() const {
  return network_url_.spec();
}

void JsonRpcService::GetNetworkUrl(
    mojom::JsonRpcService::GetNetworkUrlCallback callback) {
  std::move(callback).Run(GetNetworkUrl());
}

void JsonRpcService::SetCustomNetworkForTesting(const std::string& chain_id,
                                                const GURL& network_url) {
  chain_id_ = chain_id;
  network_url_ = network_url;
  FireNetworkChanged();
}

void JsonRpcService::GetBlockNumber(GetBlockNumberCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetBlockNumber,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth::eth_blockNumber(), true, std::move(internal_callback));
}

void JsonRpcService::OnGetBlockNumber(
    GetBlockNumberCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        0, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  uint256_t block_number;
  if (!eth::ParseEthGetBlockNumber(body, &block_number)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run(0, error, error_message);
    return;
  }

  std::move(callback).Run(block_number, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetBalance(const std::string& address,
                                mojom::CoinType coin,
                                JsonRpcService::GetBalanceCallback callback) {
  if (coin == mojom::CoinType::ETH) {
    auto internal_callback =
        base::BindOnce(&JsonRpcService::OnEthGetBalance,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));
    return Request(eth::eth_getBalance(address, "latest"), true,
                   std::move(internal_callback));
  } else if (coin == mojom::CoinType::FIL) {
    auto internal_callback =
        base::BindOnce(&JsonRpcService::OnFilGetBalance,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));
    return Request(fil_getBalance(address), true, std::move(internal_callback));
  }
}

void JsonRpcService::OnEthGetBalance(
    GetBalanceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::string balance;
  if (!eth::ParseEthGetBalance(body, &balance)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(balance, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::OnFilGetBalance(
    GetBalanceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::string balance;
  if (!ParseFilGetBalance(body, &balance)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(balance, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetTransactionCount(const std::string& address,
                                         GetTxCountCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetTransactionCount,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth::eth_getTransactionCount(address, "latest"), true,
                 std::move(internal_callback));
}

void JsonRpcService::OnGetTransactionCount(
    GetTxCountCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        0, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  uint256_t count;
  if (!eth::ParseEthGetTransactionCount(body, &count)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run(0, error, error_message);
    return;
  }

  std::move(callback).Run(count, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetTransactionReceipt(const std::string& tx_hash,
                                           GetTxReceiptCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetTransactionReceipt,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth::eth_getTransactionReceipt(tx_hash), true,
                 std::move(internal_callback));
}

void JsonRpcService::OnGetTransactionReceipt(
    GetTxReceiptCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  TransactionReceipt receipt;
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        receipt, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  if (!eth::ParseEthGetTransactionReceipt(body, &receipt)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run(receipt, error, error_message);
    return;
  }

  std::move(callback).Run(receipt, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::SendRawTransaction(const std::string& signed_tx,
                                        SendRawTxCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnSendRawTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth::eth_sendRawTransaction(signed_tx), true,
                 std::move(internal_callback));
}

void JsonRpcService::OnSendRawTransaction(
    SendRawTxCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::string tx_hash;
  if (!eth::ParseEthSendRawTransaction(body, &tx_hash)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(tx_hash, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetERC20TokenBalance(
    const std::string& contract,
    const std::string& address,
    JsonRpcService::GetERC20TokenBalanceCallback callback) {
  std::string data;
  if (!erc20::BalanceOf(address, &data)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetERC20TokenBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  Request(eth::eth_call("", contract, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void JsonRpcService::OnGetERC20TokenBalance(
    GetERC20TokenBalanceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::string result;
  if (!eth::ParseEthCall(body, &result)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }
  std::move(callback).Run(result, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetERC20TokenAllowance(
    const std::string& contract_address,
    const std::string& owner_address,
    const std::string& spender_address,
    JsonRpcService::GetERC20TokenAllowanceCallback callback) {
  std::string data;
  if (!erc20::Allowance(owner_address, spender_address, &data)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetERC20TokenAllowance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  Request(eth::eth_call("", contract_address, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void JsonRpcService::OnGetERC20TokenAllowance(
    GetERC20TokenAllowanceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::string result;
  if (!eth::ParseEthCall(body, &result)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }
  std::move(callback).Run(result, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::EnsRegistryGetResolver(const std::string& chain_id,
                                            const std::string& domain,
                                            StringResultCallback callback) {
  const std::string contract_address = GetEnsRegistryContractAddress(chain_id);
  if (contract_address.empty()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::string data;
  if (!ens::Resolver(domain, &data)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  GURL network_url = GetNetworkURL(prefs_, chain_id);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnEnsRegistryGetResolver,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(
      eth::eth_call("", contract_address, "", "", "", data, "latest"), true,
      network_url, std::move(internal_callback));
}

void JsonRpcService::OnEnsRegistryGetResolver(
    StringResultCallback callback,
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  DCHECK(callback);
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string resolver_address;
  if (!eth::ParseAddressResult(body, &resolver_address) ||
      resolver_address.empty()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(resolver_address, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::EnsResolverGetContentHash(const std::string& chain_id,
                                               const std::string& domain,
                                               StringResultCallback callback) {
  auto internal_callback = base::BindOnce(
      &JsonRpcService::ContinueEnsResolverGetContentHash,
      weak_ptr_factory_.GetWeakPtr(), chain_id, domain, std::move(callback));
  EnsRegistryGetResolver(chain_id, domain, std::move(internal_callback));
}

void JsonRpcService::ContinueEnsResolverGetContentHash(
    const std::string& chain_id,
    const std::string& domain,
    StringResultCallback callback,
    const std::string& resolver_address,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess || resolver_address.empty()) {
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::string data;
  if (!ens::ContentHash(domain, &data)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  GURL network_url = GetNetworkURL(prefs_, chain_id);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnEnsResolverGetContentHash,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(
      eth::eth_call("", resolver_address, "", "", "", data, "latest"), true,
      network_url, std::move(internal_callback));
}

void JsonRpcService::OnEnsResolverGetContentHash(
    StringResultCallback callback,
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  DCHECK(callback);
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string content_hash;
  if (!eth::ParseEnsResolverContentHash(body, &content_hash) ||
      content_hash.empty()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(content_hash, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::EnsGetEthAddr(const std::string& domain,
                                   EnsGetEthAddrCallback callback) {
  if (!IsValidDomain(domain)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback = base::BindOnce(
      &JsonRpcService::ContinueEnsGetEthAddr, weak_ptr_factory_.GetWeakPtr(),
      domain, std::move(callback));
  EnsRegistryGetResolver(chain_id_, domain, std::move(internal_callback));
}

void JsonRpcService::ContinueEnsGetEthAddr(const std::string& domain,
                                           StringResultCallback callback,
                                           const std::string& resolver_address,
                                           mojom::ProviderError error,
                                           const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess || resolver_address.empty()) {
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::string data;
  if (!ens::Addr(domain, &data)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnEnsGetEthAddr,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  Request(eth::eth_call("", resolver_address, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void JsonRpcService::OnEnsGetEthAddr(
    StringResultCallback callback,
    int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  DCHECK(callback);
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string address;
  if (!eth::ParseAddressResult(body, &address) || address.empty()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(address, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::UnstoppableDomainsProxyReaderGetMany(
    const std::string& chain_id,
    const std::string& domain,
    const std::vector<std::string>& keys,
    UnstoppableDomainsProxyReaderGetManyCallback callback) {
  const std::string contract_address =
      GetUnstoppableDomainsProxyReaderContractAddress(chain_id);
  if (contract_address.empty()) {
    std::move(callback).Run(
        std::vector<std::string>(), mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::string data;
  if (!unstoppable_domains::GetMany(keys, domain, &data)) {
    std::move(callback).Run(
        std::vector<std::string>(), mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  GURL network_url = GetNetworkURL(prefs_, chain_id);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        std::vector<std::string>(), mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnUnstoppableDomainsProxyReaderGetMany,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(
      eth::eth_call("", contract_address, "", "", "", data, "latest"), true,
      network_url, std::move(internal_callback));
}

void JsonRpcService::OnUnstoppableDomainsProxyReaderGetMany(
    UnstoppableDomainsProxyReaderGetManyCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        std::vector<std::string>(), mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::vector<std::string> values;
  if (!eth::ParseUnstoppableDomainsProxyReaderGetMany(body, &values)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run(std::vector<std::string>(), error, error_message);
    return;
  }

  std::move(callback).Run(values, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::UnstoppableDomainsGetEthAddr(
    const std::string& domain,
    UnstoppableDomainsGetEthAddrCallback callback) {
  if (!IsValidDomain(domain)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  const std::string contract_address =
      GetUnstoppableDomainsProxyReaderContractAddress(chain_id_);
  if (contract_address.empty()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::string data;
  if (!unstoppable_domains::Get(kCryptoEthAddressKey, domain, &data)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnUnstoppableDomainsGetEthAddr,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  Request(eth::eth_call("", contract_address, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void JsonRpcService::OnUnstoppableDomainsGetEthAddr(
    UnstoppableDomainsGetEthAddrCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string address;
  if (!eth::ParseUnstoppableDomainsProxyReaderGet(body, &address) ||
      address.empty()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(address, mojom::ProviderError::kSuccess, "");
}

GURL JsonRpcService::GetBlockTrackerUrlFromNetwork(std::string chain_id) {
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

void JsonRpcService::GetEstimateGas(const std::string& from_address,
                                    const std::string& to_address,
                                    const std::string& gas,
                                    const std::string& gas_price,
                                    const std::string& value,
                                    const std::string& data,
                                    GetEstimateGasCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetEstimateGas,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth::eth_estimateGas(from_address, to_address, gas, gas_price,
                                      value, data, "latest"),
                 true, std::move(internal_callback));
}

void JsonRpcService::OnGetEstimateGas(
    GetEstimateGasCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string result;
  if (!eth::ParseEthEstimateGas(body, &result)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(result, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetGasPrice(GetGasPriceCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetGasPrice,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth::eth_gasPrice(), true, std::move(internal_callback));
}

void JsonRpcService::OnGetGasPrice(
    GetGasPriceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string result;
  if (!eth::ParseEthGasPrice(body, &result)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(result, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetIsEip1559(GetIsEip1559Callback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetIsEip1559,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(eth::eth_getBlockByNumber("latest", false), true,
                 std::move(internal_callback));
}

void JsonRpcService::OnGetIsEip1559(
    GetIsEip1559Callback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        false, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  base::Value result;
  if (!ParseResult(body, &result) || !result.is_dict()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run(false, error, error_message);
    return;
  }

  const std::string* base_fee = result.FindStringKey("baseFeePerGas");
  std::move(callback).Run(base_fee && !base_fee->empty(),
                          mojom::ProviderError::kSuccess, "");
}

bool JsonRpcService::IsValidDomain(const std::string& domain) {
  static const base::NoDestructor<re2::RE2> kDomainRegex(kDomainPattern);
  return re2::RE2::FullMatch(domain, kDomainPattern);
}

void JsonRpcService::GetERC721OwnerOf(const std::string& contract,
                                      const std::string& token_id,
                                      GetERC721OwnerOfCallback callback) {
  if (!EthAddress::IsValidAddress(contract)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  uint256_t token_id_uint = 0;
  if (!HexValueToUint256(token_id, &token_id_uint)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::string data;
  if (!erc721::OwnerOf(token_id_uint, &data)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetERC721OwnerOf,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  Request(eth::eth_call("", contract, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void JsonRpcService::OnGetERC721OwnerOf(
    GetERC721OwnerOfCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string address;
  if (!eth::ParseAddressResult(body, &address) || address.empty()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(address, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetERC721TokenBalance(
    const std::string& contract_address,
    const std::string& token_id,
    const std::string& account_address,
    GetERC721TokenBalanceCallback callback) {
  const auto eth_account_address = EthAddress::FromHex(account_address);
  if (eth_account_address.IsEmpty()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback = base::BindOnce(
      &JsonRpcService::ContinueGetERC721TokenBalance,
      weak_ptr_factory_.GetWeakPtr(), eth_account_address.ToChecksumAddress(),
      std::move(callback));
  GetERC721OwnerOf(contract_address, token_id, std::move(internal_callback));
}

void JsonRpcService::ContinueGetERC721TokenBalance(
    const std::string& account_address,
    GetERC721TokenBalanceCallback callback,
    const std::string& owner_address,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess || owner_address.empty()) {
    std::move(callback).Run("", error, error_message);
    return;
  }

  bool is_owner = owner_address == account_address;
  std::move(callback).Run(is_owner ? "0x1" : "0x0",
                          mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetSupportsInterface(
    const std::string& contract_address,
    const std::string& interface_id,
    GetSupportsInterfaceCallback callback) {
  if (!EthAddress::IsValidAddress(contract_address)) {
    std::move(callback).Run(
        false, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::string data;
  if (!erc165::SupportsInterface(interface_id, &data)) {
    std::move(callback).Run(
        false, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSupportsInterface,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  Request(eth::eth_call("", contract_address, "", "", "", data, "latest"), true,
          std::move(internal_callback));
}

void JsonRpcService::OnGetSupportsInterface(
    GetSupportsInterfaceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        false, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  bool is_supported = false;
  if (!ParseBoolResult(body, &is_supported)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(body, &error, &error_message);
    std::move(callback).Run(false, error, error_message);
    return;
  }

  std::move(callback).Run(is_supported, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetPendingSwitchChainRequests(
    GetPendingSwitchChainRequestsCallback callback) {
  std::vector<mojom::SwitchChainRequestPtr> requests;
  for (const auto& request : switch_chain_requests_) {
    requests.push_back(
        mojom::SwitchChainRequest::New(request.first, request.second));
  }
  std::move(callback).Run(std::move(requests));
}

void JsonRpcService::NotifySwitchChainRequestProcessed(bool approved,
                                                       const GURL& origin) {
  if (!switch_chain_requests_.contains(origin) ||
      !switch_chain_callbacks_.contains(origin)) {
    return;
  }
  if (approved) {
    // We already check chain id validiy in
    // JsonRpcService::AddSwitchEthereumChainRequest so this should always
    // be successful unless chain id differs or we add more check other than
    // chain id
    CHECK(SetNetwork(switch_chain_requests_[origin]));
  }
  auto callback = std::move(switch_chain_callbacks_[origin]);
  switch_chain_requests_.erase(origin);
  switch_chain_callbacks_.erase(origin);

  if (approved)
    std::move(callback).Run(mojom::ProviderError::kSuccess, "");
  else
    std::move(callback).Run(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
}

bool JsonRpcService::AddSwitchEthereumChainRequest(
    const std::string& chain_id,
    const GURL& origin,
    SwitchEthereumChainRequestCallback callback) {
  if (!GetNetworkURL(prefs_, chain_id).is_valid()) {
    std::move(callback).Run(
        mojom::ProviderError::kUnknownChain,
        l10n_util::GetStringFUTF8(IDS_WALLET_UNKNOWN_CHAIN,
                                  base::ASCIIToUTF16(chain_id)));
    return false;
  }

  // Already on the chain
  if (GetChainId() == chain_id) {
    std::move(callback).Run(mojom::ProviderError::kSuccess, "");
    return false;
  }

  // There can be only 1 request per origin
  if (switch_chain_requests_.contains(origin)) {
    std::move(callback).Run(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    return false;
  }
  switch_chain_requests_[origin] = chain_id;
  switch_chain_callbacks_[origin] = std::move(callback);
  return true;
}

void JsonRpcService::Reset() {
  ClearJsonRpcServiceProfilePrefs(prefs_);
  SetNetwork(prefs_->GetString(kBraveWalletCurrentChainId));

  add_chain_pending_requests_.clear();
  switch_chain_requests_.clear();
  // Reject pending suggest token requests when network changed.
  for (auto& callback : switch_chain_callbacks_) {
    std::move(callback.second)
        .Run(mojom::ProviderError::kUserRejectedRequest,
             l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
  }
  switch_chain_callbacks_.clear();
}

void JsonRpcService::GetSolanaBalance(const std::string& pubkey,
                                      GetSolanaBalanceCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(solana::getBalance(pubkey), true,
                 std::move(internal_callback));
}

void JsonRpcService::GetSPLTokenAccountBalance(
    const std::string& pubkey,
    GetSPLTokenAccountBalanceCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSPLTokenAccountBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  return Request(solana::getTokenAccountBalance(pubkey), true,
                 std::move(internal_callback));
}

void JsonRpcService::OnGetSolanaBalance(
    GetSolanaBalanceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        0u, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  uint64_t balance = 0;
  if (!solana::ParseGetBalance(body, &balance)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(body, &error, &error_message);
    std::move(callback).Run(0u, error, error_message);
    return;
  }

  std::move(callback).Run(balance, mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::OnGetSPLTokenAccountBalance(
    GetSPLTokenAccountBalanceCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", 0u, "", mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string amount, ui_amount_string;
  uint8_t decimals = 0;
  if (!solana::ParseGetTokenAccountBalance(body, &amount, &decimals,
                                           &ui_amount_string)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(body, &error, &error_message);
    std::move(callback).Run("", 0u, "", error, error_message);
    return;
  }

  std::move(callback).Run(amount, decimals, ui_amount_string,
                          mojom::SolanaProviderError::kSuccess, "");
}

}  // namespace brave_wallet
