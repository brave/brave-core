/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/json_rpc_service.h"

#include <memory>
#include <optional>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/base64.h"
#include "base/check.h"
#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/ens_resolver_task.h"
#include "brave/components/brave_wallet/browser/eth_data_builder.h"
#include "brave/components/brave_wallet/browser/eth_requests.h"
#include "brave/components/brave_wallet/browser/eth_response_parser.h"
#include "brave/components/brave_wallet/browser/fil_requests.h"
#include "brave/components/brave_wallet/browser/fil_response_parser.h"
#include "brave/components/brave_wallet/browser/json_rpc_requests_helper.h"
#include "brave/components/brave_wallet/browser/json_rpc_response_parser.h"
#include "brave/components/brave_wallet/browser/network_manager.h"
#include "brave/components/brave_wallet/browser/nft_metadata_fetcher.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/brave_wallet/browser/solana_requests.h"
#include "brave/components/brave_wallet/browser/solana_response_parser.h"
#include "brave/components/brave_wallet/browser/unstoppable_domains_dns_resolve.h"
#include "brave/components/brave_wallet/browser/unstoppable_domains_multichain_calls.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/eth_abi_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hash_utils.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/decentralized_dns/core/constants.h"
#include "brave/components/decentralized_dns/core/utils.h"
#include "brave/components/json/json_helper.h"
#include "brave/net/base/url_util.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

using api_request_helper::APIRequestHelper;

namespace brave_wallet {

struct PendingAddChainRequest {
  mojom::AddChainRequestPtr request;
  url::Origin origin;
};

struct PendingSwitchChainRequest {
  mojom::SwitchChainRequestPtr switch_chain_request;
  url::Origin origin;
  base::Value switch_chain_id;
  mojom::EthereumProvider::RequestCallback switch_chain_callback;
};

template <typename T>
struct SolanaRPCResponse {
  std::vector<T> values;
  mojom::SolanaProviderError error;
  std::string error_message;

  SolanaRPCResponse(std::vector<T>&& values,
                    mojom::SolanaProviderError error,
                    const std::string& error_message)
      : values(std::move(values)), error(error), error_message(error_message) {}
  ~SolanaRPCResponse() = default;

  SolanaRPCResponse(SolanaRPCResponse&&) = default;
  SolanaRPCResponse& operator=(SolanaRPCResponse&&) = default;
  SolanaRPCResponse(const SolanaRPCResponse&) = delete;
  SolanaRPCResponse& operator=(const SolanaRPCResponse&) = delete;
};

template <typename T>
void MergeSolanaRPCResponses(SolanaRPCResponsesCallback<T> callback,
                             std::vector<SolanaRPCResponse<T>> responses) {
  std::vector<T> merged_responses;
  mojom::SolanaProviderError error = mojom::SolanaProviderError::kSuccess;
  std::string error_message;
  for (auto& response : responses) {
    if (response.error != mojom::SolanaProviderError::kSuccess) {
      error = response.error;
      error_message = response.error_message;
      break;
    }
    for (auto& value : response.values) {
      merged_responses.push_back(std::move(value));
    }
  }

  std::move(callback).Run(std::move(merged_responses), error, error_message);
}

namespace {

using brave_wallet::mojom::ResolveMethod;
using decentralized_dns::EnsOffchainResolveMethod;
using decentralized_dns::ResolveMethodTypes;

// The domain name should be a-z | A-Z | 0-9 and hyphen(-).
// The domain name should not start or end with hyphen (-).
// The domain name can be a subdomain.
// TLD & TLD-1 must be at least two characters.
constexpr char kEnsDomainPattern[] =
    "(?:[A-Za-z0-9][A-Za-z0-9-]*[A-Za-z0-9]\\.)+[A-Za-z]{2,}$";

// Dot separated alpha-numeric-hyphen strings ending with sol
constexpr char kSnsDomainPattern[] = R"(^(?:[a-z0-9-]+\.)+sol$)";

// Non empty group of symbols of a-z | 0-9 | hyphen(-).
// Then a dot.
// Then one of fixed suffixes(should match `supportedUDExtensions` array from
// domain-extensions.ts).
constexpr char kUDPattern[] =
    "(?:[a-z0-9-]+)\\.(?:crypto|x|nft|dao|wallet|blockchain|bitcoin|zil|"
    "altimist|anime|klever|manga|polygon|unstoppable|pudgy|tball|stepn|secret|"
    "raiin|pog|clay|metropolis|witg|ubu|kryptic|farms|dfz|kresus|binanceus|"
    "austin|bitget|wrkx)";

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

bool EnsOffchainPrefEnabled(PrefService* local_state_prefs) {
  return decentralized_dns::GetEnsOffchainResolveMethod(local_state_prefs) ==
         EnsOffchainResolveMethod::kEnabled;
}

bool EnsOffchainPrefDisabled(PrefService* local_state_prefs) {
  return decentralized_dns::GetEnsOffchainResolveMethod(local_state_prefs) ==
         EnsOffchainResolveMethod::kDisabled;
}

brave_wallet::mojom::ResolveMethod ToMojomResolveMethod(
    decentralized_dns::ResolveMethodTypes method) {
  switch (method) {
    case ResolveMethodTypes::ASK:
      return ResolveMethod::kAsk;
    case ResolveMethodTypes::DISABLED:
      return ResolveMethod::kDisabled;
    case ResolveMethodTypes::ENABLED:
      return ResolveMethod::kEnabled;
    case ResolveMethodTypes::DEPRECATED_DNS_OVER_HTTPS:
      break;
  }

  NOTREACHED_IN_MIGRATION();
  return ResolveMethod::kDisabled;
}

decentralized_dns::ResolveMethodTypes FromMojomResolveMethod(
    brave_wallet::mojom::ResolveMethod method) {
  switch (method) {
    case ResolveMethod::kAsk:
      return ResolveMethodTypes::ASK;
    case ResolveMethod::kDisabled:
      return ResolveMethodTypes::DISABLED;
    case ResolveMethod::kEnabled:
      return ResolveMethodTypes::ENABLED;
  }

  NOTREACHED_IN_MIGRATION();
  return ResolveMethodTypes::DISABLED;
}

brave_wallet::mojom::ResolveMethod ToMojomEnsOffchainResolveMethod(
    decentralized_dns::EnsOffchainResolveMethod method) {
  switch (method) {
    case EnsOffchainResolveMethod::kAsk:
      return ResolveMethod::kAsk;
    case EnsOffchainResolveMethod::kDisabled:
      return ResolveMethod::kDisabled;
    case EnsOffchainResolveMethod::kEnabled:
      return ResolveMethod::kEnabled;
  }

  NOTREACHED_IN_MIGRATION();
  return ResolveMethod::kDisabled;
}

decentralized_dns::EnsOffchainResolveMethod FromMojomEnsOffchainResolveMethod(
    brave_wallet::mojom::ResolveMethod method) {
  switch (method) {
    case ResolveMethod::kAsk:
      return EnsOffchainResolveMethod::kAsk;
    case ResolveMethod::kDisabled:
      return EnsOffchainResolveMethod::kDisabled;
    case ResolveMethod::kEnabled:
      return EnsOffchainResolveMethod::kEnabled;
  }

  NOTREACHED_IN_MIGRATION();
  return EnsOffchainResolveMethod::kDisabled;
}

// Retrieves a custom network dict from the preferences based on the chain ID.
// This function is templated to work with both base::Value::Dict as well as
// const base::Value::Dict types, for read/write and read-only access
// respectively.
template <typename T>
T* GetCustomEVMNetworkFromPrefsDict(const std::string& chain_id,
                                    T& custom_networks) {
  auto* custom_evm_networks =
      custom_networks.FindList(brave_wallet::kEthereumPrefKey);
  if (!custom_evm_networks) {
    return nullptr;
  }

  for (auto& item : *custom_evm_networks) {
    T* custom_network = item.GetIfDict();
    if (!custom_network) {
      continue;
    }

    const std::string* id = custom_network->FindString("chainId");
    if (!id || *id != chain_id) {
      continue;
    }

    return custom_network;
  }

  return nullptr;
}

// https://github.com/solana-labs/solana/blob/f7b2951c79cd07685ed62717e78ab1c200924924/rpc/src/rpc.rs#L1717
constexpr char kAccountNotCreatedError[] = "could not find account";

std::optional<std::string> GetAnkrBlockchainFromChainId(
    const std::string& chain_id) {
  auto& blockchains = GetAnkrBlockchains();
  if (blockchains.contains(chain_id)) {
    return blockchains.at(chain_id);
  }

  return std::nullopt;
}

bool ValidateNftIdentifiers(
    mojom::CoinType coin,
    std::vector<mojom::NftIdentifierPtr>& nft_identifiers,
    std::string& error_message) {
  if (nft_identifiers.empty() ||
      nft_identifiers.size() > kSimpleHashMaxBatchSize) {
    error_message = l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS);
    return false;
  }

  if (coin == mojom::CoinType::ETH) {
    for (auto& nft_identifier : nft_identifiers) {
      auto checksum_address =
          brave_wallet::EthAddress::ToEip1191ChecksumAddress(
              nft_identifier->contract_address, nft_identifier->chain_id);
      if (checksum_address) {
        nft_identifier->contract_address = checksum_address.value();
      } else {
        error_message = l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS);
        return false;
      }
    }
  }
  return true;
}

}  // namespace

JsonRpcService::JsonRpcService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    NetworkManager* network_manager,
    PrefService* prefs,
    PrefService* local_state_prefs)
    : api_request_helper_(new APIRequestHelper(GetNetworkTrafficAnnotationTag(),
                                               url_loader_factory)),
      network_manager_(network_manager),
      prefs_(prefs),
      local_state_prefs_(local_state_prefs) {
  if (!local_state_prefs_) {
    CHECK_IS_TEST();
  }

  api_request_helper_ens_offchain_ = std::make_unique<APIRequestHelper>(
      GetENSOffchainNetworkTrafficAnnotationTag(), url_loader_factory);

  nft_metadata_fetcher_ =
      std::make_unique<NftMetadataFetcher>(url_loader_factory, this, prefs_);
  simple_hash_client_ = std::make_unique<SimpleHashClient>(url_loader_factory);
}

