/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_service.h"

#include <memory>
#include <unordered_set>
#include <utility>

#include "base/base64.h"
#include "base/bind.h"
#include "base/feature_list.h"
#include "base/json/json_writer.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/ens_resolver_task.h"
#include "brave/components/brave_wallet/browser/eth_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_requests.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/browser/eth_topics_builder.h"
#include "brave/components/brave_wallet/browser/fil_requests.h"
#include "brave/components/brave_wallet/browser/fil_response_parser.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/brave_wallet/browser/solana_requests.h"
#include "brave/components/brave_wallet/browser/solana_response_parser.h"
#include "brave/components/brave_wallet/browser/unstoppable_domains_dns_resolve.h"
#include "brave/components/brave_wallet/browser/unstoppable_domains_multichain_calls.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/eth_request_helper.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/decentralized_dns/core/constants.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/ipfs_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

using api_request_helper::APIRequestHelper;
using decentralized_dns::EnsOffchainResolveMethod;

namespace {

// The domain name should be a-z | A-Z | 0-9 and hyphen(-).
// The domain name should not start or end with hyphen (-).
// The domain name can be a subdomain.
// TLD & TLD-1 must be at least two characters.
constexpr char kDomainPattern[] =
    "(?:[A-Za-z0-9][A-Za-z0-9-]*[A-Za-z0-9]\\.)+[A-Za-z]{2,}$";

// Non empty group of symbols of a-z | 0-9 | hyphen(-).
// Then a dot.
// Then one of fixed suffixes(should match `supportedUDExtensions` array from
// send.ts).
constexpr char kUDPattern[] =
    "(?:[a-z0-9-]+)\\.(?:crypto|x|coin|nft|dao|wallet|blockchain|bitcoin|zil)";

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

net::NetworkTrafficAnnotationTag GetENSOffchainNetworkTrafficAnnotationTag() {
  return net::DefineNetworkTrafficAnnotation("json_rpc_service", R"(
      semantics {
        sender: "JSON RPC Service""
        description:
          "Fetches ENS offchain data."
        trigger:
          "Triggered by ENS offchain lookup."
        data:
          "Offchain lookup info."
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
        setting:
          "You can enable or disable ENS on brave://settings/extensions page."
        policy_exception_justification:
          "Not implemented."
      }
    )");
}

bool EnsL2FeatureEnabled() {
  return base::FeatureList::IsEnabled(
      brave_wallet::features::kBraveWalletENSL2Feature);
}

bool EnsOffchainPrefEnabled(PrefService* local_state_prefs) {
  return decentralized_dns::GetEnsOffchainResolveMethod(local_state_prefs) ==
         EnsOffchainResolveMethod::kEnabled;
}

bool EnsOffchainPrefDisabled(PrefService* local_state_prefs) {
  return decentralized_dns::GetEnsOffchainResolveMethod(local_state_prefs) ==
         EnsOffchainResolveMethod::kDisabled;
}

void SetEnsOffchainPref(PrefService* local_state_prefs, bool enabled) {
  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs, enabled ? EnsOffchainResolveMethod::kEnabled
                                 : EnsOffchainResolveMethod::kDisabled);
}

namespace solana {
// https://github.com/solana-labs/solana/blob/f7b2951c79cd07685ed62717e78ab1c200924924/rpc/src/rpc.rs#L1717
constexpr char kAccountNotCreatedError[] = "could not find account";
}  // namespace solana

}  // namespace

namespace brave_wallet {

JsonRpcService::JsonRpcService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs,
    PrefService* local_state_prefs)
    : api_request_helper_(new APIRequestHelper(GetNetworkTrafficAnnotationTag(),
                                               url_loader_factory)),
      ud_get_eth_addr_calls_(
          std::make_unique<
              unstoppable_domains::MultichainCalls<std::string>>()),
      ud_resolve_dns_calls_(
          std::make_unique<unstoppable_domains::MultichainCalls<GURL>>()),
      prefs_(prefs),
      local_state_prefs_(local_state_prefs),
      weak_ptr_factory_(this) {
  if (!SetNetwork(GetCurrentChainId(prefs_, mojom::CoinType::ETH),
                  mojom::CoinType::ETH)) {
    LOG(ERROR) << "Could not set network from JsonRpcService() for ETH";
  }
  if (!SetNetwork(GetCurrentChainId(prefs_, mojom::CoinType::SOL),
                  mojom::CoinType::SOL)) {
    LOG(ERROR) << "Could not set network from JsonRpcService() for SOL";
  }
  if (!SetNetwork(GetCurrentChainId(prefs_, mojom::CoinType::FIL),
                  mojom::CoinType::FIL)) {
    LOG(ERROR) << "Could not set network from JsonRpcService() for FIL";
  }

  if (EnsL2FeatureEnabled()) {
    api_request_helper_ens_offchain_ = std::make_unique<APIRequestHelper>(
        GetENSOffchainNetworkTrafficAnnotationTag(), url_loader_factory);
  }
}

JsonRpcService::JsonRpcService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* prefs)
    : JsonRpcService(std::move(url_loader_factory), std::move(prefs), nullptr) {
}

void JsonRpcService::SetAPIRequestHelperForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_request_helper_ = std::make_unique<APIRequestHelper>(
      GetNetworkTrafficAnnotationTag(), url_loader_factory);
  if (EnsL2FeatureEnabled()) {
    api_request_helper_ens_offchain_ = std::make_unique<APIRequestHelper>(
        GetENSOffchainNetworkTrafficAnnotationTag(), url_loader_factory);
  }
}

JsonRpcService::~JsonRpcService() = default;

// static
void JsonRpcService::MigrateMultichainNetworks(PrefService* prefs) {
  // custom networks
  if (prefs->HasPrefPath(kBraveWalletCustomNetworksDeprecated)) {
    const auto& custom_networks =
        prefs->GetList(kBraveWalletCustomNetworksDeprecated);

    base::Value::Dict new_custom_networks;
    new_custom_networks.Set(kEthereumPrefKey, custom_networks.Clone());

    prefs->SetDict(kBraveWalletCustomNetworks, std::move(new_custom_networks));

    prefs->ClearPref(kBraveWalletCustomNetworksDeprecated);
  }
  // selected networks
  if (prefs->HasPrefPath(kBraveWalletCurrentChainId)) {
    const std::string chain_id = prefs->GetString(kBraveWalletCurrentChainId);
    DictionaryPrefUpdate update(prefs, kBraveWalletSelectedNetworks);
    base::Value::Dict* selected_networks = update.Get()->GetIfDict();
    if (selected_networks) {
      selected_networks->Set(kEthereumPrefKey, chain_id);
      prefs->ClearPref(kBraveWalletCurrentChainId);
    }
  }
}

// static
void JsonRpcService::MigrateDeprecatedEthereumTestnets(PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletDeprecateEthereumTestNetworksMigrated))
    return;

  if (prefs->HasPrefPath(kBraveWalletSelectedNetworks)) {
    DictionaryPrefUpdate update(prefs, kBraveWalletSelectedNetworks);
    auto& selected_networks_pref = update.Get()->GetDict();
    const std::string* selected_eth_network =
        selected_networks_pref.FindString(kEthereumPrefKey);
    if (!selected_eth_network) {
      return;
    }
    if ((*selected_eth_network == "0x3") || (*selected_eth_network == "0x4") ||
        (*selected_eth_network == "0x2a")) {
      selected_networks_pref.Set(kEthereumPrefKey, mojom::kMainnetChainId);
    }
  }

  prefs->SetBoolean(kBraveWalletDeprecateEthereumTestNetworksMigrated, true);
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

