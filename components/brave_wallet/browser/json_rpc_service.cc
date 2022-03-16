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
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
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
  std::vector<::brave_wallet::mojom::NetworkInfoPtr> custom_chains;
  brave_wallet::GetAllChains(prefs, brave_wallet::mojom::CoinType::ETH,
                             &custom_chains);
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
  if (!SetNetwork(GetCurrentChainId(prefs_, mojom::CoinType::ETH),
                  mojom::CoinType::ETH))
    LOG(ERROR) << "Could not set netowrk from JsonRpcService() for ETH";
  if (!SetNetwork(GetCurrentChainId(prefs_, mojom::CoinType::SOL),
                  mojom::CoinType::SOL))
    LOG(ERROR) << "Could not set netowrk from JsonRpcService() for SOL";
  if (!SetNetwork(GetCurrentChainId(prefs_, mojom::CoinType::FIL),
                  mojom::CoinType::FIL))
    LOG(ERROR) << "Could not set netowrk from JsonRpcService() for FIL";
}

void JsonRpcService::SetAPIRequestHelperForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_request_helper_.reset(new api_request_helper::APIRequestHelper(
      GetNetworkTrafficAnnotationTag(), url_loader_factory));
}

JsonRpcService::~JsonRpcService() {}

// static
void JsonRpcService::MigrateMultichainNetworks(PrefService* prefs) {
  // custom networks
  if (prefs->HasPrefPath(kBraveWalletCustomNetworksDeprecated)) {
    const base::Value* custom_networks =
        prefs->GetList(kBraveWalletCustomNetworksDeprecated);
    if (custom_networks) {
      base::Value new_custom_networks(base::Value::Type::DICTIONARY);
      new_custom_networks.SetKey(kEthereumPrefKey, custom_networks->Clone());

      prefs->Set(kBraveWalletCustomNetworks, new_custom_networks);

      prefs->ClearPref(kBraveWalletCustomNetworksDeprecated);
    }
  }
  // selected networks
  if (prefs->HasPrefPath(kBraveWalletCurrentChainId)) {
    const std::string chain_id = prefs->GetString(kBraveWalletCurrentChainId);
    DictionaryPrefUpdate update(prefs, kBraveWalletSelectedNetworks);
    base::Value* selected_networks = update.Get();
    if (selected_networks) {
      selected_networks->SetStringKey(kEthereumPrefKey, chain_id);
      prefs->ClearPref(kBraveWalletCurrentChainId);
    }
  }
}

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
                             base::Value id,
                             mojom::CoinType coin,
                             RequestCallback callback) {
  RequestInternal(
      json_payload, auto_retry_on_network_change, network_urls_[coin],
      base::BindOnce(&JsonRpcService::OnRequestResult, base::Unretained(this),
                     std::move(callback), std::move(id)));
}

void JsonRpcService::OnRequestResult(
    RequestCallback callback,
    base::Value id,
    const int code,
    const std::string& message,
    const base::flat_map<std::string, std::string>& headers) {
  bool reject;
  std::unique_ptr<base::Value> formed_response =
      GetProviderRequestReturnFromEthJsonResponse(code, message, &reject);
  std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                          "", false);
}

void JsonRpcService::RequestInternal(const std::string& json_payload,
                                     bool auto_retry_on_network_change,
                                     const GURL& network_url,
                                     RequestIntermediateCallback callback) {
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
  for (const auto& request : add_chain_pending_requests_origins_) {
    if (request.second == origin)
      return true;
  }
  return false;
}

void JsonRpcService::GetPendingChainRequests(
    GetPendingChainRequestsCallback callback) {
  std::vector<mojom::NetworkInfoPtr> all_chains;
  for (const auto& request : add_chain_pending_requests_) {
    all_chains.push_back(request.second.Clone());
  }
  std::move(callback).Run(std::move(all_chains));
}