void JsonRpcService::SetAPIRequestHelperForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  api_request_helper_ = std::make_unique<APIRequestHelper>(
      GetNetworkTrafficAnnotationTag(), url_loader_factory);
  api_request_helper_ens_offchain_ = std::make_unique<APIRequestHelper>(
      GetENSOffchainNetworkTrafficAnnotationTag(), url_loader_factory);
}

JsonRpcService::~JsonRpcService() = default;

void JsonRpcService::Bind(
    mojo::PendingReceiver<mojom::JsonRpcService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void JsonRpcService::AddObserver(
    ::mojo::PendingRemote<mojom::JsonRpcServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

GURL JsonRpcService::GetNetworkURL(const std::string& chain_id,
                                   mojom::CoinType coin) {
  return network_manager_->GetNetworkURL(chain_id, coin);
}

void JsonRpcService::RequestInternal(
    const std::string& json_payload,
    bool auto_retry_on_network_change,
    const GURL& network_url,
    RequestIntermediateCallback callback,
    APIRequestHelper::ResponseConversionCallback conversion_callback =
        base::NullCallback()) {
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        APIRequestResult(400, {}, {}, net::ERR_UNEXPECTED, GURL()));
    return;
  }

  api_request_helper_->Request(
      "POST", network_url, json_payload, "application/json",
      std::move(callback), MakeCommonJsonRpcHeaders(json_payload, network_url),
      {.auto_retry_on_network_change = auto_retry_on_network_change},
      std::move(conversion_callback));
}

void JsonRpcService::Request(const std::string& chain_id,
                             const std::string& json_payload,
                             bool auto_retry_on_network_change,
                             base::Value id,
                             mojom::CoinType coin,
                             RequestCallback callback) {
  RequestInternal(
      json_payload, auto_retry_on_network_change, GetNetworkURL(chain_id, coin),
      base::BindOnce(&JsonRpcService::OnRequestResult, base::Unretained(this),
                     std::move(callback), std::move(id)));
}

void JsonRpcService::OnRequestResult(RequestCallback callback,
                                     base::Value id,
                                     APIRequestResult api_request_result) {
  bool reject;
  base::Value formed_response = GetProviderRequestReturnFromEthJsonResponse(
      api_request_result.response_code(), api_request_result.value_body(),
      &reject);
  std::move(callback).Run(std::move(id), std::move(formed_response), reject, "",
                          false);
}

void JsonRpcService::FirePendingRequestCompleted(const std::string& chain_id,
                                                 const std::string& error) {
  for (const auto& observer : observers_) {
    observer->OnAddEthereumChainRequestCompleted(chain_id, error);
  }
}

bool JsonRpcService::HasAddChainRequestFromOrigin(
    const url::Origin& origin) const {
  return base::ranges::any_of(add_chain_pending_requests_, [origin](auto& req) {
    return req.second.origin == origin;
  });
}

bool JsonRpcService::HasSwitchChainRequestFromOrigin(
    const url::Origin& origin) const {
  return base::ranges::any_of(
      pending_switch_chain_requests_,
      [origin](auto& req) { return req.second.origin == origin; });
}

void JsonRpcService::GetPendingAddChainRequests(
    GetPendingAddChainRequestsCallback callback) {
  std::vector<mojom::AddChainRequestPtr> all_requests;
  for (const auto& request : add_chain_pending_requests_) {
    all_requests.push_back(request.second.request.Clone());
  }
  std::move(callback).Run(std::move(all_requests));
}

void JsonRpcService::AddChain(mojom::NetworkInfoPtr chain,
                              AddChainCallback callback) {
  if (!base::ranges::all_of(chain->rpc_endpoints,
                            &net::IsHTTPSOrLocalhostURL) ||
      !base::ranges::all_of(chain->block_explorer_urls,
                            &IsHTTPSOrLocalhostURL) ||
      !base::ranges::all_of(chain->icon_urls, &IsHTTPSOrLocalhostURL)) {
    std::move(callback).Run(
        chain->chain_id, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_ADD_CHAIN_INVALID_URL));
    return;
  }

  auto chain_id = chain->chain_id;
  GURL url = GetActiveEndpointUrl(*chain);

  if (!url.is_valid()) {
    std::move(callback).Run(
        chain_id, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_ADD_CHAIN_INVALID_URL));
    return;
  }

  if (network_manager_->CustomChainExists(chain_id, chain->coin)) {
    std::move(callback).Run(
        chain_id, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_SETTINGS_WALLET_NETWORKS_EXISTS));
    return;
  }

  // Custom networks for non-EVM chain are allowed to replace only known chain
  // ids. So just update prefs without chain id validation.
  if (chain->coin != mojom::CoinType::ETH) {
    if (!network_manager_->KnownChainExists(chain_id, chain->coin)) {
      std::move(callback).Run(
          chain_id, mojom::ProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      return;
    }
    AddCustomNetworkInternal(*chain);
    std::move(callback).Run(chain->chain_id, mojom::ProviderError::kSuccess,
                            "");
    return;
  }

  if (skip_eth_chain_id_validation_for_testing_) {
    OnEthChainIdValidated(std::move(chain), url, std::move(callback), {});
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
  if (ParseSingleStringResult(api_request_result.value_body()) !=
          chain->chain_id &&
      !skip_eth_chain_id_validation_for_testing_) {
    std::move(callback).Run(
        chain->chain_id, mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                  base::ASCIIToUTF16(rpc_url.spec())));
    return;
  }

  AddCustomNetworkInternal(*chain);
  std::move(callback).Run(chain->chain_id, mojom::ProviderError::kSuccess, "");
}

std::string JsonRpcService::AddEthereumChainForOrigin(
    mojom::NetworkInfoPtr chain,
    const url::Origin& origin) {
  auto chain_id = chain->chain_id;
  if (network_manager_->KnownChainExists(chain_id, mojom::CoinType::ETH) ||
      network_manager_->CustomChainExists(chain_id, mojom::CoinType::ETH)) {
    return l10n_util::GetStringUTF8(IDS_SETTINGS_WALLET_NETWORKS_EXISTS);
  }
  if (origin.opaque() || add_chain_pending_requests_.contains(chain_id) ||
      HasAddChainRequestFromOrigin(origin)) {
    return l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR);
  }

  add_chain_pending_requests_[chain_id].origin = origin;
  add_chain_pending_requests_[chain_id].request =
      mojom::AddChainRequest::New(MakeOriginInfo(origin), std::move(chain));
  return "";
}

void JsonRpcService::AddEthereumChainRequestCompleted(
    const std::string& chain_id,
    bool approved) {
  if (!add_chain_pending_requests_.contains(chain_id)) {
    return;
  }

  if (!approved) {
    FirePendingRequestCompleted(
        chain_id, l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    add_chain_pending_requests_.erase(chain_id);
    return;
  }

  const auto& chain =
      *add_chain_pending_requests_[chain_id].request->network_info;
  GURL url = GetActiveEndpointUrl(chain);
  if (!url.is_valid()) {
    FirePendingRequestCompleted(
        chain_id,
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_ADD_CHAIN_INVALID_URL));
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
  if (!add_chain_pending_requests_.contains(chain_id)) {
    return;
  }

  const auto& chain =
      *add_chain_pending_requests_[chain_id].request->network_info;
  if (ParseSingleStringResult(api_request_result.value_body()) != chain_id &&
      !skip_eth_chain_id_validation_for_testing_) {
    FirePendingRequestCompleted(
        chain_id,
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_ETH_CHAIN_ID_FAILED,
                                  base::ASCIIToUTF16(rpc_url.spec())));
    add_chain_pending_requests_.erase(chain_id);
    return;
  }

  AddCustomNetworkInternal(chain);

  FirePendingRequestCompleted(chain_id, "");
  add_chain_pending_requests_.erase(chain_id);
}

void JsonRpcService::RemoveChain(const std::string& chain_id,
                                 mojom::CoinType coin,
                                 RemoveChainCallback callback) {
  network_manager_->RemoveCustomNetwork(chain_id, coin);
  std::move(callback).Run(true);
}

bool JsonRpcService::SetNetwork(const std::string& chain_id,
                                mojom::CoinType coin,
                                const std::optional<url::Origin>& origin) {
  if (!network_manager_->SetCurrentChainId(coin, origin, chain_id)) {
    return false;
  }

  FireNetworkChanged(coin, chain_id, origin);

  if (coin == mojom::CoinType::ETH) {
    MaybeUpdateIsEip1559(chain_id);
  }
  return true;
}

void JsonRpcService::SetNetwork(const std::string& chain_id,
                                mojom::CoinType coin,
                                const std::optional<url::Origin>& origin,
                                SetNetworkCallback callback) {
  std::move(callback).Run(SetNetwork(chain_id, coin, origin));
}

void JsonRpcService::GetNetwork(mojom::CoinType coin,
                                const std::optional<::url::Origin>& origin,
                                GetNetworkCallback callback) {
  std::move(callback).Run(GetNetworkSync(coin, origin));
}

mojom::NetworkInfoPtr JsonRpcService::GetNetworkSync(
    mojom::CoinType coin,
    const std::optional<::url::Origin>& origin) {
  return network_manager_->GetChain(GetChainIdSync(coin, origin), coin);
}

void JsonRpcService::MaybeUpdateIsEip1559(const std::string& chain_id) {
  // Only try to update is_eip1559 for localhost or custom chains.
  if (chain_id != brave_wallet::mojom::kLocalhostChainId &&
      !network_manager_->CustomChainExists(chain_id, mojom::CoinType::ETH)) {
    return;
  }

  GetBaseFeePerGas(chain_id,
                   base::BindOnce(&JsonRpcService::UpdateIsEip1559,
                                  weak_ptr_factory_.GetWeakPtr(), chain_id));
}

void JsonRpcService::UpdateIsEip1559(const std::string& chain_id,
                                     const std::string& base_fee_per_gas,
                                     mojom::ProviderError error,
                                     const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    return;
  }
  bool is_eip1559 = !base_fee_per_gas.empty();

  network_manager_->SetEip1559ForCustomChain(chain_id, is_eip1559);
}

void JsonRpcService::AddCustomNetworkInternal(
    const mojom::NetworkInfo& network) {
  // FIL and SOL allow custom chains only over known ones.
  DCHECK(network.coin == mojom::CoinType::ETH ||
         network_manager_->KnownChainExists(network.chain_id, network.coin));

  network_manager_->AddCustomNetwork(network);
  EnsureNativeTokenForNetwork(prefs_, network);
}