void JsonRpcService::RequestInternal(
    const std::string& json_payload,
    bool auto_retry_on_network_change,
    const GURL& network_url,
    RequestIntermediateCallback callback,
    APIRequestHelper::ResponseConversionCallback conversion_callback =
        base::NullCallback()) {
  DCHECK(network_url.is_valid());

  api_request_helper_->Request("POST", network_url, json_payload,
                               "application/json", auto_retry_on_network_change,
                               std::move(callback),
                               MakeCommonJsonRpcHeaders(json_payload), -1u,
                               std::move(conversion_callback));
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

void JsonRpcService::OnRequestResult(RequestCallback callback,
                                     base::Value id,
                                     APIRequestResult api_request_result) {
  bool reject;
  base::Value formed_response = GetProviderRequestReturnFromEthJsonResponse(
      api_request_result.response_code(), api_request_result.body(), &reject);
  std::move(callback).Run(std::move(id), std::move(formed_response), reject, "",
                          false);
}

void JsonRpcService::FirePendingRequestCompleted(const std::string& chain_id,
                                                 const std::string& error) {
  for (const auto& observer : observers_) {
    observer->OnAddEthereumChainRequestCompleted(chain_id, error);
  }
}

bool JsonRpcService::HasRequestFromOrigin(const url::Origin& origin) const {
  for (const auto& request : add_chain_pending_requests_) {
    if (request.second->origin_info->origin == origin)
      return true;
  }
  return false;
}

void JsonRpcService::GetPendingAddChainRequests(
    GetPendingAddChainRequestsCallback callback) {
  std::vector<mojom::AddChainRequestPtr> all_requests;
  for (const auto& request : add_chain_pending_requests_) {
    all_requests.push_back(request.second.Clone());
  }
  std::move(callback).Run(std::move(all_requests));
}

void JsonRpcService::AddChain(mojom::NetworkInfoPtr chain,
                              AddChainCallback callback) {
  auto chain_id = chain->chain_id;
  GURL url = MaybeAddInfuraProjectId(GetActiveEndpointUrl(*chain));

  if (!url.is_valid()) {
    std::move(callback).Run(
        chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                  base::ASCIIToUTF16(url.spec())));
    return;
  }

  if (CustomChainExists(prefs_, chain_id, chain->coin)) {
    std::move(callback).Run(
        chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_SETTINGS_WALLET_NETWORKS_EXISTS));
    return;
  }

  // Custom networks for FIL and SOL are allowed to replace only known chain
  // ids. So just update prefs without chain id validation.
  if (chain->coin == mojom::CoinType::FIL ||
      chain->coin == mojom::CoinType::SOL) {
    if (!KnownChainExists(chain_id, chain->coin)) {
      std::move(callback).Run(
          chain_id, mojom::ProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      return;
    }
    AddCustomNetwork(prefs_, *chain);
    std::move(callback).Run(chain->chain_id, mojom::ProviderError::kSuccess,
                            "");
    return;
  }

  auto result = base::BindOnce(&JsonRpcService::OnEthChainIdValidated,
                               weak_ptr_factory_.GetWeakPtr(), std::move(chain),
                               url, std::move(callback));
  RequestInternal(eth::eth_chainId(), true, url, std::move(result));
}