void JsonRpcService::AddEthereumChain(mojom::NetworkInfoPtr chain,
                                      AddEthereumChainCallback callback) {
  auto chain_id = chain->chain_id;
  GURL url = GetFirstValidChainURL(chain->rpc_urls);

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

void JsonRpcService::OnEthChainIdValidated(mojom::NetworkInfoPtr chain,
                                           AddEthereumChainCallback callback,
                                           bool success) {
  if (!success) {
    std::move(callback).Run(
        chain->chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringFUTF8(
            IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
            base::ASCIIToUTF16(GetFirstValidChainURL(chain->rpc_urls).spec())));
    return;
  }

  auto chain_id = chain->chain_id;
  AddCustomNetwork(prefs_, std::move(chain));
  std::move(callback).Run(chain_id, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::AddEthereumChainForOrigin(
    mojom::NetworkInfoPtr chain,
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
  GURL url = GetFirstValidChainURL(chain->rpc_urls);
  if (!url.is_valid()) {
    std::move(callback).Run(
        chain->chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                  base::ASCIIToUTF16(url.spec())));
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
    mojom::NetworkInfoPtr chain,
    const GURL& origin,
    AddEthereumChainForOriginCallback callback,
    bool success) {
  if (!success) {
    std::move(callback).Run(
        chain->chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringFUTF8(
            IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
            base::ASCIIToUTF16(GetFirstValidChainURL(chain->rpc_urls).spec())));
    return;
  }

  auto chain_id = chain->chain_id;
  add_chain_pending_requests_[chain_id] = std::move(chain);
  add_chain_pending_requests_origins_[chain_id] = origin;
  std::move(callback).Run(chain_id, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::AddEthereumChainRequestCompleted(
    const std::string& chain_id,
    bool approved) {
  if (!add_chain_pending_requests_.contains(chain_id))
    return;
  if (approved) {
    AddCustomNetwork(prefs_, add_chain_pending_requests_.at(chain_id).Clone());
  }

  std::string error =
      approved ? std::string()
               : l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST);
  FirePendingRequestCompleted(chain_id, error);
  add_chain_pending_requests_.erase(chain_id);
  add_chain_pending_requests_origins_.erase(chain_id);
}

void JsonRpcService::RemoveEthereumChain(const std::string& chain_id,
                                         RemoveEthereumChainCallback callback) {
  RemoveCustomNetwork(prefs_, chain_id);
  std::move(callback).Run(true);
}

bool JsonRpcService::SetNetwork(const std::string& chain_id,
                                mojom::CoinType coin) {
  auto network_url = GetNetworkURL(prefs_, chain_id, coin);
  if (!network_url.is_valid()) {
    return false;
  }

  chain_ids_[coin] = chain_id;
  network_urls_[coin] = network_url;
  DictionaryPrefUpdate update(prefs_, kBraveWalletSelectedNetworks);
  base::Value* dict = update.Get();
  DCHECK(dict);
  auto key = GetPrefKeyForCoinType(coin);
  if (!key)
    return false;
  dict->SetStringKey(*key, chain_id);

  FireNetworkChanged(coin);
  if (coin == mojom::CoinType::ETH)
    MaybeUpdateIsEip1559(chain_id);
  return true;
}

void JsonRpcService::SetNetwork(const std::string& chain_id,
                                mojom::CoinType coin,
                                SetNetworkCallback callback) {
  if (!SetNetwork(chain_id, coin))
    std::move(callback).Run(false);
  else
    std::move(callback).Run(true);
}

void JsonRpcService::GetNetwork(mojom::CoinType coin,
                                GetNetworkCallback callback) {
  if (!chain_ids_.contains(coin)) {
    std::move(callback).Run(nullptr);
  } else {
    std::move(callback).Run(GetChain(prefs_, chain_ids_[coin], coin));
  }
}

void JsonRpcService::MaybeUpdateIsEip1559(const std::string& chain_id) {
  // Only try to update is_eip1559 for localhost or custom chains.
  auto chain = GetKnownEthChain(prefs_, chain_id);
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

  bool changed = false;
  if (chain_id == brave_wallet::mojom::kLocalhostChainId) {
    changed = prefs_->GetBoolean(kSupportEip1559OnLocalhostChain) != is_eip1559;
    prefs_->SetBoolean(kSupportEip1559OnLocalhostChain, is_eip1559);
  } else {
    DictionaryPrefUpdate update(prefs_, kBraveWalletCustomNetworks);
    for (base::Value& custom_network :
         update.Get()->FindKey(kEthereumPrefKey)->GetList()) {
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

void JsonRpcService::FireNetworkChanged(mojom::CoinType coin) {
  for (const auto& observer : observers_) {
    observer->ChainChangedEvent(GetChainId(coin), coin);
  }
}

std::string JsonRpcService::GetChainId(mojom::CoinType coin) const {
  return chain_ids_.contains(coin) ? chain_ids_.at(coin) : std::string();
}

void JsonRpcService::GetChainId(
    mojom::CoinType coin,
    mojom::JsonRpcService::GetChainIdCallback callback) {
  std::move(callback).Run(GetChainId(coin));
}

void JsonRpcService::GetBlockTrackerUrl(
    mojom::JsonRpcService::GetBlockTrackerUrlCallback callback) {
  std::move(callback).Run(
      GetBlockTrackerUrlFromNetwork(GetChainId(mojom::CoinType::ETH)).spec());
}

void JsonRpcService::GetAllNetworks(mojom::CoinType coin,
                                    GetAllNetworksCallback callback) {
  std::vector<mojom::NetworkInfoPtr> all_chains;
  GetAllChains(prefs_, coin, &all_chains);
  std::move(callback).Run(std::move(all_chains));
}

std::string JsonRpcService::GetNetworkUrl(mojom::CoinType coin) const {
  return network_urls_.contains(coin) ? network_urls_.at(coin).spec()
                                      : std::string();
}

void JsonRpcService::GetNetworkUrl(
    mojom::CoinType coin,
    mojom::JsonRpcService::GetNetworkUrlCallback callback) {
  std::move(callback).Run(GetNetworkUrl(coin));
}

void JsonRpcService::SetCustomNetworkForTesting(const std::string& chain_id,
                                                mojom::CoinType coin,
                                                const GURL& network_url) {
  chain_ids_[coin] = chain_id;
  network_urls_[coin] = network_url;
  FireNetworkChanged(coin);
}

void JsonRpcService::GetBlockNumber(GetBlockNumberCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetBlockNumber,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_blockNumber(), true,
                  network_urls_[mojom::CoinType::ETH],
                  std::move(internal_callback));
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

void JsonRpcService::GetFeeHistory(GetFeeHistoryCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetFeeHistory,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  RequestInternal(
      eth::eth_feeHistory(40, "latest", std::vector<double>{20, 50, 80}), true,
      network_urls_[mojom::CoinType::ETH], std::move(internal_callback));
}

void JsonRpcService::OnGetFeeHistory(
    GetFeeHistoryCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        std::vector<std::string>(), std::vector<double>(), "",
        std::vector<std::vector<std::string>>(),
        mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::vector<std::string> base_fee_per_gas;
  std::vector<double> gas_used_ratio;
  std::string oldest_block;
  std::vector<std::vector<std::string>> reward;
  if (!eth::ParseEthGetFeeHistory(body, &base_fee_per_gas, &gas_used_ratio,
                                  &oldest_block, &reward)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult(body, &error, &error_message);
    std::move(callback).Run(std::vector<std::string>(), std::vector<double>(),
                            "", std::vector<std::vector<std::string>>(), error,
                            error_message);
    return;
  }

  std::move(callback).Run(base_fee_per_gas, gas_used_ratio, oldest_block,
                          reward, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetBalance(const std::string& address,
                                mojom::CoinType coin,
                                const std::string& chain_id,
                                JsonRpcService::GetBalanceCallback callback) {
  auto network_url = GetNetworkURL(prefs_, chain_id, coin);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  if (coin == mojom::CoinType::ETH) {
    auto internal_callback =
        base::BindOnce(&JsonRpcService::OnEthGetBalance,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));
    RequestInternal(eth::eth_getBalance(address, "latest"), true, network_url,
                    std::move(internal_callback));
    return;
  } else if (coin == mojom::CoinType::FIL) {
    auto internal_callback =
        base::BindOnce(&JsonRpcService::OnFilGetBalance,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));
    // TODO(spyloggsster): Make sure network url is available when known
    // Filcoin networks are added.
    RequestInternal(fil_getBalance(address), true,
                    network_urls_[mojom::CoinType::FIL],
                    std::move(internal_callback));
    return;
  }
  std::move(callback).Run("", mojom::ProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
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
  RequestInternal(eth::eth_getTransactionCount(address, "latest"), true,
                  network_urls_[mojom::CoinType::ETH],
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
  RequestInternal(eth::eth_getTransactionReceipt(tx_hash), true,
                  network_urls_[mojom::CoinType::ETH],
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
  RequestInternal(eth::eth_sendRawTransaction(signed_tx), true,
                  network_urls_[mojom::CoinType::ETH],
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
    const std::string& chain_id,
    JsonRpcService::GetERC20TokenBalanceCallback callback) {
  std::string data;
  auto network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH);
  if (!erc20::BalanceOf(address, &data) || !network_url.is_valid()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetERC20TokenBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_call("", contract, "", "", "", data, "latest"), true,
                  network_url, std::move(internal_callback));
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
  RequestInternal(
      eth::eth_call("", contract_address, "", "", "", data, "latest"), true,
      network_urls_[mojom::CoinType::ETH], std::move(internal_callback));
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

  GURL network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH);
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

  GURL network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH);
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
  EnsRegistryGetResolver(chain_ids_[mojom::CoinType::ETH], domain,
                         std::move(internal_callback));
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
  RequestInternal(
      eth::eth_call("", resolver_address, "", "", "", data, "latest"), true,
      network_urls_[mojom::CoinType::ETH], std::move(internal_callback));
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

  GURL network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH);
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
      GetUnstoppableDomainsProxyReaderContractAddress(
          chain_ids_[mojom::CoinType::ETH]);
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
  RequestInternal(
      eth::eth_call("", contract_address, "", "", "", data, "latest"), true,
      network_urls_[mojom::CoinType::ETH], std::move(internal_callback));
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
  std::vector<mojom::NetworkInfoPtr> networks;
  brave_wallet::GetAllChains(prefs_, mojom::CoinType::ETH, &networks);
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
  RequestInternal(eth::eth_estimateGas(from_address, to_address, gas, gas_price,
                                       value, data),
                  true, network_urls_[mojom::CoinType::ETH],
                  std::move(internal_callback));
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
  RequestInternal(eth::eth_gasPrice(), true,
                  network_urls_[mojom::CoinType::ETH],
                  std::move(internal_callback));
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
  RequestInternal(eth::eth_getBlockByNumber("latest", false), true,
                  network_urls_[mojom::CoinType::ETH],
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
  RequestInternal(eth::eth_call("", contract, "", "", "", data, "latest"), true,
                  network_urls_[mojom::CoinType::ETH],
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
    const std::string& chain_id,
    GetERC721TokenBalanceCallback callback) {
  const auto eth_account_address = EthAddress::FromHex(account_address);
  auto network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH);
  if (eth_account_address.IsEmpty() || !network_url.is_valid()) {
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
  RequestInternal(
      eth::eth_call("", contract_address, "", "", "", data, "latest"), true,
      network_urls_[mojom::CoinType::ETH], std::move(internal_callback));
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
      !switch_chain_callbacks_.contains(origin) ||
      !switch_chain_ids_.contains(origin)) {
    return;
  }
  if (approved) {
    // We already check chain id validiy in
    // JsonRpcService::AddSwitchEthereumChainRequest so this should always
    // be successful unless chain id differs or we add more check other than
    // chain id
    CHECK(SetNetwork(switch_chain_requests_[origin], mojom::CoinType::ETH));
  }
  auto callback = std::move(switch_chain_callbacks_[origin]);
  base::Value id = std::move(switch_chain_ids_[origin]);
  switch_chain_requests_.erase(origin);
  switch_chain_callbacks_.erase(origin);
  switch_chain_ids_.erase(origin);

  bool reject = false;
  std::unique_ptr<base::Value> formed_response;
  if (approved) {
    reject = false;
    formed_response = base::Value::ToUniquePtrValue(base::Value());
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
  } else {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
  }
}

bool JsonRpcService::AddSwitchEthereumChainRequest(const std::string& chain_id,
                                                   const GURL& origin,
                                                   RequestCallback callback,
                                                   base::Value id) {
  bool reject = false;
  std::unique_ptr<base::Value> formed_response;
  if (!GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH).is_valid()) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUnknownChain,
        l10n_util::GetStringFUTF8(IDS_WALLET_UNKNOWN_CHAIN,
                                  base::ASCIIToUTF16(chain_id)));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return false;
  }

  // Already on the chain
  if (GetChainId(mojom::CoinType::ETH) == chain_id) {
    formed_response = base::Value::ToUniquePtrValue(base::Value());
    reject = false;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return false;
  }

  // There can be only 1 request per origin
  if (switch_chain_requests_.contains(origin)) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return false;
  }
  switch_chain_requests_[origin] = chain_id;
  switch_chain_callbacks_[origin] = std::move(callback);
  switch_chain_ids_[origin] = std::move(id);
  return true;
}

void JsonRpcService::Reset() {
  ClearJsonRpcServiceProfilePrefs(prefs_);
  SetNetwork(GetCurrentChainId(prefs_, mojom::CoinType::ETH),
             mojom::CoinType::ETH);

  add_chain_pending_requests_.clear();
  add_chain_pending_requests_origins_.clear();
  switch_chain_requests_.clear();
  // Reject pending suggest token requests when network changed.
  for (auto& callback : switch_chain_callbacks_) {
    std::unique_ptr<base::Value> formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    bool reject = true;
    std::move(callback.second)
        .Run(std::move(switch_chain_ids_[callback.first]),
             std::move(*formed_response), reject, "", false);
  }
  switch_chain_callbacks_.clear();
  switch_chain_ids_.clear();
}

void JsonRpcService::GetSolanaBalance(const std::string& pubkey,
                                      GetSolanaBalanceCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::getBalance(pubkey), true,
                  network_urls_[mojom::CoinType::SOL],
                  std::move(internal_callback));
}

void JsonRpcService::GetSPLTokenAccountBalance(
    const std::string& pubkey,
    GetSPLTokenAccountBalanceCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSPLTokenAccountBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::getTokenAccountBalance(pubkey), true,
                  network_urls_[mojom::CoinType::SOL],
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

void JsonRpcService::SendSolanaTransaction(
    const std::string& signed_tx,
    SendSolanaTransactionCallback callback) {
  if (signed_tx.empty()) {
    std::move(callback).Run(
        "", mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnSendSolanaTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::sendTransaction(signed_tx), true,
                  network_urls_[mojom::CoinType::SOL],
                  std::move(internal_callback));
}

void JsonRpcService::OnSendSolanaTransaction(
    SendSolanaTransactionCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string transaction_id;
  if (!solana::ParseSendTransaction(body, &transaction_id)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(transaction_id, mojom::SolanaProviderError::kSuccess,
                          "");
}

void JsonRpcService::GetSolanaLatestBlockhash(
    GetSolanaLatestBlockhashCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaLatestBlockhash,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::getLatestBlockhash(), true,
                  network_urls_[mojom::CoinType::SOL],
                  std::move(internal_callback));
}

void JsonRpcService::OnGetSolanaLatestBlockhash(
    GetSolanaLatestBlockhashCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        "", mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string blockhash;
  if (!solana::ParseGetLatestBlockhash(body, &blockhash)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(body, &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(blockhash, mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::GetSolanaSignatureStatuses(
    const std::vector<std::string>& tx_signatures,
    GetSolanaSignatureStatusesCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaSignatureStatuses,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::getSignatureStatuses(tx_signatures), true,
                  network_urls_[mojom::CoinType::SOL],
                  std::move(internal_callback));
}

void JsonRpcService::OnGetSolanaSignatureStatuses(
    GetSolanaSignatureStatusesCallback callback,
    const int status,
    const std::string& body,
    const base::flat_map<std::string, std::string>& headers) {
  if (status < 200 || status > 299) {
    std::move(callback).Run(
        std::vector<absl::optional<SolanaSignatureStatus>>(),
        mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::vector<absl::optional<SolanaSignatureStatus>> statuses;
  if (!solana::ParseGetSignatureStatuses(body, &statuses)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(body, &error, &error_message);
    std::move(callback).Run(
        std::vector<absl::optional<SolanaSignatureStatus>>(), error,
        error_message);
    return;
  }

  std::move(callback).Run(statuses, mojom::SolanaProviderError::kSuccess, "");
}

}  // namespace brave_wallet