void JsonRpcService::FireNetworkChanged(
    mojom::CoinType coin,
    const std::string& chain_id,
    const std::optional<url::Origin>& origin) {
  for (const auto& observer : observers_) {
    observer->ChainChangedEvent(chain_id, coin, origin);
  }
}

std::string JsonRpcService::GetChainIdSync(
    mojom::CoinType coin,
    const std::optional<::url::Origin>& origin) const {
  return network_manager_->GetCurrentChainId(coin, origin);
}

void JsonRpcService::GetDefaultChainId(
    mojom::CoinType coin,
    mojom::JsonRpcService::GetDefaultChainIdCallback callback) {
  std::move(callback).Run(GetChainIdSync(coin, std::nullopt));
}

void JsonRpcService::GetChainIdForOrigin(
    mojom::CoinType coin,
    const ::url::Origin& origin,
    mojom::JsonRpcService::GetChainIdForOriginCallback callback) {
  std::move(callback).Run(GetChainIdSync(coin, origin));
}

void JsonRpcService::GetAllNetworks(GetAllNetworksCallback callback) {
  std::move(callback).Run(network_manager_->GetAllChains());
}

void JsonRpcService::GetCustomNetworks(mojom::CoinType coin,
                                       GetCustomNetworksCallback callback) {
  std::vector<std::string> chain_ids;
  for (const auto& it : network_manager_->GetAllCustomChains(coin)) {
    chain_ids.push_back(it->chain_id);
  }
  std::move(callback).Run(std::move(chain_ids));
}

void JsonRpcService::GetKnownNetworks(mojom::CoinType coin,
                                      GetKnownNetworksCallback callback) {
  std::vector<std::string> chain_ids;
  for (const auto& it : network_manager_->GetAllKnownChains(coin)) {
    chain_ids.push_back(it->chain_id);
  }
  std::move(callback).Run(std::move(chain_ids));
}

void JsonRpcService::GetHiddenNetworks(mojom::CoinType coin,
                                       GetHiddenNetworksCallback callback) {
  auto hidden_networks = network_manager_->GetHiddenNetworks(coin);

  // Currently selected chain is never hidden for coin.
  std::erase(hidden_networks,
             base::ToLowerASCII(GetChainIdSync(coin, std::nullopt)));

  std::move(callback).Run(hidden_networks);
}

void JsonRpcService::AddHiddenNetwork(mojom::CoinType coin,
                                      const std::string& chain_id,
                                      AddHiddenNetworkCallback callback) {
  network_manager_->AddHiddenNetwork(coin, chain_id);

  std::move(callback).Run(true);
}

void JsonRpcService::RemoveHiddenNetwork(mojom::CoinType coin,
                                         const std::string& chain_id,
                                         RemoveHiddenNetworkCallback callback) {
  network_manager_->RemoveHiddenNetwork(coin, chain_id);

  std::move(callback).Run(true);
}

void JsonRpcService::GetBlockNumber(const std::string& chain_id,
                                    GetBlockNumberCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetBlockNumber,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_blockNumber(), true,
                  GetNetworkURL(chain_id, mojom::CoinType::ETH),
                  std::move(internal_callback));
}