void JsonRpcService::OnEthChainIdValidated(
    mojom::NetworkInfoPtr chain,
    const GURL& rpc_url,
    AddChainCallback callback,
    APIRequestResult api_request_result) {
  if (ParseSingleStringResult(api_request_result.body()) != chain->chain_id) {
    std::move(callback).Run(
        chain->chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                  base::ASCIIToUTF16(rpc_url.spec())));
    return;
  }

  auto chain_id = chain->chain_id;
  AddCustomNetwork(prefs_, *chain);
  std::move(callback).Run(chain_id, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::AddEthereumChainForOrigin(
    mojom::NetworkInfoPtr chain,
    const url::Origin& origin,
    AddEthereumChainForOriginCallback callback) {
  auto chain_id = chain->chain_id;
  if (KnownChainExists(chain_id, mojom::CoinType::ETH) ||
      CustomChainExists(prefs_, chain_id, mojom::CoinType::ETH)) {
    std::move(callback).Run(
        chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_SETTINGS_WALLET_NETWORKS_EXISTS));
    return;
  }
  if (origin.opaque() || add_chain_pending_requests_.contains(chain_id) ||
      HasRequestFromOrigin(origin)) {
    std::move(callback).Run(
        chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    return;
  }

  add_chain_pending_requests_[chain_id] =
      mojom::AddChainRequest::New(MakeOriginInfo(origin), std::move(chain));
  std::move(callback).Run(chain_id, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::AddEthereumChainRequestCompleted(
    const std::string& chain_id,
    bool approved) {
  if (!add_chain_pending_requests_.contains(chain_id))
    return;

  if (!approved) {
    FirePendingRequestCompleted(
        chain_id, l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    add_chain_pending_requests_.erase(chain_id);
    return;
  }

  const auto& chain = *add_chain_pending_requests_.at(chain_id)->network_info;
  GURL url = MaybeAddInfuraProjectId(GetActiveEndpointUrl(chain));
  if (!url.is_valid()) {
    FirePendingRequestCompleted(
        chain_id,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                  base::ASCIIToUTF16(url.spec())));
    add_chain_pending_requests_.erase(chain_id);
    return;
  }

  auto result = base::BindOnce(&JsonRpcService::OnEthChainIdValidatedForOrigin,
                               weak_ptr_factory_.GetWeakPtr(), chain_id, url);
  RequestInternal(eth::eth_chainId(), true, url, std::move(result));
}

void JsonRpcService::OnEthChainIdValidatedForOrigin(
    const std::string& chain_id,
    const GURL& rpc_url,
    APIRequestResult api_request_result) {
  if (!add_chain_pending_requests_.contains(chain_id))
    return;

  const auto& chain = *add_chain_pending_requests_.at(chain_id)->network_info;
  if (ParseSingleStringResult(api_request_result.body()) != chain_id) {
    FirePendingRequestCompleted(
        chain_id,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                  base::ASCIIToUTF16(rpc_url.spec())));
    add_chain_pending_requests_.erase(chain_id);
    return;
  }

  AddCustomNetwork(prefs_, chain);
  FirePendingRequestCompleted(chain_id, "");
  add_chain_pending_requests_.erase(chain_id);
}

void JsonRpcService::RemoveChain(const std::string& chain_id,
                                 mojom::CoinType coin,
                                 RemoveChainCallback callback) {
  RemoveCustomNetwork(prefs_, chain_id, coin);
  std::move(callback).Run(true);
}

bool JsonRpcService::SetNetwork(const std::string& chain_id,
                                mojom::CoinType coin,
                                bool silent) {
  auto network_url = GetNetworkURL(prefs_, chain_id, coin);
  if (!network_url.is_valid()) {
    return false;
  }

  chain_ids_[coin] = chain_id;
  network_urls_[coin] = network_url;
  DictionaryPrefUpdate update(prefs_, kBraveWalletSelectedNetworks);
  base::Value* dict = update.Get();
  DCHECK(dict);
  dict->SetStringKey(GetPrefKeyForCoinType(coin), chain_id);

  if (!silent) {
    FireNetworkChanged(coin);
  }
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
  if (chain_id != brave_wallet::mojom::kLocalhostChainId &&
      !CustomChainExists(prefs_, chain_id, mojom::CoinType::ETH)) {
    return;
  }

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
    // TODO(apaymyshev): move all work with kBraveWalletCustomNetworks into one
    // file.
    DictionaryPrefUpdate update(prefs_, kBraveWalletCustomNetworks);
    for (base::Value& item :
         *update.Get()->GetDict().FindList(kEthereumPrefKey)) {
      base::Value::Dict* custom_network = item.GetIfDict();
      if (!custom_network)
        continue;

      const std::string* id = custom_network->FindString("chainId");
      if (!id || *id != chain_id)
        continue;

      changed =
          custom_network->FindBool("is_eip1559").value_or(false) != is_eip1559;
      custom_network->Set("is_eip1559", is_eip1559);
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
  std::move(callback).Run(GetAllChains(prefs_, coin));
}

void JsonRpcService::GetCustomNetworks(mojom::CoinType coin,
                                       GetCustomNetworksCallback callback) {
  std::vector<std::string> chain_ids;
  for (const auto& it : brave_wallet::GetAllCustomChains(prefs_, coin)) {
    chain_ids.push_back(it->chain_id);
  }
  std::move(callback).Run(std::move(chain_ids));
}

void JsonRpcService::GetKnownNetworks(mojom::CoinType coin,
                                      GetKnownNetworksCallback callback) {
  std::vector<std::string> chain_ids;
  for (const auto& it : brave_wallet::GetAllKnownChains(prefs_, coin)) {
    chain_ids.push_back(it->chain_id);
  }
  std::move(callback).Run(std::move(chain_ids));
}

void JsonRpcService::GetHiddenNetworks(mojom::CoinType coin,
                                       GetHiddenNetworksCallback callback) {
  std::move(callback).Run(GetAllHiddenNetworks(prefs_, coin));
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

void JsonRpcService::OnGetFilStateSearchMsgLimited(
    GetFilStateSearchMsgLimitedCallback callback,
    const std::string& cid,
    APIRequestResult api_request_result) {
  int64_t exit_code = -1;
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        exit_code, mojom::FilecoinProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  if (!ParseFilStateSearchMsgLimited(api_request_result.body(), cid,
                                     &exit_code)) {
    mojom::FilecoinProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::FilecoinProviderError>(api_request_result.body(),
                                                   &error, &error_message);
    std::move(callback).Run(exit_code, error, error_message);
    return;
  }

  std::move(callback).Run(exit_code, mojom::FilecoinProviderError::kSuccess,
                          "");
}

void JsonRpcService::OnGetBlockNumber(GetBlockNumberCallback callback,
                                      APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        0, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  uint256_t block_number;
  if (!eth::ParseEthGetBlockNumber(api_request_result.body(), &block_number)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
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
      eth::eth_feeHistory("0x28",  // blockCount = 40
                          "latest", std::vector<double>{20, 50, 80}),
      true, network_urls_[mojom::CoinType::ETH], std::move(internal_callback));
}

void JsonRpcService::OnGetFeeHistory(GetFeeHistoryCallback callback,
                                     APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
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
  if (!eth::ParseEthGetFeeHistory(api_request_result.body(), &base_fee_per_gas,
                                  &gas_used_ratio, &oldest_block, &reward)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult(api_request_result.body(), &error, &error_message);
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
    RequestInternal(fil::getBalance(address), true, network_url,
                    std::move(internal_callback));
    return;
  }
  std::move(callback).Run("", mojom::ProviderError::kInternalError,
                          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
}

void JsonRpcService::OnEthGetBalance(GetBalanceCallback callback,
                                     APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::string balance;
  if (!eth::ParseEthGetBalance(api_request_result.body(), &balance)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(balance, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::OnFilGetBalance(GetBalanceCallback callback,
                                     APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::string balance;
  if (!ParseFilGetBalance(api_request_result.body(), &balance)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(balance, mojom::ProviderError::kSuccess, "");
}
void JsonRpcService::GetFilStateSearchMsgLimited(
    const std::string& cid,
    uint64_t period,
    GetFilStateSearchMsgLimitedCallback callback) {
  auto network_url = network_urls_[mojom::CoinType::FIL];
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        0, mojom::FilecoinProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetFilStateSearchMsgLimited,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback), cid);

  RequestInternal(
      fil::getStateSearchMsgLimited(cid, period), true, network_url,
      std::move(internal_callback),
      base::BindOnce(&ConvertInt64ToString, "/result/Receipt/ExitCode"));
}

void JsonRpcService::GetFilBlockHeight(GetFilBlockHeightCallback callback) {
  auto network_url = network_urls_[mojom::CoinType::FIL];
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        0, mojom::FilecoinProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetFilBlockHeight,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  RequestInternal(fil::getChainHead(), true, network_url,
                  std::move(internal_callback),
                  base::BindOnce(&ConvertUint64ToString, "/result/Height"));
}

void JsonRpcService::OnGetFilBlockHeight(GetFilBlockHeightCallback callback,
                                         APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        0, mojom::FilecoinProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  uint64_t height = 0;
  if (!ParseFilGetChainHead(api_request_result.body(), &height)) {
    mojom::FilecoinProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::FilecoinProviderError>(api_request_result.body(),
                                                   &error, &error_message);
    std::move(callback).Run(height, error, error_message);
    return;
  }

  std::move(callback).Run(height, mojom::FilecoinProviderError::kSuccess, "");
}

void JsonRpcService::GetFilTransactionCount(const std::string& address,
                                            GetFilTxCountCallback callback) {
  auto network_url = network_urls_[mojom::CoinType::FIL];
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        0, mojom::FilecoinProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnFilGetTransactionCount,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  RequestInternal(fil::getTransactionCount(address), true, network_url,
                  std::move(internal_callback),
                  base::BindOnce(&ConvertUint64ToString, "/result"));
}

void JsonRpcService::GetEthTransactionCount(const std::string& address,
                                            GetTxCountCallback callback) {
  auto network_url = network_urls_[mojom::CoinType::ETH];
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        0, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnEthGetTransactionCount,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  RequestInternal(eth::eth_getTransactionCount(address, "latest"), true,
                  network_url, std::move(internal_callback));
}

void JsonRpcService::OnFilGetTransactionCount(
    GetFilTxCountCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        0, mojom::FilecoinProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  uint64_t count = 0;
  if (!ParseFilGetTransactionCount(api_request_result.body(), &count)) {
    mojom::FilecoinProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::FilecoinProviderError>(api_request_result.body(),
                                                   &error, &error_message);
    std::move(callback).Run(0u, error, error_message);
    return;
  }

  std::move(callback).Run(static_cast<uint256_t>(count),
                          mojom::FilecoinProviderError::kSuccess, "");
}

void JsonRpcService::OnEthGetTransactionCount(
    GetTxCountCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        0, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  uint256_t count;
  if (!eth::ParseEthGetTransactionCount(api_request_result.body(), &count)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
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
    APIRequestResult api_request_result) {
  TransactionReceipt receipt;
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        receipt, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  if (!eth::ParseEthGetTransactionReceipt(api_request_result.body(),
                                          &receipt)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
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

void JsonRpcService::OnSendRawTransaction(SendRawTxCallback callback,
                                          APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::string tx_hash;
  if (!eth::ParseEthSendRawTransaction(api_request_result.body(), &tx_hash)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
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
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::string result;
  if (!eth::ParseEthCall(api_request_result.body(), &result)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  const auto& args = eth::DecodeEthCallResponse(result, {"uint256"});
  if (args == absl::nullopt) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::move(callback).Run(args->at(0), mojom::ProviderError::kSuccess, "");
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
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::string result;
  if (!eth::ParseEthCall(api_request_result.body(), &result)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  const auto& args = eth::DecodeEthCallResponse(result, {"uint256"});
  if (args == absl::nullopt) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::move(callback).Run(args->at(0), mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::EnsRegistryGetResolver(const std::string& domain,
                                            StringResultCallback callback) {
  const std::string contract_address =
      GetEnsRegistryContractAddress(brave_wallet::mojom::kMainnetChainId);
  if (contract_address.empty()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::string data = ens::Resolver(domain);

  GURL network_url = GetNetworkURL(prefs_, brave_wallet::mojom::kMainnetChainId,
                                   mojom::CoinType::ETH);
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
    APIRequestResult api_request_result) {
  DCHECK(callback);
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string resolver_address;
  if (!eth::ParseAddressResult(api_request_result.body(), &resolver_address) ||
      resolver_address.empty()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(resolver_address, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::EnsGetContentHash(const std::string& domain,
                                       EnsGetContentHashCallback callback) {
  if (EnsL2FeatureEnabled()) {
    if (ens_get_content_hash_tasks_.ContainsTaskForDomain(domain)) {
      ens_get_content_hash_tasks_.AddCallbackForDomain(domain,
                                                       std::move(callback));
      return;
    }

    absl::optional<bool> allow_offchain;
    if (EnsOffchainPrefEnabled(local_state_prefs_)) {
      allow_offchain = true;
    } else if (EnsOffchainPrefDisabled(local_state_prefs_)) {
      allow_offchain = false;
    }

    GURL network_url = GetNetworkURL(
        prefs_, brave_wallet::mojom::kMainnetChainId, mojom::CoinType::ETH);
    if (!network_url.is_valid()) {
      std::move(callback).Run(
          {}, false, mojom::ProviderError::kInvalidParams,
          l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
      return;
    }

    // JsonRpcService owns EnsResolverTask instance, so Unretained is safe here.
    auto done_callback = base::BindOnce(
        &JsonRpcService::OnEnsGetContentHashTaskDone, base::Unretained(this));

    ens_get_content_hash_tasks_.AddTask(
        std::make_unique<EnsResolverTask>(
            std::move(done_callback), api_request_helper_.get(),
            api_request_helper_ens_offchain_.get(), MakeContentHashCall(domain),
            domain, network_url, allow_offchain),
        std::move(callback));
    return;
  }

  auto internal_callback = base::BindOnce(
      &JsonRpcService::ContinueEnsGetContentHash,
      weak_ptr_factory_.GetWeakPtr(), domain, std::move(callback));
  EnsRegistryGetResolver(domain, std::move(internal_callback));
}

void JsonRpcService::ContinueEnsGetContentHash(
    const std::string& domain,
    EnsGetContentHashCallback callback,
    const std::string& resolver_address,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess || resolver_address.empty()) {
    std::move(callback).Run({}, false, error, error_message);
    return;
  }

  std::string data;
  if (!ens::ContentHash(domain, &data)) {
    std::move(callback).Run(
        {}, false, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  GURL network_url = GetNetworkURL(prefs_, brave_wallet::mojom::kMainnetChainId,
                                   mojom::CoinType::ETH);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        {}, false, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnEnsGetContentHash,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(
      eth::eth_call("", resolver_address, "", "", "", data, "latest"), true,
      network_url, std::move(internal_callback));
}

void JsonRpcService::OnEnsGetContentHash(EnsGetContentHashCallback callback,
                                         APIRequestResult api_request_result) {
  DCHECK(callback);
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        {}, false, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::vector<uint8_t> content_hash;
  if (!eth::ParseEnsResolverContentHash(api_request_result.body(),
                                        &content_hash) ||
      content_hash.empty()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
    std::move(callback).Run({}, false, error, error_message);
    return;
  }

  std::move(callback).Run(content_hash, false, mojom::ProviderError::kSuccess,
                          "");
}

void JsonRpcService::EnsGetEthAddr(const std::string& domain,
                                   mojom::EnsOffchainLookupOptionsPtr options,
                                   EnsGetEthAddrCallback callback) {
  if (!IsValidDomain(domain)) {
    std::move(callback).Run(
        "", false, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  if (EnsL2FeatureEnabled()) {
    if (ens_get_eth_addr_tasks_.ContainsTaskForDomain(domain)) {
      ens_get_eth_addr_tasks_.AddCallbackForDomain(domain, std::move(callback));
      return;
    }

    if (options && options->remember) {
      SetEnsOffchainPref(local_state_prefs_, options->allow);
    }

    absl::optional<bool> allow_offchain;
    if (EnsOffchainPrefEnabled(local_state_prefs_) ||
        (options && options->allow)) {
      allow_offchain = true;
    } else if (EnsOffchainPrefDisabled(local_state_prefs_) ||
               (options && !options->allow)) {
      allow_offchain = false;
    }

    GURL network_url = GetNetworkURL(
        prefs_, brave_wallet::mojom::kMainnetChainId, mojom::CoinType::ETH);
    if (!network_url.is_valid()) {
      std::move(callback).Run(
          "", false, mojom::ProviderError::kInvalidParams,
          l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
      return;
    }

    // JsonRpcService owns EnsResolverTask instance, so Unretained is safe here.
    auto done_callback = base::BindOnce(
        &JsonRpcService::OnEnsGetEthAddrTaskDone, base::Unretained(this));

    ens_get_eth_addr_tasks_.AddTask(
        std::make_unique<EnsResolverTask>(
            std::move(done_callback), api_request_helper_.get(),
            api_request_helper_ens_offchain_.get(), MakeAddrCall(domain),
            domain, network_url, allow_offchain),
        std::move(callback));
    return;
  }

  auto internal_callback = base::BindOnce(
      &JsonRpcService::ContinueEnsGetEthAddr, weak_ptr_factory_.GetWeakPtr(),
      domain, std::move(callback));
  EnsRegistryGetResolver(domain, std::move(internal_callback));
}

void JsonRpcService::OnEnsGetEthAddrTaskDone(
    EnsResolverTask* task,
    absl::optional<EnsResolverTaskResult> task_result,
    absl::optional<EnsResolverTaskError> task_error) {
  auto callbacks = ens_get_eth_addr_tasks_.TaskDone(task);
  if (callbacks.empty()) {
    return;
  }

  std::string address;
  mojom::ProviderError error =
      task_error ? task_error->error : mojom::ProviderError::kSuccess;
  std::string error_message = task_error ? task_error->error_message : "";

  if (task_result && !task_result->resolved_result.empty()) {
    EthAddress eth_address =
        eth_abi::ExtractAddress(task_result->resolved_result);
    if (eth_address.IsValid() && !eth_address.IsZeroAddress()) {
      address = eth_address.ToHex();
    } else {
      error = mojom::ProviderError::kInvalidParams;
      error_message = l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS);
    }
  }

  bool require_offchain_consent =
      (task_result ? task_result->need_to_allow_offchain : false);
  if (require_offchain_consent && EnsOffchainPrefDisabled(local_state_prefs_)) {
    require_offchain_consent = false;
    error = mojom::ProviderError::kInternalError;
    error_message = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
  }

  for (auto& cb : callbacks) {
    std::move(cb).Run(address, require_offchain_consent, error, error_message);
  }
}

void JsonRpcService::OnEnsGetContentHashTaskDone(
    EnsResolverTask* task,
    absl::optional<EnsResolverTaskResult> task_result,
    absl::optional<EnsResolverTaskError> task_error) {
  auto callbacks = ens_get_content_hash_tasks_.TaskDone(task);
  if (callbacks.empty()) {
    return;
  }

  absl::optional<std::vector<uint8_t>> content_hash;
  mojom::ProviderError error =
      task_error ? task_error->error : mojom::ProviderError::kSuccess;
  std::string error_message = task_error ? task_error->error_message : "";

  if (task_result && !task_result->resolved_result.empty()) {
    content_hash =
        eth_abi::ExtractBytesFromTuple(task_result->resolved_result, 0);
    if (!content_hash || content_hash->empty()) {
      error = mojom::ProviderError::kInvalidParams;
      error_message = l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS);
    }
  }

  bool require_offchain_consent =
      (task_result ? task_result->need_to_allow_offchain : false);
  if (require_offchain_consent && EnsOffchainPrefDisabled(local_state_prefs_)) {
    require_offchain_consent = false;
    error = mojom::ProviderError::kInvalidParams;
    error_message = l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS);
  }

  for (auto& cb : callbacks) {
    std::move(cb).Run(content_hash.value_or(std::vector<uint8_t>()),
                      require_offchain_consent, error, error_message);
  }
}

void JsonRpcService::ContinueEnsGetEthAddr(const std::string& domain,
                                           EnsGetEthAddrCallback callback,
                                           const std::string& resolver_address,
                                           mojom::ProviderError error,
                                           const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess || resolver_address.empty()) {
    std::move(callback).Run("", false, error, error_message);
    return;
  }

  std::string data;
  if (!ens::Addr(domain, &data)) {
    std::move(callback).Run(
        "", false, mojom::ProviderError::kInvalidParams,
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

void JsonRpcService::OnEnsGetEthAddr(EnsGetEthAddrCallback callback,
                                     APIRequestResult api_request_result) {
  DCHECK(callback);
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", false, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string address;
  if (!eth::ParseAddressResult(api_request_result.body(), &address) ||
      address.empty()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
    std::move(callback).Run("", false, error, error_message);
    return;
  }

  if (EthAddress::FromHex(address).IsZeroAddress()) {
    std::move(callback).Run(
        "", false, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::move(callback).Run(address, false, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::UnstoppableDomainsResolveDns(
    const std::string& domain,
    UnstoppableDomainsResolveDnsCallback callback) {
  if (ud_resolve_dns_calls_->HasCall(domain)) {
    ud_resolve_dns_calls_->AddCallback(domain, std::move(callback));
    return;
  }

  if (!IsValidUnstoppableDomain(domain)) {
    std::move(callback).Run(
        GURL(), mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto data = unstoppable_domains::GetMany(unstoppable_domains::GetRecordKeys(),
                                           domain);
  if (!data) {
    std::move(callback).Run(
        GURL(), mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  ud_resolve_dns_calls_->AddCallback(domain, std::move(callback));
  for (const auto& chain_id : ud_resolve_dns_calls_->GetChains()) {
    auto internal_callback =
        base::BindOnce(&JsonRpcService::OnUnstoppableDomainsResolveDns,
                       weak_ptr_factory_.GetWeakPtr(), domain, chain_id);
    auto eth_call = eth::eth_call(
        "", GetUnstoppableDomainsProxyReaderContractAddress(chain_id), "", "",
        "", *data, "latest");
    RequestInternal(std::move(eth_call), true,
                    GetUnstoppableDomainsRpcUrl(chain_id),
                    std::move(internal_callback));
  }
}

void JsonRpcService::OnUnstoppableDomainsResolveDns(
    const std::string& domain,
    const std::string& chain_id,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    ud_resolve_dns_calls_->SetError(
        domain, chain_id, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto values =
      eth::ParseUnstoppableDomainsProxyReaderGetMany(api_request_result.body());
  if (!values) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
    ud_resolve_dns_calls_->SetError(domain, chain_id, error, error_message);
    return;
  }

  GURL resolved_url = unstoppable_domains::ResolveUrl(*values);
  if (!resolved_url.is_valid()) {
    ud_resolve_dns_calls_->SetNoResult(domain, chain_id);
    return;
  }

  ud_resolve_dns_calls_->SetResult(domain, chain_id, std::move(resolved_url));
}

void JsonRpcService::UnstoppableDomainsGetEthAddr(
    const std::string& domain,
    UnstoppableDomainsGetEthAddrCallback callback) {
  if (ud_get_eth_addr_calls_->HasCall(domain)) {
    ud_get_eth_addr_calls_->AddCallback(domain, std::move(callback));
    return;
  }

  if (!IsValidUnstoppableDomain(domain)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto call_data = unstoppable_domains::Get(kCryptoEthAddressKey, domain);
  if (!call_data) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  ud_get_eth_addr_calls_->AddCallback(domain, std::move(callback));
  for (const auto& chain_id : ud_get_eth_addr_calls_->GetChains()) {
    auto internal_callback =
        base::BindOnce(&JsonRpcService::OnUnstoppableDomainsGetEthAddr,
                       weak_ptr_factory_.GetWeakPtr(), domain, chain_id);
    auto eth_call = eth::eth_call(
        "", GetUnstoppableDomainsProxyReaderContractAddress(chain_id), "", "",
        "", *call_data, "latest");
    RequestInternal(std::move(eth_call), true,
                    GetUnstoppableDomainsRpcUrl(chain_id),
                    std::move(internal_callback));
  }
}

void JsonRpcService::OnUnstoppableDomainsGetEthAddr(
    const std::string& domain,
    const std::string& chain_id,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    ud_get_eth_addr_calls_->SetError(
        domain, chain_id, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto address =
      eth::ParseUnstoppableDomainsProxyReaderGet(api_request_result.body());
  if (!address) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);

    ud_get_eth_addr_calls_->SetError(domain, chain_id, error, error_message);
    return;
  }

  if (address->empty()) {
    ud_get_eth_addr_calls_->SetNoResult(domain, chain_id);
    return;
  }

  ud_get_eth_addr_calls_->SetResult(domain, chain_id, address.value());
}

GURL JsonRpcService::GetBlockTrackerUrlFromNetwork(
    const std::string& chain_id) {
  if (auto network = GetChain(prefs_, chain_id, mojom::CoinType::ETH)) {
    if (network->block_explorer_urls.size())
      return GURL(network->block_explorer_urls.front());
  }
  return GURL();
}

void JsonRpcService::GetFilEstimateGas(const std::string& from_address,
                                       const std::string& to_address,
                                       const std::string& gas_premium,
                                       const std::string& gas_fee_cap,
                                       int64_t gas_limit,
                                       uint64_t nonce,
                                       const std::string& max_fee,
                                       const std::string& value,
                                       GetFilEstimateGasCallback callback) {
  if (from_address.empty() || to_address.empty()) {
    std::move(callback).Run(
        "", "", 0, mojom::FilecoinProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetFilEstimateGas,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  auto request =
      fil::getEstimateGas(from_address, to_address, gas_premium, gas_fee_cap,
                          gas_limit, nonce, max_fee, value);
  RequestInternal(request, true, network_urls_[mojom::CoinType::FIL],
                  std::move(internal_callback),
                  base::BindOnce(&ConvertInt64ToString, "/result/GasLimit"));
}

void JsonRpcService::OnGetFilEstimateGas(GetFilEstimateGasCallback callback,
                                         APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", "", 0, mojom::FilecoinProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string gas_fee_cap;
  int64_t gas_limit = 0;
  std::string gas_premium;
  if (!ParseFilEstimateGas(api_request_result.body(), &gas_premium,
                           &gas_fee_cap, &gas_limit)) {
    mojom::FilecoinProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::FilecoinProviderError>(api_request_result.body(),
                                                   &error, &error_message);
    std::move(callback).Run("", "", 0, error, error_message);
    return;
  }

  std::move(callback).Run(gas_premium, gas_fee_cap, gas_limit,
                          mojom::FilecoinProviderError::kSuccess, "");
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

void JsonRpcService::OnGetEstimateGas(GetEstimateGasCallback callback,
                                      APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string result;
  if (!eth::ParseEthEstimateGas(api_request_result.body(), &result)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
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

void JsonRpcService::OnGetGasPrice(GetGasPriceCallback callback,
                                   APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string result;
  if (!eth::ParseEthGasPrice(api_request_result.body(), &result)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
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

void JsonRpcService::OnGetIsEip1559(GetIsEip1559Callback callback,
                                    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        false, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto result = ParseResultDict(api_request_result.body());
  if (!result) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
    std::move(callback).Run(false, error, error_message);
    return;
  }

  const std::string* base_fee = result->FindString("baseFeePerGas");
  std::move(callback).Run(base_fee && !base_fee->empty(),
                          mojom::ProviderError::kSuccess, "");
}

/*static*/
bool JsonRpcService::IsValidDomain(const std::string& domain) {
  static const base::NoDestructor<re2::RE2> kDomainRegex(kDomainPattern);
  return re2::RE2::FullMatch(domain, *kDomainRegex);
}

/*static*/
bool JsonRpcService::IsValidUnstoppableDomain(const std::string& domain) {
  static const base::NoDestructor<re2::RE2> kDomainRegex(kUDPattern);
  return re2::RE2::FullMatch(domain, *kDomainRegex);
}

void JsonRpcService::GetERC721OwnerOf(const std::string& contract,
                                      const std::string& token_id,
                                      const std::string& chain_id,
                                      GetERC721OwnerOfCallback callback) {
  auto network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH);
  if (!EthAddress::IsValidAddress(contract) || !network_url.is_valid()) {
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
                  network_url, std::move(internal_callback));
}

void JsonRpcService::OnGetERC721OwnerOf(GetERC721OwnerOfCallback callback,
                                        APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string address;
  if (!eth::ParseAddressResult(api_request_result.body(), &address) ||
      address.empty()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
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
  GetERC721OwnerOf(contract_address, token_id, chain_id,
                   std::move(internal_callback));
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

void JsonRpcService::GetERC721Metadata(const std::string& contract_address,
                                       const std::string& token_id,
                                       const std::string& chain_id,
                                       GetTokenMetadataCallback callback) {
  JsonRpcService::GetTokenMetadata(contract_address, token_id, chain_id,
                                   kERC721MetadataInterfaceId,
                                   std::move(callback));
}

void JsonRpcService::GetERC1155Metadata(const std::string& contract_address,
                                        const std::string& token_id,
                                        const std::string& chain_id,
                                        GetTokenMetadataCallback callback) {
  JsonRpcService::GetTokenMetadata(contract_address, token_id, chain_id,
                                   kERC1155MetadataInterfaceId,
                                   std::move(callback));
}

void JsonRpcService::GetTokenMetadata(const std::string& contract_address,
                                      const std::string& token_id,
                                      const std::string& chain_id,
                                      const std::string& interface_id,
                                      GetTokenMetadataCallback callback) {
  auto network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  if (!EthAddress::IsValidAddress(contract_address)) {
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

  std::string function_signature;
  if (interface_id == kERC721MetadataInterfaceId) {
    if (!erc721::TokenUri(token_id_uint, &function_signature)) {
      std::move(callback).Run(
          "", mojom::ProviderError::kInvalidParams,
          l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
      return;
    }
  } else if (interface_id == kERC1155MetadataInterfaceId) {
    if (!erc1155::Uri(token_id_uint, &function_signature)) {
      std::move(callback).Run(
          "", mojom::ProviderError::kInvalidParams,
          l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
      return;
    }
  } else {
    // Unknown inteface ID
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSupportsInterfaceTokenMetadata,
                     weak_ptr_factory_.GetWeakPtr(), contract_address,
                     function_signature, network_url, std::move(callback));

  GetSupportsInterface(contract_address, interface_id, chain_id,
                       std::move(internal_callback));
}

void JsonRpcService::OnGetSupportsInterfaceTokenMetadata(
    const std::string& contract_address,
    const std::string& function_signature,
    const GURL& network_url,
    GetTokenMetadataCallback callback,
    bool is_supported,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    std::move(callback).Run("", error, error_message);
    return;
  }

  if (!is_supported) {
    std::move(callback).Run(
        "", mojom::ProviderError::kMethodNotSupported,
        l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetTokenUri,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  RequestInternal(eth::eth_call("", contract_address, "", "", "",
                                function_signature, "latest"),
                  true, network_url, std::move(internal_callback));
}

void JsonRpcService::OnGetTokenUri(GetTokenMetadataCallback callback,
                                   APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  // Parse response JSON that wraps the result
  GURL url;
  if (!eth::ParseTokenUri(api_request_result.body(), &url)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  // Obtain JSON from the URL depending on the scheme.
  // IPFS, HTTPS, and data URIs are supported.
  // IPFS and HTTPS URIs require an additional request to fetch the metadata.
  std::string metadata_json;
  std::string scheme = url.scheme();
  if (scheme != url::kDataScheme && scheme != url::kHttpsScheme &&
      scheme != ipfs::kIPFSScheme) {
    std::move(callback).Run(
        "", mojom::ProviderError::kMethodNotSupported,
        l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR));
    return;
  }

  if (scheme == url::kDataScheme) {
    if (!eth::ParseDataURIAndExtractJSON(url, &metadata_json)) {
      std::move(callback).Run(
          "", mojom::ProviderError::kParsingError,
          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
      return;
    }

    // Sanitize JSON
    data_decoder::JsonSanitizer::Sanitize(
        std::move(metadata_json),
        base::BindOnce(&JsonRpcService::OnSanitizeTokenMetadata,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }

  if (scheme == ipfs::kIPFSScheme &&
      !ipfs::TranslateIPFSURI(url, &url, ipfs::GetDefaultIPFSGateway(prefs_),
                              false)) {
    std::move(callback).Run("", mojom::ProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetTokenMetadataPayload,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  api_request_helper_->Request("GET", url, "", "", true,
                               std::move(internal_callback));
}

void JsonRpcService::OnSanitizeTokenMetadata(
    GetTokenMetadataCallback callback,
    data_decoder::JsonSanitizer::Result result) {
  if (result.error) {
    VLOG(1) << "Data URI JSON validation error:" << *result.error;
    std::move(callback).Run("", mojom::ProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  std::string metadata_json;
  if (result.value.has_value()) {
    metadata_json = result.value.value();
  }

  std::move(callback).Run(metadata_json, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::OnGetTokenMetadataPayload(
    GetTokenMetadataCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  // Invalid JSON becomes an empty string after sanitization
  if (api_request_result.body().empty()) {
    std::move(callback).Run("", mojom::ProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  std::move(callback).Run(api_request_result.body(),
                          mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetERC1155TokenBalance(
    const std::string& contract_address,
    const std::string& token_id,
    const std::string& owner_address,
    const std::string& chain_id,
    GetERC1155TokenBalanceCallback callback) {
  const auto eth_account_address = EthAddress::FromHex(owner_address);
  auto network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH);

  if (eth_account_address.IsEmpty() || !network_url.is_valid()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  if (!EthAddress::IsValidAddress(contract_address)) {
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
  if (!erc1155::BalanceOf(owner_address, token_id_uint, &data)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnEthGetBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(
      eth::eth_call("", contract_address, "", "", "", data, "latest"), true,
      network_url, std::move(internal_callback));
}

// Called by KeyringService::CreateWallet, KeyringService::RestoreWallet,
// KeyringService::AddAccount, KeyringService::ImportAccountForKeyring,
// and KeyringService::AddHardwareAccounts
void JsonRpcService::DiscoverAssets(
    const std::string& chain_id,
    mojom::CoinType coin,
    const std::vector<std::string>& account_addresses) {
  auto callback = base::BindOnce(&JsonRpcService::OnDiscoverAssetsCompleted,
                                 weak_ptr_factory_.GetWeakPtr());
  DiscoverAssetsInternal(chain_id, coin, account_addresses,
                         std::move(callback));
}

void JsonRpcService::OnDiscoverAssetsCompleted(
    const std::vector<mojom::BlockchainTokenPtr> discovered_assets,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    VLOG(1) << __func__ << "Encountered error during asset discovery "
            << error_message;
  }
}

void JsonRpcService::DiscoverAssetsInternal(
    const std::string& chain_id,
    mojom::CoinType coin,
    const std::vector<std::string>& account_addresses,
    DiscoverAssetsCallback callback) {
  if (coin != mojom::CoinType::ETH || chain_id != mojom::kMainnetChainId) {
    std::move(callback).Run(
        std::vector<mojom::BlockchainTokenPtr>(),
        mojom::ProviderError::kMethodNotSupported,
        l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR));
    return;
  }

  // Asset discovery only supported when using Infura proxy
  GURL infura_url = GetInfuraURLForKnownChainId(chain_id);
  GURL active_url = GetNetworkURL(prefs_, chain_id, coin);
  if (infura_url.host() != active_url.host()) {
    std::move(callback).Run(
        std::vector<mojom::BlockchainTokenPtr>(),
        mojom::ProviderError::kMethodNotSupported,
        l10n_util::GetStringUTF8(IDS_WALLET_METHOD_NOT_SUPPORTED_ERROR));
    return;
  }

  if (account_addresses.empty()) {
    std::move(callback).Run(
        std::vector<mojom::BlockchainTokenPtr>(),
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  for (const auto& account_address : account_addresses) {
    if (!EthAddress::IsValidAddress(account_address)) {
      std::move(callback).Run(
          std::vector<mojom::BlockchainTokenPtr>(),
          mojom::ProviderError::kInvalidParams,
          l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
      return;
    }
  }

  std::vector<mojom::BlockchainTokenPtr> user_assets =
      BraveWalletService::GetUserAssets(chain_id, mojom::CoinType::ETH, prefs_);
  auto internal_callback = base::BindOnce(
      &JsonRpcService::OnGetAllTokensDiscoverAssets,
      weak_ptr_factory_.GetWeakPtr(), chain_id, account_addresses,
      std::move(user_assets), std::move(callback));

  BlockchainRegistry::GetInstance()->GetAllTokens(
      chain_id, mojom::CoinType::ETH, std::move(internal_callback));
}

void JsonRpcService::OnGetAllTokensDiscoverAssets(
    const std::string& chain_id,
    const std::vector<std::string>& account_addresses,
    std::vector<mojom::BlockchainTokenPtr> user_assets,
    DiscoverAssetsCallback callback,
    std::vector<mojom::BlockchainTokenPtr> token_registry) {
  auto network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        std::vector<mojom::BlockchainTokenPtr>(),
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  base::Value::List topics;
  if (!MakeAssetDiscoveryTopics(account_addresses, &topics)) {
    std::move(callback).Run(
        std::vector<mojom::BlockchainTokenPtr>(),
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  // Create set of contract addresses the user already has for easy lookups
  base::flat_set<std::string> user_asset_contract_addresses;
  for (const auto& user_asset : user_assets) {
    user_asset_contract_addresses.insert(user_asset->contract_address);
  }

  // Create a list of contract addresses to search by removing
  // all erc20s and assets the user has already added.
  base::Value::List contract_addresses_to_search;
  // Also create a map for addresses to blockchain tokens for easy lookup
  // for blockchain tokens in OnGetTransferLogs
  base::flat_map<std::string, mojom::BlockchainTokenPtr> tokens_to_search;
  for (auto& registry_token : token_registry) {
    if (registry_token->is_erc20 && !registry_token->contract_address.empty() &&
        !user_asset_contract_addresses.contains(
            registry_token->contract_address)) {
      // Use lowercase representation of hex address for comparisons
      // because providers may return all lowercase addresses.
      const std::string lower_case_contract_address =
          base::ToLowerASCII(registry_token->contract_address);
      contract_addresses_to_search.Append(lower_case_contract_address);
      tokens_to_search[lower_case_contract_address] = std::move(registry_token);
    }
  }

  if (contract_addresses_to_search.size() == 0) {
    std::move(callback).Run(std::vector<mojom::BlockchainTokenPtr>(),
                            mojom::ProviderError::kSuccess, "");
    return;
  }

  auto internal_callback = base::BindOnce(
      &JsonRpcService::OnGetTransferLogs, weak_ptr_factory_.GetWeakPtr(),
      std::move(callback), base::OwnedRef(std::move(tokens_to_search)));

  RequestInternal(eth::eth_getLogs("earliest", "latest",
                                   std::move(contract_addresses_to_search),
                                   std::move(topics), ""),
                  true, network_url, std::move(internal_callback));
}

void JsonRpcService::OnGetTransferLogs(
    DiscoverAssetsCallback callback,
    base::flat_map<std::string, mojom::BlockchainTokenPtr>& tokens_to_search,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        std::vector<mojom::BlockchainTokenPtr>(),
        mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::vector<Log> logs;
  if (!eth::ParseEthGetLogs(api_request_result.body(), &logs)) {
    std::move(callback).Run(std::vector<mojom::BlockchainTokenPtr>(),
                            mojom::ProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  // Create unique list of addresses that matched eth_getLogs query
  base::flat_set<std::string> matching_contract_addresses;
  for (const auto& log : logs) {
    matching_contract_addresses.insert(base::ToLowerASCII(log.address));
  }
  std::vector<mojom::BlockchainTokenPtr> discovered_assets;

  for (const auto& contract_address : matching_contract_addresses) {
    if (!tokens_to_search.contains(contract_address)) {
      continue;
    }
    mojom::BlockchainTokenPtr token =
        std::move(tokens_to_search.at(contract_address));

    if (!BraveWalletService::AddUserAsset(token.Clone(), prefs_)) {
      continue;
    }
    discovered_assets.push_back(std::move(token));
  }

  std::move(callback).Run(std::move(discovered_assets),
                          mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetSupportsInterface(
    const std::string& contract_address,
    const std::string& interface_id,
    const std::string& chain_id,
    GetSupportsInterfaceCallback callback) {
  auto network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH);
  if (!EthAddress::IsValidAddress(contract_address) ||
      !network_url.is_valid()) {
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
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSupportsInterface,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  DCHECK(network_urls_.contains(mojom::CoinType::ETH));
  RequestInternal(
      eth::eth_call("", contract_address, "", "", "", data, "latest"), true,
      network_url, std::move(internal_callback));
}

void JsonRpcService::OnGetSupportsInterface(
    GetSupportsInterfaceCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        false, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  bool is_supported = false;
  if (!ParseBoolResult(api_request_result.body(), &is_supported)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.body(), &error,
                                           &error_message);
    std::move(callback).Run(false, error, error_message);
    return;
  }

  std::move(callback).Run(is_supported, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetPendingSwitchChainRequests(
    GetPendingSwitchChainRequestsCallback callback) {
  std::vector<mojom::SwitchChainRequestPtr> requests;
  for (const auto& request : switch_chain_requests_) {
    requests.push_back(mojom::SwitchChainRequest::New(
        MakeOriginInfo(request.first), request.second));
  }
  std::move(callback).Run(std::move(requests));
}

void JsonRpcService::NotifySwitchChainRequestProcessed(
    bool approved,
    const url::Origin& origin) {
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
  if (approved) {
    reject = false;
    std::move(callback).Run(std::move(id), base::Value(), reject, "", false);
  } else {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
  }
}

bool JsonRpcService::AddSwitchEthereumChainRequest(const std::string& chain_id,
                                                   const url::Origin& origin,
                                                   RequestCallback callback,
                                                   base::Value id) {
  bool reject = false;
  if (!GetNetworkURL(prefs_, chain_id, mojom::CoinType::ETH).is_valid()) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUnknownChain,
        l10n_util::GetStringFUTF8(IDS_WALLET_UNKNOWN_CHAIN,
                                  base::ASCIIToUTF16(chain_id)));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return false;
  }

  // Already on the chain
  if (GetChainId(mojom::CoinType::ETH) == chain_id) {
    reject = false;
    std::move(callback).Run(std::move(id), base::Value(), reject, "", false);
    return false;
  }

  // There can be only 1 request per origin
  if (switch_chain_requests_.contains(origin)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
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
  switch_chain_requests_.clear();
  // Reject pending suggest token requests when network changed.
  for (auto& callback : switch_chain_callbacks_) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    bool reject = true;
    std::move(callback.second)
        .Run(std::move(switch_chain_ids_[callback.first]),
             std::move(formed_response), reject, "", false);
  }
  switch_chain_callbacks_.clear();
  switch_chain_ids_.clear();
}

void JsonRpcService::GetSolanaBalance(const std::string& pubkey,
                                      const std::string& chain_id,
                                      GetSolanaBalanceCallback callback) {
  auto network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::SOL);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        0u, mojom::SolanaProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::getBalance(pubkey), true, network_url,
                  std::move(internal_callback),
                  base::BindOnce(&ConvertUint64ToString, "/result/value"));
}

void JsonRpcService::GetSPLTokenAccountBalance(
    const std::string& wallet_address,
    const std::string& token_mint_address,
    const std::string& chain_id,
    GetSPLTokenAccountBalanceCallback callback) {
  auto network_url = GetNetworkURL(prefs_, chain_id, mojom::CoinType::SOL);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        "", 0, "", mojom::SolanaProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  absl::optional<std::string> associated_token_account =
      SolanaKeyring::GetAssociatedTokenAccount(token_mint_address,
                                               wallet_address);
  if (!associated_token_account) {
    std::move(callback).Run(
        "", 0, "", mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSPLTokenAccountBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::getTokenAccountBalance(*associated_token_account),
                  true, network_url, std::move(internal_callback));
}

void JsonRpcService::OnGetSolanaBalance(GetSolanaBalanceCallback callback,
                                        APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        0u, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  uint64_t balance = 0;
  if (!solana::ParseGetBalance(api_request_result.body(), &balance)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(api_request_result.body(),
                                                 &error, &error_message);
    std::move(callback).Run(0u, error, error_message);
    return;
  }

  std::move(callback).Run(balance, mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::OnGetSPLTokenAccountBalance(
    GetSPLTokenAccountBalanceCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", 0u, "", mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string amount, ui_amount_string;
  uint8_t decimals = 0;
  if (!solana::ParseGetTokenAccountBalance(api_request_result.body(), &amount,
                                           &decimals, &ui_amount_string)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(api_request_result.body(),
                                                 &error, &error_message);

    // Treat balance as 0 if the associated token account is not created yet.
    if (error == mojom::SolanaProviderError::kInvalidParams &&
        error_message.find(::solana::kAccountNotCreatedError) !=
            std::string::npos) {
      std::move(callback).Run("0", 0u, "0",
                              mojom::SolanaProviderError::kSuccess, "");
      return;
    }

    std::move(callback).Run("", 0u, "", error, error_message);
    return;
  }

  std::move(callback).Run(amount, decimals, ui_amount_string,
                          mojom::SolanaProviderError::kSuccess, "");
}
void JsonRpcService::SendFilecoinTransaction(
    const std::string& signed_tx,
    SendFilecoinTransactionCallback callback) {
  if (signed_tx.empty()) {
    std::move(callback).Run(
        "", mojom::FilecoinProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  auto request = fil::getSendTransaction(signed_tx);
  if (!request) {
    std::move(callback).Run(
        "", mojom::FilecoinProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnSendFilecoinTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(request.value(), true, network_urls_[mojom::CoinType::FIL],
                  std::move(internal_callback));
}

void JsonRpcService::OnSendFilecoinTransaction(
    SendFilecoinTransactionCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::FilecoinProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string cid;
  if (!ParseSendFilecoinTransaction(api_request_result.body(), &cid)) {
    mojom::FilecoinProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::FilecoinProviderError>(api_request_result.body(),
                                                   &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(cid, mojom::FilecoinProviderError::kSuccess, "");
}

void JsonRpcService::SendSolanaTransaction(
    const std::string& signed_tx,
    absl::optional<SolanaTransaction::SendOptions> send_options,
    SendSolanaTransactionCallback callback) {
  if (signed_tx.empty()) {
    std::move(callback).Run(
        "", mojom::SolanaProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnSendSolanaTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::sendTransaction(signed_tx, std::move(send_options)),
                  true, network_urls_[mojom::CoinType::SOL],
                  std::move(internal_callback));
}

void JsonRpcService::OnSendSolanaTransaction(
    SendSolanaTransactionCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string transaction_id;
  if (!solana::ParseSendTransaction(api_request_result.body(),
                                    &transaction_id)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(api_request_result.body(),
                                                 &error, &error_message);
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
                  std::move(internal_callback),
                  base::BindOnce(&ConvertUint64ToString,
                                 "/result/value/lastValidBlockHeight"));
}

void JsonRpcService::OnGetSolanaLatestBlockhash(
    GetSolanaLatestBlockhashCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", 0, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string blockhash;
  uint64_t last_valid_block_height = 0;
  if (!solana::ParseGetLatestBlockhash(api_request_result.body(), &blockhash,
                                       &last_valid_block_height)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(api_request_result.body(),
                                                 &error, &error_message);
    std::move(callback).Run("", 0, error, error_message);
    return;
  }

  std::move(callback).Run(blockhash, last_valid_block_height,
                          mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::GetSolanaSignatureStatuses(
    const std::vector<std::string>& tx_signatures,
    GetSolanaSignatureStatusesCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaSignatureStatuses,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(
      solana::getSignatureStatuses(tx_signatures), true,
      network_urls_[mojom::CoinType::SOL], std::move(internal_callback),
      base::BindOnce(&ConvertMultiUint64InObjectArrayToString, "/result/value",
                     std::vector<std::string>({"slot", "confirmations"})));
}

void JsonRpcService::OnGetSolanaSignatureStatuses(
    GetSolanaSignatureStatusesCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        std::vector<absl::optional<SolanaSignatureStatus>>(),
        mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::vector<absl::optional<SolanaSignatureStatus>> statuses;
  if (!solana::ParseGetSignatureStatuses(api_request_result.body(),
                                         &statuses)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(api_request_result.body(),
                                                 &error, &error_message);
    std::move(callback).Run(
        std::vector<absl::optional<SolanaSignatureStatus>>(), error,
        error_message);
    return;
  }

  std::move(callback).Run(statuses, mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::GetSolanaAccountInfo(
    const std::string& pubkey,
    GetSolanaAccountInfoCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaAccountInfo,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(
      solana::getAccountInfo(pubkey), true, network_urls_[mojom::CoinType::SOL],
      std::move(internal_callback),
      base::BindOnce(&ConvertMultiUint64ToString,
                     std::vector<std::string>({"/result/value/lamports",
                                               "/result/value/rentEpoch"})));
}

void JsonRpcService::OnGetSolanaAccountInfo(
    GetSolanaAccountInfoCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        absl::nullopt, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  absl::optional<SolanaAccountInfo> account_info;
  if (!solana::ParseGetAccountInfo(api_request_result.body(), &account_info)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(api_request_result.body(),
                                                 &error, &error_message);
    std::move(callback).Run(absl::nullopt, error, error_message);
    return;
  }

  std::move(callback).Run(account_info, mojom::SolanaProviderError::kSuccess,
                          "");
}

void JsonRpcService::GetSolanaFeeForMessage(
    const std::string& message,
    GetSolanaFeeForMessageCallback callback) {
  if (message.empty() || !base::Base64Decode(message)) {
    std::move(callback).Run(
        0, mojom::SolanaProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaFeeForMessage,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::getFeeForMessage(message), true,
                  network_urls_[mojom::CoinType::SOL],
                  std::move(internal_callback),
                  base::BindOnce(&ConvertUint64ToString, "/result/value"));
}

void JsonRpcService::OnGetSolanaFeeForMessage(
    GetSolanaFeeForMessageCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        0, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  uint64_t fee;
  if (!solana::ParseGetFeeForMessage(api_request_result.body(), &fee)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(api_request_result.body(),
                                                 &error, &error_message);
    std::move(callback).Run(0, error, error_message);
    return;
  }

  std::move(callback).Run(fee, mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::GetSolanaBlockHeight(
    GetSolanaBlockHeightCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaBlockHeight,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::getBlockHeight(), true,
                  network_urls_[mojom::CoinType::SOL],
                  std::move(internal_callback),
                  base::BindOnce(&ConvertUint64ToString, "/result"));
}

void JsonRpcService::OnGetSolanaBlockHeight(
    GetSolanaBlockHeightCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        0, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  uint64_t block_height = 0;
  if (!solana::ParseGetBlockHeight(api_request_result.body(), &block_height)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(api_request_result.body(),
                                                 &error, &error_message);
    std::move(callback).Run(0, error, error_message);
    return;
  }

  std::move(callback).Run(block_height, mojom::SolanaProviderError::kSuccess,
                          "");
}

}  // namespace brave_wallet