void JsonRpcService::GetCode(const std::string& address,
                             mojom::CoinType coin,
                             const std::string& chain_id,
                             GetCodeCallback callback) {
  auto network_url = GetNetworkURL(chain_id, coin);
  if (coin != mojom::CoinType::ETH || !network_url.is_valid()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetCode, weak_ptr_factory_.GetWeakPtr(),
                     std::move(callback));
  RequestInternal(eth::eth_getCode(address, "latest"), true, network_url,
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
  if (!ParseFilStateSearchMsgLimited(api_request_result.value_body(), cid,
                                     &exit_code)) {
    mojom::FilecoinProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::FilecoinProviderError>(
        api_request_result.value_body(), &error, &error_message);
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
  if (!eth::ParseEthGetBlockNumber(api_request_result.value_body(),
                                   &block_number)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run(0, error, error_message);
    return;
  }

  std::move(callback).Run(block_number, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetFeeHistory(const std::string& chain_id,
                                   GetFeeHistoryCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetFeeHistory,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback =
      base::BindOnce(&ConvertAllNumbersToString, "/result");

  RequestInternal(eth::eth_feeHistory("0x28",  // blockCount = 40
                                      kEthereumBlockTagLatest,
                                      std::vector<double>{20, 50, 80}),
                  true, GetNetworkURL(chain_id, mojom::CoinType::ETH),
                  std::move(internal_callback), std::move(conversion_callback));
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
  if (!eth::ParseEthGetFeeHistory(api_request_result.value_body(),
                                  &base_fee_per_gas, &gas_used_ratio,
                                  &oldest_block, &reward)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult(api_request_result.value_body(), &error, &error_message);
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
  auto network_url = GetNetworkURL(chain_id, coin);
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
    RequestInternal(eth::eth_getBalance(address, kEthereumBlockTagLatest), true,
                    network_url, std::move(internal_callback));
    return;
  } else if (coin == mojom::CoinType::FIL) {
    auto internal_callback =
        base::BindOnce(&JsonRpcService::OnFilGetBalance,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback));
    RequestInternal(fil::getBalance(address), true, network_url,
                    std::move(internal_callback));
    return;
  } else {
    NOTREACHED_IN_MIGRATION();
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
  if (!eth::ParseEthGetBalance(api_request_result.value_body(), &balance)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
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
  auto balance = ParseFilGetBalance(api_request_result.value_body());
  if (!balance) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(*balance, mojom::ProviderError::kSuccess, "");
}
void JsonRpcService::GetFilStateSearchMsgLimited(
    const std::string& chain_id,
    const std::string& cid,
    uint64_t period,
    GetFilStateSearchMsgLimitedCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::FIL);
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

void JsonRpcService::GetFilBlockHeight(const std::string& chain_id,
                                       GetFilBlockHeightCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::FIL);
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
  if (!ParseFilGetChainHead(api_request_result.value_body(), &height)) {
    mojom::FilecoinProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::FilecoinProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run(height, error, error_message);
    return;
  }

  std::move(callback).Run(height, mojom::FilecoinProviderError::kSuccess, "");
}

void JsonRpcService::GetFilTransactionCount(const std::string& chain_id,
                                            const std::string& address,
                                            GetFilTxCountCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::FIL);
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

void JsonRpcService::GetEthTransactionCount(const std::string& chain_id,
                                            const std::string& address,
                                            GetTxCountCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        0, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnEthGetTransactionCount,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  RequestInternal(
      eth::eth_getTransactionCount(address, kEthereumBlockTagLatest), true,
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
  auto count = ParseFilGetTransactionCount(api_request_result.value_body());
  if (!count) {
    mojom::FilecoinProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::FilecoinProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run(0u, error, error_message);
    return;
  }

  std::move(callback).Run(static_cast<uint256_t>(count.value()),
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
  if (!eth::ParseEthGetTransactionCount(api_request_result.value_body(),
                                        &count)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run(0, error, error_message);
    return;
  }

  std::move(callback).Run(count, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetTransactionReceipt(const std::string& chain_id,
                                           const std::string& tx_hash,
                                           GetTxReceiptCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetTransactionReceipt,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_getTransactionReceipt(tx_hash), true,
                  GetNetworkURL(chain_id, mojom::CoinType::ETH),
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
  if (!eth::ParseEthGetTransactionReceipt(api_request_result.value_body(),
                                          &receipt)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run(receipt, error, error_message);
    return;
  }

  std::move(callback).Run(receipt, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::SendRawTransaction(const std::string& chain_id,
                                        const std::string& signed_tx,
                                        SendRawTxCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnSendRawTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_sendRawTransaction(signed_tx), true,
                  GetNetworkURL(chain_id, mojom::CoinType::ETH),
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
  auto tx_hash =
      eth::ParseEthSendRawTransaction(api_request_result.value_body());
  if (!tx_hash) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(*tx_hash, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetERC20TokenBalance(
    const std::string& contract,
    const std::string& address,
    const std::string& chain_id,
    JsonRpcService::GetERC20TokenBalanceCallback callback) {
  std::string data;
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);
  if (!erc20::BalanceOf(address, &data) || !network_url.is_valid()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetERC20TokenBalance,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(
      eth::eth_call("", contract, "", "", "", data, kEthereumBlockTagLatest),
      true, network_url, std::move(internal_callback));
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
  auto result = eth::ParseEthCall(api_request_result.value_body());
  if (!result) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  // (uint256)
  auto type = eth_abi::Tuple().AddTupleType(eth_abi::Uint(256)).build();
  const auto& args = eth::DecodeEthCallResponse(*result, type);
  if (args == std::nullopt) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::move(callback).Run(args->at(0), mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::OnGetCode(GetCodeCallback callback,
                               APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  // Result is 0x when the address was an EOA
  auto result = ParseSingleStringResult(api_request_result.value_body());
  if (!result) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::move(callback).Run(*result, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetERC20TokenAllowance(
    const std::string& contract_address,
    const std::string& owner_address,
    const std::string& spender_address,
    const std::string& chain_id,
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
  RequestInternal(eth::eth_call("", contract_address, "", "", "", data,
                                kEthereumBlockTagLatest),
                  true, GetNetworkURL(chain_id, mojom::CoinType::ETH),
                  std::move(internal_callback));
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
  auto result = eth::ParseEthCall(api_request_result.value_body());
  if (!result) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  // (uint256)
  auto type = eth_abi::Tuple().AddTupleType(eth_abi::Uint(256)).build();
  const auto& args = eth::DecodeEthCallResponse(*result, type);
  if (args == std::nullopt) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::move(callback).Run(args->at(0), mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetERC20TokenBalances(
    const std::vector<std::string>& token_contract_addresses,
    const std::string& user_address,
    const std::string& chain_id,
    GetERC20TokenBalancesCallback callback) {
  const auto& balance_scanner_contract_addresses =
      GetEthBalanceScannerContractAddresses();
  if (!base::Contains(balance_scanner_contract_addresses, chain_id)) {
    std::move(callback).Run(
        {}, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  const auto& balance_scanner_contract_address =
      balance_scanner_contract_addresses.at(chain_id);

  if (token_contract_addresses.empty() || user_address.empty()) {
    std::move(callback).Run(
        {}, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::optional<std::string> calldata =
      balance_scanner::TokensBalance(user_address, token_contract_addresses);
  if (!calldata) {
    std::move(callback).Run(
        {}, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        {}, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  // Makes the eth_call request to the balance scanner contract.
  auto internal_callback = base::BindOnce(
      &JsonRpcService::OnGetERC20TokenBalances, weak_ptr_factory_.GetWeakPtr(),
      token_contract_addresses, std::move(callback));
  RequestInternal(
      eth::eth_call(balance_scanner_contract_address, calldata.value()), true,
      network_url, std::move(internal_callback));
}

void JsonRpcService::OnGetERC20TokenBalances(
    const std::vector<std::string>& token_contract_addresses,
    GetERC20TokenBalancesCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        {}, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto result = eth::ParseEthCall(api_request_result.value_body());
  if (!result) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run({}, error, error_message);
    return;
  }

  auto results = eth::DecodeGetERC20TokenBalancesEthCallResponse(*result);
  if (!results) {
    std::move(callback).Run({}, mojom::ProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  // The number of contract addresses supplied to the BalanceScanner
  // should match the number of balances it returns
  if (token_contract_addresses.size() != results->size()) {
    std::move(callback).Run(
        {}, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  // Match up the balances with the contract addresses
  std::vector<mojom::ERC20BalanceResultPtr> erc20_balance_results;
  for (size_t i = 0; i < token_contract_addresses.size(); i++) {
    auto erc20_balance_result = mojom::ERC20BalanceResult::New();
    erc20_balance_result->contract_address = token_contract_addresses[i];
    erc20_balance_result->balance = results->at(i);
    erc20_balance_results.push_back(std::move(erc20_balance_result));
  }

  std::move(callback).Run(std::move(erc20_balance_results),
                          mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::EnsGetContentHash(const std::string& domain,
                                       EnsGetContentHashCallback callback) {
  if (ens_get_content_hash_tasks_.ContainsTaskForDomain(domain)) {
    ens_get_content_hash_tasks_.AddCallbackForDomain(domain,
                                                     std::move(callback));
    return;
  }

  std::optional<bool> allow_offchain;
  if (EnsOffchainPrefEnabled(local_state_prefs_)) {
    allow_offchain = true;
  } else if (EnsOffchainPrefDisabled(local_state_prefs_)) {
    allow_offchain = false;
  }

  // JsonRpcService owns EnsResolverTask instance, so Unretained is safe here.
  auto done_callback = base::BindOnce(
      &JsonRpcService::OnEnsGetContentHashTaskDone, base::Unretained(this));

  ens_get_content_hash_tasks_.AddTask(
      std::make_unique<EnsResolverTask>(
          std::move(done_callback), api_request_helper_.get(),
          api_request_helper_ens_offchain_.get(), MakeContentHashCall(domain),
          domain, NetworkManager::GetEnsRpcUrl(), allow_offchain),
      std::move(callback));
}

void JsonRpcService::GetUnstoppableDomainsResolveMethod(
    GetUnstoppableDomainsResolveMethodCallback callback) {
  std::move(callback).Run(ToMojomResolveMethod(
      decentralized_dns::GetUnstoppableDomainsResolveMethod(
          local_state_prefs_)));
}

void JsonRpcService::GetEnsResolveMethod(GetEnsResolveMethodCallback callback) {
  std::move(callback).Run(ToMojomResolveMethod(
      decentralized_dns::GetENSResolveMethod(local_state_prefs_)));
}

void JsonRpcService::GetEnsOffchainLookupResolveMethod(
    GetEnsOffchainLookupResolveMethodCallback callback) {
  std::move(callback).Run(ToMojomEnsOffchainResolveMethod(
      decentralized_dns::GetEnsOffchainResolveMethod(local_state_prefs_)));
}

void JsonRpcService::GetSnsResolveMethod(GetSnsResolveMethodCallback callback) {
  std::move(callback).Run(ToMojomResolveMethod(
      decentralized_dns::GetSnsResolveMethod(local_state_prefs_)));
}

void JsonRpcService::SetUnstoppableDomainsResolveMethod(
    mojom::ResolveMethod method) {
  decentralized_dns::SetUnstoppableDomainsResolveMethod(
      local_state_prefs_, FromMojomResolveMethod(method));
}

void JsonRpcService::SetEnsResolveMethod(mojom::ResolveMethod method) {
  decentralized_dns::SetENSResolveMethod(local_state_prefs_,
                                         FromMojomResolveMethod(method));
}

void JsonRpcService::SetEnsOffchainLookupResolveMethod(
    mojom::ResolveMethod method) {
  decentralized_dns::SetEnsOffchainResolveMethod(
      local_state_prefs_, FromMojomEnsOffchainResolveMethod(method));
}

void JsonRpcService::SetSnsResolveMethod(mojom::ResolveMethod method) {
  decentralized_dns::SetSnsResolveMethod(local_state_prefs_,
                                         FromMojomResolveMethod(method));
}

void JsonRpcService::EnsGetEthAddr(const std::string& domain,
                                   EnsGetEthAddrCallback callback) {
  if (!IsValidEnsDomain(domain)) {
    std::move(callback).Run(
        "", false, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  if (ens_get_eth_addr_tasks_.ContainsTaskForDomain(domain)) {
    ens_get_eth_addr_tasks_.AddCallbackForDomain(domain, std::move(callback));
    return;
  }

  std::optional<bool> allow_offchain;
  if (EnsOffchainPrefEnabled(local_state_prefs_)) {
    allow_offchain = true;
  } else if (EnsOffchainPrefDisabled(local_state_prefs_)) {
    allow_offchain = false;
  }

  // JsonRpcService owns EnsResolverTask instance, so Unretained is safe here.
  auto done_callback = base::BindOnce(&JsonRpcService::OnEnsGetEthAddrTaskDone,
                                      base::Unretained(this));

  ens_get_eth_addr_tasks_.AddTask(
      std::make_unique<EnsResolverTask>(
          std::move(done_callback), api_request_helper_.get(),
          api_request_helper_ens_offchain_.get(), MakeAddrCall(domain), domain,
          NetworkManager::GetEnsRpcUrl(), allow_offchain),
      std::move(callback));
}

void JsonRpcService::OnEnsGetEthAddrTaskDone(
    EnsResolverTask* task,
    std::optional<EnsResolverTaskResult> task_result,
    std::optional<EnsResolverTaskError> task_error) {
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

void JsonRpcService::SnsGetSolAddr(const std::string& domain,
                                   SnsGetSolAddrCallback callback) {
  if (!IsValidSnsDomain(domain)) {
    std::move(callback).Run(
        "", mojom::SolanaProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  if (sns_get_sol_addr_tasks_.ContainsTaskForDomain(domain)) {
    sns_get_sol_addr_tasks_.AddCallbackForDomain(domain, std::move(callback));
    return;
  }

  // JsonRpcService owns EnsResolverTask instance, so Unretained is safe here.
  auto done_callback = base::BindOnce(&JsonRpcService::OnSnsGetSolAddrTaskDone,
                                      base::Unretained(this));

  sns_get_sol_addr_tasks_.AddTask(
      std::make_unique<SnsResolverTask>(
          std::move(done_callback), api_request_helper_.get(), domain,
          NetworkManager::GetSnsRpcUrl(),
          SnsResolverTask::TaskType::kResolveWalletAddress),
      std::move(callback));
}

void JsonRpcService::OnSnsGetSolAddrTaskDone(
    SnsResolverTask* task,
    std::optional<SnsResolverTaskResult> task_result,
    std::optional<SnsResolverTaskError> task_error) {
  auto callbacks = sns_get_sol_addr_tasks_.TaskDone(task);
  if (callbacks.empty()) {
    return;
  }

  std::string address;
  mojom::SolanaProviderError error =
      task_error ? task_error->error : mojom::SolanaProviderError::kSuccess;
  std::string error_message = task_error ? task_error->error_message : "";

  if (task_result) {
    if (task_result->resolved_address.IsValid()) {
      address = task_result->resolved_address.ToBase58();
    } else {
      error = mojom::SolanaProviderError::kInvalidParams;
      error_message = l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS);
    }
  }

  for (auto& cb : callbacks) {
    std::move(cb).Run(address, error, error_message);
  }
}

void JsonRpcService::SnsResolveHost(const std::string& domain,
                                    SnsResolveHostCallback callback) {
  if (!IsValidSnsDomain(domain)) {
    std::move(callback).Run(
        std::nullopt, mojom::SolanaProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  if (sns_resolve_host_tasks_.ContainsTaskForDomain(domain)) {
    sns_resolve_host_tasks_.AddCallbackForDomain(domain, std::move(callback));
    return;
  }

  // JsonRpcService owns EnsResolverTask instance, so Unretained is safe here.
  auto done_callback = base::BindOnce(&JsonRpcService::OnSnsResolveHostTaskDone,
                                      base::Unretained(this));

  sns_resolve_host_tasks_.AddTask(
      std::make_unique<SnsResolverTask>(std::move(done_callback),
                                        api_request_helper_.get(), domain,
                                        NetworkManager::GetSnsRpcUrl(),
                                        SnsResolverTask::TaskType::kResolveUrl),
      std::move(callback));
}

void JsonRpcService::OnSnsResolveHostTaskDone(
    SnsResolverTask* task,
    std::optional<SnsResolverTaskResult> task_result,
    std::optional<SnsResolverTaskError> task_error) {
  auto callbacks = sns_resolve_host_tasks_.TaskDone(task);
  if (callbacks.empty()) {
    return;
  }

  GURL url;
  mojom::SolanaProviderError error =
      task_error ? task_error->error : mojom::SolanaProviderError::kSuccess;
  std::string error_message = task_error ? task_error->error_message : "";

  if (task_result) {
    if (task_result->resolved_url.is_valid()) {
      url = task_result->resolved_url;
    } else {
      error = mojom::SolanaProviderError::kInternalError;
      error_message = l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR);
    }
  }

  for (auto& cb : callbacks) {
    std::move(cb).Run(url, error, error_message);
  }
}

void JsonRpcService::OnEnsGetContentHashTaskDone(
    EnsResolverTask* task,
    std::optional<EnsResolverTaskResult> task_result,
    std::optional<EnsResolverTaskError> task_error) {
  auto callbacks = ens_get_content_hash_tasks_.TaskDone(task);
  if (callbacks.empty()) {
    return;
  }

  std::optional<std::vector<uint8_t>> content_hash;
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

void JsonRpcService::UnstoppableDomainsResolveDns(
    const std::string& domain,
    UnstoppableDomainsResolveDnsCallback callback) {
  if (ud_resolve_dns_calls_.HasCall(domain)) {
    ud_resolve_dns_calls_.AddCallback(domain, std::move(callback));
    return;
  }

  if (!IsValidUnstoppableDomain(domain)) {
    std::move(callback).Run(
        std::nullopt, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto data = unstoppable_domains::GetMany(unstoppable_domains::GetRecordKeys(),
                                           domain);
  if (!data) {
    std::move(callback).Run(
        std::nullopt, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  ud_resolve_dns_calls_.AddCallback(domain, std::move(callback));
  for (const auto& chain_id : ud_resolve_dns_calls_.GetChains()) {
    auto internal_callback =
        base::BindOnce(&JsonRpcService::OnUnstoppableDomainsResolveDns,
                       weak_ptr_factory_.GetWeakPtr(), domain, chain_id);
    auto eth_call = eth::eth_call(
        "", GetUnstoppableDomainsProxyReaderContractAddress(chain_id), "", "",
        "", *data, kEthereumBlockTagLatest);
    RequestInternal(std::move(eth_call), true,
                    NetworkManager::GetUnstoppableDomainsRpcUrl(chain_id),
                    std::move(internal_callback));
  }
}

void JsonRpcService::OnUnstoppableDomainsResolveDns(
    const std::string& domain,
    const std::string& chain_id,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    ud_resolve_dns_calls_.SetError(
        domain, chain_id, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto values = eth::ParseUnstoppableDomainsProxyReaderGetMany(
      api_request_result.value_body());
  if (!values) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    ud_resolve_dns_calls_.SetError(domain, chain_id, error, error_message);
    return;
  }

  GURL resolved_url = unstoppable_domains::ResolveUrl(*values);
  if (!resolved_url.is_valid()) {
    ud_resolve_dns_calls_.SetNoResult(domain, chain_id);
    return;
  }

  ud_resolve_dns_calls_.SetResult(domain, chain_id, std::move(resolved_url));
}

void JsonRpcService::UnstoppableDomainsGetWalletAddr(
    const std::string& domain,
    mojom::BlockchainTokenPtr token,
    UnstoppableDomainsGetWalletAddrCallback callback) {
  if (!token || !IsValidUnstoppableDomain(domain)) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  unstoppable_domains::WalletAddressKey key = {domain, token->coin,
                                               token->symbol, token->chain_id};
  if (ud_get_eth_addr_calls_.HasCall(key)) {
    ud_get_eth_addr_calls_.AddCallback(key, std::move(callback));
    return;
  }

  auto call_data = unstoppable_domains::GetWalletAddr(
      domain, token->coin, token->symbol, token->chain_id);

  ud_get_eth_addr_calls_.AddCallback(key, std::move(callback));
  for (const auto& chain_id : ud_get_eth_addr_calls_.GetChains()) {
    auto internal_callback =
        base::BindOnce(&JsonRpcService::OnUnstoppableDomainsGetWalletAddr,
                       weak_ptr_factory_.GetWeakPtr(), key, chain_id);
    auto eth_call =
        eth::eth_call(GetUnstoppableDomainsProxyReaderContractAddress(chain_id),
                      ToHex(call_data));
    RequestInternal(std::move(eth_call), true,
                    NetworkManager::GetUnstoppableDomainsRpcUrl(chain_id),
                    std::move(internal_callback));
  }
}

void JsonRpcService::OnUnstoppableDomainsGetWalletAddr(
    const unstoppable_domains::WalletAddressKey& key,
    const std::string& chain_id,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    ud_get_eth_addr_calls_.SetError(
        key, chain_id, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto array_result = eth::ParseUnstoppableDomainsProxyReaderGetMany(
      api_request_result.value_body());
  if (!array_result) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    ud_get_eth_addr_calls_.SetError(key, chain_id, error, error_message);
    return;
  }

  for (auto& item : *array_result) {
    if (!item.empty()) {
      ud_get_eth_addr_calls_.SetResult(key, chain_id, item);
      return;
    }
  }

  ud_get_eth_addr_calls_.SetNoResult(key, chain_id);
}

void JsonRpcService::GetFilEstimateGas(const std::string& chain_id,
                                       const std::string& from_address,
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
  RequestInternal(request, true, GetNetworkURL(chain_id, mojom::CoinType::FIL),
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
  if (!ParseFilEstimateGas(api_request_result.value_body(), &gas_premium,
                           &gas_fee_cap, &gas_limit)) {
    mojom::FilecoinProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::FilecoinProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run("", "", 0, error, error_message);
    return;
  }

  std::move(callback).Run(gas_premium, gas_fee_cap, gas_limit,
                          mojom::FilecoinProviderError::kSuccess, "");
}

void JsonRpcService::GetEstimateGas(const std::string& chain_id,
                                    const std::string& from_address,
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
                  true, GetNetworkURL(chain_id, mojom::CoinType::ETH),
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

  auto result = eth::ParseEthEstimateGas(api_request_result.value_body());
  if (!result) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(*result, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetGasPrice(const std::string& chain_id,
                                 GetGasPriceCallback callback) {
  if (gas_price_for_testing_) {
    std::move(callback).Run(*gas_price_for_testing_,
                            mojom::ProviderError::kSuccess, "");
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetGasPrice,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_gasPrice(), true,
                  GetNetworkURL(chain_id, mojom::CoinType::ETH),
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

  auto result = eth::ParseEthGasPrice(api_request_result.value_body());
  if (!result) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(*result, mojom::ProviderError::kSuccess, "");
}

// Retrieves the BASEFEE per gas for a given chain ID.
//
// If the chain does not support EIP-1559, and assuming no other provider
// errors, an empty string as base fee with kSuccess error will be delivered
// to the provided callback.
void JsonRpcService::GetBaseFeePerGas(const std::string& chain_id,
                                      GetBaseFeePerGasCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetBaseFeePerGas,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_getBlockByNumber(kEthereumBlockTagLatest, false),
                  true, GetNetworkURL(chain_id, mojom::CoinType::ETH),
                  std::move(internal_callback));
}

void JsonRpcService::OnGetBaseFeePerGas(GetBaseFeePerGasCallback callback,
                                        APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto result = ParseResultDict(api_request_result.value_body());
  if (!result) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  const std::string* base_fee = result->FindString("baseFeePerGas");
  if (base_fee && !base_fee->empty()) {
    std::move(callback).Run(*base_fee, mojom::ProviderError::kSuccess, "");
  } else {
    // Successful response, but the chain does not support EIP-1559.
    std::move(callback).Run("", mojom::ProviderError::kSuccess, "");
  }
}

void JsonRpcService::GetBlockByNumber(const std::string& chain_id,
                                      const std::string& block_number,
                                      GetBlockByNumberCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetBlockByNumber,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_getBlockByNumber(block_number, false), true,
                  GetNetworkURL(chain_id, mojom::CoinType::ETH),
                  std::move(internal_callback));
}

void JsonRpcService::OnGetBlockByNumber(GetBlockByNumberCallback callback,
                                        APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        base::Value(), mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto result = ParseResultValue(api_request_result.value_body());
  if (!result) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run(base::Value(), error, error_message);
    return;
  }

  std::move(callback).Run(std::move(*result), mojom::ProviderError::kSuccess,
                          "");
}

/*static*/
bool JsonRpcService::IsValidEnsDomain(const std::string& domain) {
  static const base::NoDestructor<re2::RE2> kDomainRegex(kEnsDomainPattern);
  return re2::RE2::FullMatch(domain, *kDomainRegex);
}

/*static*/
bool JsonRpcService::IsValidSnsDomain(const std::string& domain) {
  static const base::NoDestructor<re2::RE2> kDomainRegex(kSnsDomainPattern);
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
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);
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
  RequestInternal(
      eth::eth_call("", contract, "", "", "", data, kEthereumBlockTagLatest),
      true, network_url, std::move(internal_callback));
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
  if (!eth::ParseAddressResult(api_request_result.value_body(), &address) ||
      address.empty()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
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
                                       GetERC721MetadataCallback callback) {
  nft_metadata_fetcher_->GetEthTokenMetadata(
      contract_address, token_id, chain_id, kERC721MetadataInterfaceId,
      std::move(callback));
}

void JsonRpcService::GetERC1155Metadata(const std::string& contract_address,
                                        const std::string& token_id,
                                        const std::string& chain_id,
                                        GetERC1155MetadataCallback callback) {
  nft_metadata_fetcher_->GetEthTokenMetadata(
      contract_address, token_id, chain_id, kERC1155MetadataInterfaceId,
      std::move(callback));
}

void JsonRpcService::GetEthTokenUri(const std::string& chain_id,
                                    const std::string& contract_address,
                                    const std::string& token_id,
                                    const std::string& interface_id,
                                    GetEthTokenUriCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        GURL(), mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  if (!EthAddress::IsValidAddress(contract_address)) {
    std::move(callback).Run(
        GURL(), mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  uint256_t token_id_uint = 0;
  if (!HexValueToUint256(token_id, &token_id_uint)) {
    std::move(callback).Run(
        GURL(), mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  std::string function_signature;
  if (interface_id == kERC721MetadataInterfaceId) {
    if (!erc721::TokenUri(token_id_uint, &function_signature)) {
      std::move(callback).Run(
          GURL(), mojom::ProviderError::kInvalidParams,
          l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
      return;
    }
  } else if (interface_id == kERC1155MetadataInterfaceId) {
    if (!erc1155::Uri(token_id_uint, &function_signature)) {
      std::move(callback).Run(
          GURL(), mojom::ProviderError::kInvalidParams,
          l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
      return;
    }
  } else {
    // Unknown inteface ID
    std::move(callback).Run(
        GURL(), mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetEthTokenUri,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  RequestInternal(eth::eth_call("", contract_address, "", "", "",
                                function_signature, kEthereumBlockTagLatest),
                  true, network_url, std::move(internal_callback));
}

void JsonRpcService::OnGetEthTokenUri(GetEthTokenUriCallback callback,
                                      APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        GURL(), mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  // Parse response JSON that wraps the result
  GURL url;
  if (!eth::ParseTokenUri(api_request_result.value_body(), &url)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run(GURL(), error, error_message);
    return;
  }

  std::move(callback).Run(url, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetERC1155TokenBalance(
    const std::string& contract_address,
    const std::string& token_id,
    const std::string& owner_address,
    const std::string& chain_id,
    GetERC1155TokenBalanceCallback callback) {
  const auto eth_account_address = EthAddress::FromHex(owner_address);
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);

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
  RequestInternal(eth::eth_call("", contract_address, "", "", "", data,
                                kEthereumBlockTagLatest),
                  true, network_url, std::move(internal_callback));
}

void JsonRpcService::EthGetLogs(const std::string& chain_id,
                                base::Value::Dict filter_options,
                                EthGetLogsCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        std::vector<Log>(), {}, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnEthGetLogs,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_getLogs(std::move(filter_options)), true,
                  network_url, std::move(internal_callback));
}

void JsonRpcService::OnEthGetLogs(EthGetLogsCallback callback,
                                  APIRequestResult api_request_result) {
  std::vector<Log> logs;
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        logs, {}, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  if (!eth::ParseEthGetLogs(api_request_result.value_body(), &logs)) {
    std::move(callback).Run(logs, {}, mojom::ProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  std::move(callback).Run(logs, api_request_result.value_body().Clone(),
                          mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetSupportsInterface(
    const std::string& contract_address,
    const std::string& interface_id,
    const std::string& chain_id,
    GetSupportsInterfaceCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);
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
  RequestInternal(eth::eth_call("", contract_address, "", "", "", data,
                                kEthereumBlockTagLatest),
                  true, network_url, std::move(internal_callback));
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

  auto is_supported = ParseBoolResult(api_request_result.value_body());
  if (!is_supported.has_value()) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run(false, error, error_message);
    return;
  }

  std::move(callback).Run(is_supported.value(), mojom::ProviderError::kSuccess,
                          "");
}

void JsonRpcService::GetEthNftStandard(
    const std::string& contract_address,
    const std::string& chain_id,
    const std::vector<std::string>& interfaces,
    GetEthNftStandardCallback callback,
    size_t index) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);
  if (!EthAddress::IsValidAddress(contract_address) ||
      !network_url.is_valid()) {
    std::move(callback).Run(
        std::nullopt, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  if (index >= interfaces.size()) {
    std::move(callback).Run(
        std::nullopt, mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  // Check the next interface
  const std::string& interface_id = interfaces[index];
  auto internal_callback = base::BindOnce(
      &JsonRpcService::OnGetEthNftStandard, weak_ptr_factory_.GetWeakPtr(),
      contract_address, chain_id, interfaces, index, std::move(callback));

  GetSupportsInterface(contract_address, interface_id, chain_id,
                       std::move(internal_callback));
}

void JsonRpcService::OnGetEthNftStandard(
    const std::string& contract_address,
    const std::string& chain_id,
    const std::vector<std::string>& interfaces,
    size_t index,
    GetEthNftStandardCallback callback,
    bool is_supported,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess) {
    std::move(callback).Run(std::nullopt, error, error_message);
    return;
  }

  const std::string& interface_id_checked = interfaces[index];
  if (is_supported) {
    std::move(callback).Run(interface_id_checked,
                            mojom::ProviderError::kSuccess, "");
    return;
  }

  index++;
  if (index >= interfaces.size()) {
    std::move(callback).Run(std::nullopt, mojom::ProviderError::kSuccess, "");
    return;
  }
  // If the contract does not implement the interface, try the next one
  GetEthNftStandard(contract_address, chain_id, interfaces, std::move(callback),
                    index);
}

void JsonRpcService::GetPendingSwitchChainRequests(
    GetPendingSwitchChainRequestsCallback callback) {
  std::move(callback).Run(GetPendingSwitchChainRequestsSync());
}

std::vector<mojom::SwitchChainRequestPtr>
JsonRpcService::GetPendingSwitchChainRequestsSync() {
  std::vector<mojom::SwitchChainRequestPtr> requests;
  for (const auto& request : pending_switch_chain_requests_) {
    requests.push_back(request.second.switch_chain_request.Clone());
  }
  return requests;
}

void JsonRpcService::NotifySwitchChainRequestProcessed(
    const std::string& request_id,
    bool approved) {
  if (!pending_switch_chain_requests_.contains(request_id)) {
    return;
  }

  auto pending_request = std::move(pending_switch_chain_requests_[request_id]);
  pending_switch_chain_requests_.erase(request_id);

  if (approved) {
    // We already check chain id validity in
    // JsonRpcService::AddSwitchEthereumChainRequest so this should always
    // be successful unless chain id differs or we add more check other than
    // chain id
    CHECK(SetNetwork(pending_request.switch_chain_request->chain_id,
                     mojom::CoinType::ETH, pending_request.origin));
  }
  auto callback = std::move(pending_request.switch_chain_callback);
  base::Value id = std::move(pending_request.switch_chain_id);

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
  if (!GetNetworkURL(chain_id, mojom::CoinType::ETH).is_valid()) {
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
  if (GetChainIdSync(mojom::CoinType::ETH, origin) == chain_id) {
    reject = false;
    std::move(callback).Run(std::move(id), base::Value(), reject, "", false);
    return false;
  }

  // There can be only 1 request per origin
  if (HasSwitchChainRequestFromOrigin(origin)) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    reject = true;
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return false;
  }

  auto request_id = GenerateRandomHexString();
  auto& pending_request = pending_switch_chain_requests_[request_id];
  pending_request.switch_chain_request = mojom::SwitchChainRequest::New(
      request_id, MakeOriginInfo(origin), chain_id);
  pending_request.origin = origin;
  pending_request.switch_chain_callback = std::move(callback);
  pending_request.switch_chain_id = std::move(id);

  return true;
}

void JsonRpcService::GetEthTokenSymbol(
    const std::string& contract_address,
    const std::string& chain_id,
    GetEthTokenStringResultCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  const std::string data = GetFunctionHash("symbol()");
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetEthTokenSymbol,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_call(contract_address, data), true, network_url,
                  std::move(internal_callback));
}

void JsonRpcService::OnGetEthTokenSymbol(
    GetEthTokenStringResultCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string symbol;
  if (!eth::ParseStringResult(api_request_result.value_body(), &symbol)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }
  std::move(callback).Run(symbol, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetEthTokenDecimals(
    const std::string& contract_address,
    const std::string& chain_id,
    GetEthTokenStringResultCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  const std::string data = GetFunctionHash("decimals()");
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetEthTokenDecimals,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_call(contract_address, data), true, network_url,
                  std::move(internal_callback));
}

void JsonRpcService::OnGetEthTokenDecimals(
    GetEthTokenStringResultCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto result = eth::ParseEthCall(api_request_result.value_body());
  if (!result) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  // (uint8)
  auto type = eth_abi::Tuple().AddTupleType(eth_abi::Uint(8)).build();
  const auto& args = eth::DecodeEthCallResponse(*result, type);
  if (args == std::nullopt) {
    std::move(callback).Run("", mojom::ProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  std::move(callback).Run(args->at(0), mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetEthTokenName(const std::string& contract_address,
                                     const std::string& chain_id,
                                     GetEthTokenStringResultCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::ETH);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }
  const std::string data = GetFunctionHash("name()");
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetEthTokenName,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(eth::eth_call(contract_address, data), true, network_url,
                  std::move(internal_callback));
}

void JsonRpcService::OnGetEthTokenName(GetEthTokenStringResultCallback callback,
                                       APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        "", mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::string name;
  if (!eth::ParseStringResult(api_request_result.value_body(), &name)) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }
  std::move(callback).Run(name, mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::GetEthTokenInfo(const std::string& contract_address,
                                     const std::string& chain_id,
                                     GetEthTokenInfoCallback callback) {
  GetEthTokenSymbol(
      contract_address, chain_id,
      base::BindOnce(&JsonRpcService::OnGetEthTokenSymbolForInfo,
                     weak_ptr_factory_.GetWeakPtr(), contract_address, chain_id,
                     std::move(callback)));
}

void JsonRpcService::OnGetEthTokenSymbolForInfo(
    const std::string& contract_address,
    const std::string& chain_id,
    GetEthTokenInfoCallback callback,
    const std::string& symbol,
    mojom::ProviderError error,
    const std::string& error_message) {
  // ERC-1155 does not strictly require a symbol, so we allow this to be empty.
  GetEthTokenName(
      contract_address, chain_id,
      base::BindOnce(&JsonRpcService::OnGetEthTokenNameForInfo,
                     weak_ptr_factory_.GetWeakPtr(), contract_address, chain_id,
                     std::move(callback), symbol));
}

void JsonRpcService::OnGetEthTokenNameForInfo(
    const std::string& contract_address,
    const std::string& chain_id,
    GetEthTokenInfoCallback callback,
    const std::string& symbol,
    const std::string& name,
    mojom::ProviderError error,
    const std::string& error_message) {
  GetEthTokenDecimals(
      contract_address, chain_id,
      base::BindOnce(&JsonRpcService::OnGetEthTokenDecimalsForInfo,
                     weak_ptr_factory_.GetWeakPtr(), contract_address, chain_id,
                     std::move(callback), symbol, name));
}

void JsonRpcService::OnGetEthTokenDecimalsForInfo(
    const std::string& contract_address,
    const std::string& chain_id,
    GetEthTokenInfoCallback callback,
    const std::string& symbol,
    const std::string& name,
    const std::string& decimals,
    mojom::ProviderError error,
    const std::string& error_message) {
  auto asset = mojom::BlockchainToken::New();
  asset->name = name;
  asset->symbol = symbol;
  asset->chain_id = chain_id;
  asset->contract_address = contract_address;
  asset->coin = mojom::CoinType::ETH;
  asset->spl_token_program = mojom::SPLTokenProgram::kUnsupported;

  int decimals_int = 0;
  // This condition will never be met since the response of GetEthTokenDecimals
  // is already sanitized, but we keep it here for completeness.
  if (decimals != "" && IsValidHexString(decimals) &&
      !base::HexStringToInt(decimals, &decimals_int)) {
    std::move(callback).Run(nullptr, mojom::ProviderError::kParsingError,
                            l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    return;
  }

  asset->decimals = decimals_int;

  auto coingecko_id = BlockchainRegistry::GetInstance()->GetCoingeckoId(
      asset->chain_id, asset->contract_address);
  if (coingecko_id) {
    asset->coingecko_id = *coingecko_id;
  }

  std::move(callback).Run(std::move(asset), mojom::ProviderError::kSuccess, "");
}

void JsonRpcService::Reset() {
  ClearJsonRpcServiceProfilePrefs(prefs_);

  add_chain_pending_requests_.clear();

  base::Value formed_response = GetProviderErrorDictionary(
      mojom::ProviderError::kUserRejectedRequest,
      l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  // Reject pending suggest token requests when network changed.
  for (auto& pending_request : pending_switch_chain_requests_) {
    bool reject = true;

    auto callback = std::move(pending_request.second.switch_chain_callback);
    base::Value id = std::move(pending_request.second.switch_chain_id);

    std::move(callback).Run(std::move(id), formed_response.Clone(), reject, "",
                            false);
  }
  pending_switch_chain_requests_.clear();
}

void JsonRpcService::GetSolanaBalance(const std::string& pubkey,
                                      const std::string& chain_id,
                                      GetSolanaBalanceCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::SOL);
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
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::SOL);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        "", 0, "", mojom::SolanaProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  GetSPLTokenProgramByMint(
      chain_id, token_mint_address,
      base::BindOnce(&JsonRpcService::OnGetSPLTokenProgramByMint,
                     weak_ptr_factory_.GetWeakPtr(), wallet_address,
                     token_mint_address, network_url, std::move(callback)));
}

void JsonRpcService::OnGetSPLTokenProgramByMint(
    const std::string& wallet_address,
    const std::string& token_mint_address,
    const GURL& network_url,
    GetSPLTokenAccountBalanceCallback callback,
    mojom::SPLTokenProgram token_program,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  std::optional<std::string> associated_token_account =
      SolanaKeyring::GetAssociatedTokenAccount(token_mint_address,
                                               wallet_address, token_program);
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
  if (!solana::ParseGetBalance(api_request_result.value_body(), &balance)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
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
  if (!solana::ParseGetTokenAccountBalance(api_request_result.value_body(),
                                           &amount, &decimals,
                                           &ui_amount_string)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);

    // Treat balance as 0 if the associated token account is not created yet.
    if (error == mojom::SolanaProviderError::kInvalidParams &&
        error_message.find(kAccountNotCreatedError) != std::string::npos) {
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

void JsonRpcService::GetSolTokenMetadata(const std::string& chain_id,
                                         const std::string& token_mint_address,
                                         GetSolTokenMetadataCallback callback) {
  nft_metadata_fetcher_->GetSolTokenMetadata(chain_id, token_mint_address,
                                             std::move(callback));
}

void JsonRpcService::GetNftMetadatas(
    mojom::CoinType coin,
    std::vector<mojom::NftIdentifierPtr> nft_identifiers,
    GetNftMetadatasCallback callback) {
  std::string error_message;
  if (!ValidateNftIdentifiers(coin, nft_identifiers, error_message)) {
    std::move(callback).Run({}, error_message);
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetNftMetadatas,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  simple_hash_client_->GetNftMetadatas(coin, std::move(nft_identifiers),
                                       std::move(internal_callback));
}

void JsonRpcService::OnGetNftMetadatas(
    GetNftMetadatasCallback callback,
    std::vector<mojom::NftMetadataPtr> metadatas) {
  // If there are no metadatas, then there was an error
  if (metadatas.empty()) {
    std::move(callback).Run(
        {}, l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::move(callback).Run(std::move(metadatas), "");
}

void JsonRpcService::GetNftBalances(
    const std::string& wallet_address,
    std::vector<mojom::NftIdentifierPtr> nft_identifiers,
    mojom::CoinType coin,
    GetNftBalancesCallback callback) {
  std::string error_message;
  if (!ValidateNftIdentifiers(coin, nft_identifiers, error_message)) {
    std::move(callback).Run({}, error_message);
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetNftBalances,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  simple_hash_client_->GetNftBalances(wallet_address,
                                      std::move(nft_identifiers), coin,
                                      std::move(internal_callback));
}

void JsonRpcService::OnGetNftBalances(GetNftBalancesCallback callback,
                                      const std::vector<uint64_t>& balances) {
  std::move(callback).Run(balances, "");
}

void JsonRpcService::IsSolanaBlockhashValid(
    const std::string& chain_id,
    const std::string& blockhash,
    const std::optional<std::string>& commitment,
    IsSolanaBlockhashValidCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnIsSolanaBlockhashValid,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::isBlockhashValid(blockhash, commitment), true,
                  GetNetworkURL(chain_id, mojom::CoinType::SOL),
                  std::move(internal_callback));
}

void JsonRpcService::OnIsSolanaBlockhashValid(
    IsSolanaBlockhashValidCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        false, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto is_valid =
      solana::ParseIsBlockhashValid(api_request_result.value_body());
  if (!is_valid) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run(false, error, error_message);
    return;
  }

  std::move(callback).Run(*is_valid, mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::SendFilecoinTransaction(
    const std::string& chain_id,
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
  RequestInternal(request.value(), true,
                  GetNetworkURL(chain_id, mojom::CoinType::FIL),
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
  if (!ParseSendFilecoinTransaction(api_request_result.value_body(), &cid)) {
    mojom::FilecoinProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::FilecoinProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(cid, mojom::FilecoinProviderError::kSuccess, "");
}

void JsonRpcService::SendSolanaTransaction(
    const std::string& chain_id,
    const std::string& signed_tx,
    std::optional<SolanaTransaction::SendOptions> send_options,
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

  auto request = solana::sendTransaction(signed_tx, std::move(send_options));

  RequestInternal(request, true, GetNetworkURL(chain_id, mojom::CoinType::SOL),
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
  if (!solana::ParseSendTransaction(api_request_result.value_body(),
                                    &transaction_id)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run("", error, error_message);
    return;
  }

  std::move(callback).Run(transaction_id, mojom::SolanaProviderError::kSuccess,
                          "");
}

void JsonRpcService::GetSolanaLatestBlockhash(
    const std::string& chain_id,
    GetSolanaLatestBlockhashCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaLatestBlockhash,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::getLatestBlockhash(), true,
                  GetNetworkURL(chain_id, mojom::CoinType::SOL),
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
  if (!solana::ParseGetLatestBlockhash(api_request_result.value_body(),
                                       &blockhash, &last_valid_block_height)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run("", 0, error, error_message);
    return;
  }

  std::move(callback).Run(blockhash, last_valid_block_height,
                          mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::GetSolanaSignatureStatuses(
    const std::string& chain_id,
    const std::vector<std::string>& tx_signatures,
    GetSolanaSignatureStatusesCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaSignatureStatuses,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(
      solana::getSignatureStatuses(tx_signatures), true,
      GetNetworkURL(chain_id, mojom::CoinType::SOL),
      std::move(internal_callback),
      base::BindOnce(&ConvertMultiUint64InObjectArrayToString, "/result/value",
                     "", std::vector<std::string>({"slot", "confirmations"})));
}

void JsonRpcService::OnGetSolanaSignatureStatuses(
    GetSolanaSignatureStatusesCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        std::vector<std::optional<SolanaSignatureStatus>>(),
        mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::vector<std::optional<SolanaSignatureStatus>> statuses;
  if (!solana::ParseGetSignatureStatuses(api_request_result.value_body(),
                                         &statuses)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run(std::vector<std::optional<SolanaSignatureStatus>>(),
                            error, error_message);
    return;
  }

  std::move(callback).Run(statuses, mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::GetSolanaAccountInfo(
    const std::string& chain_id,
    const std::string& pubkey,
    GetSolanaAccountInfoCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::SOL);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        {}, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaAccountInfo,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  RequestInternal(solana::getAccountInfo(pubkey), true, network_url,
                  std::move(internal_callback),
                  solana::ConverterForGetAccountInfo());
}

void JsonRpcService::OnGetSolanaAccountInfo(
    GetSolanaAccountInfoCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        std::nullopt, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  std::optional<SolanaAccountInfo> account_info;
  if (!solana::ParseGetAccountInfo(api_request_result.value_body(),
                                   &account_info)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run(std::nullopt, error, error_message);
    return;
  }

  std::move(callback).Run(std::move(account_info),
                          mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::GetSolanaFeeForMessage(
    const std::string& chain_id,
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
                  GetNetworkURL(chain_id, mojom::CoinType::SOL),
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
  if (!solana::ParseGetFeeForMessage(api_request_result.value_body(), &fee)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run(0, error, error_message);
    return;
  }

  std::move(callback).Run(fee, mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::GetSolanaBlockHeight(
    const std::string& chain_id,
    GetSolanaBlockHeightCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetSolanaBlockHeight,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(solana::getBlockHeight(), true,
                  GetNetworkURL(chain_id, mojom::CoinType::SOL),
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
  if (!solana::ParseGetBlockHeight(api_request_result.value_body(),
                                   &block_height)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run(0, error, error_message);
    return;
  }

  std::move(callback).Run(block_height, mojom::SolanaProviderError::kSuccess,
                          "");
}

void JsonRpcService::GetSolanaTokenAccountsByOwner(
    const SolanaAddress& pubkey,
    const std::string& chain_id,
    GetSolanaTokenAccountsByOwnerCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::SOL);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        {}, mojom::SolanaProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  const auto barrier_callback =
      base::BarrierCallback<SolanaRPCResponse<SolanaAccountInfo>>(
          2, /* token and token2022 */
          base::BindOnce(&MergeSolanaRPCResponses<SolanaAccountInfo>,
                         std::move(callback)));

  for (const auto& program_id :
       {mojom::kSolanaTokenProgramId, mojom::kSolanaToken2022ProgramId}) {
    RequestInternal(
        solana::getTokenAccountsByOwner(pubkey.ToBase58(), "base64",
                                        program_id),
        true, network_url,
        base::BindOnce(&JsonRpcService::OnGetSolanaTokenAccountsByOwner,
                       weak_ptr_factory_.GetWeakPtr(), barrier_callback),
        base::BindOnce(&ConvertMultiUint64InObjectArrayToString,
                       "/result/value", "/account",
                       std::vector<std::string>({"lamports", "rentEpoch"})));
  }
}

void JsonRpcService::OnGetSolanaTokenAccountsByOwner(
    base::OnceCallback<void(SolanaRPCResponse<SolanaAccountInfo>)> callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        {{},
         mojom::SolanaProviderError::kInternalError,
         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
    return;
  }

  std::vector<SolanaAccountInfo> token_accounts;
  if (!solana::ParseGetTokenAccountsByOwner(api_request_result.value_body(),
                                            &token_accounts)) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run({{}, error, error_message});
    return;
  }

  std::move(callback).Run(
      {std::move(token_accounts), mojom::SolanaProviderError::kSuccess, ""});
}

void JsonRpcService::GetSPLTokenBalances(const std::string& pubkey,
                                         const std::string& chain_id,
                                         GetSPLTokenBalancesCallback callback) {
  auto network_url = GetNetworkURL(chain_id, mojom::CoinType::SOL);
  if (!network_url.is_valid()) {
    std::move(callback).Run(
        {}, mojom::SolanaProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  const auto barrier_callback =
      base::BarrierCallback<SolanaRPCResponse<mojom::SPLTokenAmountPtr>>(
          2, /* token and token2022 */
          base::BindOnce(&MergeSolanaRPCResponses<mojom::SPLTokenAmountPtr>,
                         std::move(callback)));

  for (const auto& program_id :
       {mojom::kSolanaTokenProgramId, mojom::kSolanaToken2022ProgramId}) {
    RequestInternal(
        solana::getTokenAccountsByOwner(pubkey, "jsonParsed", program_id), true,
        network_url,
        base::BindOnce(&JsonRpcService::OnGetSPLTokenBalances,
                       weak_ptr_factory_.GetWeakPtr(), barrier_callback));
  }
}

void JsonRpcService::OnGetSPLTokenBalances(
    base::OnceCallback<void(SolanaRPCResponse<mojom::SPLTokenAmountPtr>)>
        callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        {{},
         mojom::SolanaProviderError::kInternalError,
         l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR)});
    return;
  }

  auto result =
      solana::ParseGetSPLTokenBalances(api_request_result.value_body());
  if (!result) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run({{}, error, error_message});
    return;
  }

  std::move(callback).Run(
      {std::move(*result), mojom::SolanaProviderError::kSuccess, ""});
}

void JsonRpcService::AnkrGetAccountBalances(
    const std::string& account_address,
    const std::vector<std::string>& chain_ids,
    AnkrGetAccountBalancesCallback callback) {
  if (!IsAnkrBalancesEnabled()) {
    std::move(callback).Run(
        {}, mojom::ProviderError::kMethodNotFound,
        l10n_util::GetStringUTF8(IDS_WALLET_REQUEST_PROCESSING_ERROR));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnAnkrGetAccountBalances,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));

  auto conversion_callback =
      base::BindOnce(&ConvertAllNumbersToString, "/result");

  // Translate chain ids to Ankr blockchains. Unsupported chain ids will be
  // ignored.
  std::vector<std::string> blockchains;
  for (const auto& chain_id : chain_ids) {
    if (auto blockchain = GetAnkrBlockchainFromChainId(chain_id); blockchain) {
      blockchains.push_back(*blockchain);
    }
  }

  std::string encoded_params =
      EncodeAnkrGetAccountBalancesParams(account_address, blockchains);

  auto headers = IsEndpointUsingBraveWalletProxy(GURL(kAnkrAdvancedAPIBaseURL))
                     ? MakeBraveServicesKeyHeaders()
                     : base::flat_map<std::string, std::string>();

  api_request_helper_->Request(
      "POST", GURL(kAnkrAdvancedAPIBaseURL), encoded_params, "application/json",
      std::move(internal_callback), headers,
      {.auto_retry_on_network_change = false}, std::move(conversion_callback));
}

void JsonRpcService::OnAnkrGetAccountBalances(
    AnkrGetAccountBalancesCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        {}, mojom::ProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto result =
      ankr::ParseGetAccountBalanceResponse(api_request_result.value_body());
  if (!result) {
    mojom::ProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::ProviderError>(api_request_result.value_body(),
                                           &error, &error_message);

    if (!error_message.empty()) {
      std::move(callback).Run({}, error, error_message);
    } else {
      std::move(callback).Run(
          {}, mojom::ProviderError::kParsingError,
          l10n_util::GetStringUTF8(IDS_WALLET_PARSING_ERROR));
    }
    return;
  }

  auto* blockchain_registry = BlockchainRegistry::GetInstance();

  for (auto& balance : *result) {
    std::optional<std::string> coingecko_id =
        blockchain_registry->GetCoingeckoId(balance->asset->chain_id,
                                            balance->asset->contract_address);
    if (coingecko_id) {
      balance->asset->coingecko_id = *coingecko_id;
    }
  }

  std::move(callback).Run(std::move(*result), mojom::ProviderError::kSuccess,
                          "");
}

void JsonRpcService::GetSPLTokenProgramByMint(
    const std::string& chain_id,
    const std::string& mint_address,
    GetSPLTokenProgramByMintCallback callback) {
  if (chain_id.empty() || mint_address.empty()) {
    std::move(callback).Run(
        mojom::SPLTokenProgram::kUnknown,
        mojom::SolanaProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  mojom::BlockchainTokenPtr user_asset;
  if ((user_asset = GetUserAsset(prefs_, mojom::CoinType::SOL, chain_id,
                                 mint_address, "", false, false))) {
    if (user_asset->spl_token_program != mojom::SPLTokenProgram::kUnknown) {
      std::move(callback).Run(user_asset->spl_token_program,
                              mojom::SolanaProviderError::kSuccess, "");
      return;
    }
  } else if (auto token = BlockchainRegistry::GetInstance()->GetTokenByAddress(
                 chain_id, mojom::CoinType::SOL, mint_address)) {
    // In theory token in registry won't have unknown value appears, but we let
    // it fall through to fetch from network as it won't hurt if that does
    // happen somehow.
    if (token->spl_token_program != mojom::SPLTokenProgram::kUnknown) {
      std::move(callback).Run(token->spl_token_program,
                              mojom::SolanaProviderError::kSuccess, "");
      return;
    }
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::ContinueGetSPLTokenProgramByMint,
                     weak_ptr_factory_.GetWeakPtr(), std::move(user_asset),
                     std::move(callback));
  GetSolanaAccountInfo(chain_id, mint_address, std::move(internal_callback));
}

void JsonRpcService::ContinueGetSPLTokenProgramByMint(
    mojom::BlockchainTokenPtr user_asset,
    GetSPLTokenProgramByMintCallback callback,
    std::optional<SolanaAccountInfo> account_info,
    mojom::SolanaProviderError error,
    const std::string& error_message) {
  if (!account_info) {
    std::move(callback).Run(mojom::SPLTokenProgram::kUnknown, error,
                            error_message);
    return;
  }

  mojom::SPLTokenProgram program = mojom::SPLTokenProgram::kUnknown;
  if (account_info->owner == mojom::kSolanaTokenProgramId) {
    program = mojom::SPLTokenProgram::kToken;
  } else if (account_info->owner == mojom::kSolanaToken2022ProgramId) {
    program = mojom::SPLTokenProgram::kToken2022;
  } else {
    program = mojom::SPLTokenProgram::kUnsupported;
  }

  if (user_asset) {
    SetAssetSPLTokenProgram(prefs_, user_asset, program);
  }

  std::move(callback).Run(program, mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::SimulateSolanaTransaction(
    const std::string& chain_id,
    const std::string& unsigned_tx,
    SimulateSolanaTransactionCallback callback) {
  if (unsigned_tx.empty()) {
    std::move(callback).Run(
        0, mojom::SolanaProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_INVALID_PARAMETERS));
    return;
  }

  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnSimulateSolanaTransaction,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(
      solana::simulateTransaction(unsigned_tx), true,
      GetNetworkURL(chain_id, mojom::CoinType::SOL),
      std::move(internal_callback),
      base::BindOnce(&ConvertUint64ToString, "/result/value/unitsConsumed"));
}

void JsonRpcService::OnSimulateSolanaTransaction(
    SimulateSolanaTransactionCallback callback,
    APIRequestResult api_request_result) {
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        0, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }
  std::optional<uint64_t> compute_units =
      solana::ParseSimulateTransaction(api_request_result.value_body());
  if (!compute_units) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run(0, error, error_message);
    return;
  }

  std::move(callback).Run(*compute_units, mojom::SolanaProviderError::kSuccess,
                          "");
}

void JsonRpcService::GetRecentSolanaPrioritizationFees(
    const std::string& chain_id,
    GetRecentSolanaPrioritizationFeesCallback callback) {
  auto internal_callback =
      base::BindOnce(&JsonRpcService::OnGetRecentSolanaPrioritizationFees,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback));
  RequestInternal(
      solana::getRecentPrioritizationFees(), true,
      GetNetworkURL(chain_id, mojom::CoinType::SOL),
      std::move(internal_callback),
      base::BindOnce(&ConvertMultiUint64InObjectArrayToString, "/result", "",
                     std::vector<std::string>{"slot", "prioritizationFee"}));
}

void JsonRpcService::OnGetRecentSolanaPrioritizationFees(
    GetRecentSolanaPrioritizationFeesCallback callback,
    APIRequestResult api_request_result) {
  std::vector<std::pair<uint64_t, uint64_t>> recent_fees;
  if (!api_request_result.Is2XXResponseCode()) {
    std::move(callback).Run(
        recent_fees, mojom::SolanaProviderError::kInternalError,
        l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
    return;
  }

  auto recent_fees_opt =
      solana::ParseGetSolanaPrioritizationFees(api_request_result.value_body());
  if (!recent_fees_opt) {
    mojom::SolanaProviderError error;
    std::string error_message;
    ParseErrorResult<mojom::SolanaProviderError>(
        api_request_result.value_body(), &error, &error_message);
    std::move(callback).Run(recent_fees, error, error_message);
    return;
  }

  std::move(callback).Run(*recent_fees_opt,
                          mojom::SolanaProviderError::kSuccess, "");
}

void JsonRpcService::FetchSolCompressedNftProofData(
    const std::string& token_address,
    SimpleHashClient::FetchSolCompressedNftProofDataCallback callback) {
  simple_hash_client_->FetchSolCompressedNftProofData(token_address,
                                                      std::move(callback));
}

}  // namespace brave_wallet
