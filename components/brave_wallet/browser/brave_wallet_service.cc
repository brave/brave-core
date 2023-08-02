/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include <map>
#include <vector>

#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_allowance_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/sign_message_request.mojom.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/origin.h"

// kBraveWalletUserAssets
// {
//    "ethereum": {
//      "mainnet": // network_id
//        [
//          {
//            "address": "",
//            "name": "Ethereum",
//            "symbol": "ETH",
//            "is_erc20": false,
//            "is_erc721": false,
//            "is_erc1155": false,
//            "is_spam": false,
//            "decimals": 18,
//            "visible": true
//            ...
//          },
//          {
//            "address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
//            "name": "Basic Attention Token",
//            "symbol": "BAT",
//            "is_erc20": true,
//            "is_erc721": false,
//            "is_erc1155": false,
//            "is_spam": false,
//            "decimals": 18,
//            "visible": true
//            ...
//          },
//          {
//            "address": "0x4729c2017edD1BaDf768595378c668955b537197",
//            "name": "MOR",
//            "symbol": "MOR",
//            "is_erc20": true,
//            "is_erc721": false,
//            "is_erc1155": false,
//            "is_spam": false,
//            "decimals": 18,
//            "visible": true
//            ...
//          },
//          ...
//        ]
//      },
//      "rinkeby": [
//        ...
//      ],
//      ...
//    },
//    "solana": {
//      "mainnet":  // network_id
//        [
//          {
//            "address": "",
//            "name": "Solana",
//            "symbol": "SOL",
//            "is_erc20": false,
//            "is_erc721": false,
//            "is_erc1155": false,
//            "is_spam": false,
//            "decimals": 9,
//            "visible": true
//            ...
//          },
//          ...
//        ]
//      ...
//    }
// }
//
//
namespace {

// T could be base::Value or const base::Value
template <typename T>
decltype(std::declval<T>().begin()) FindAsset(
    T* user_assets_list,
    const std::string& address,
    const std::string& token_id,
    bool check_token_id,
    const std::string& address_key = "address") {
  static_assert(std::is_same<std::decay_t<T>, base::Value::List>::value,
                "Only call with base::Value::List");

  auto iter =
      base::ranges::find_if(*user_assets_list, [&](const base::Value& value) {
        const auto* dict = value.GetIfDict();
        if (!dict) {
          return false;
        }
        const std::string* address_value = dict->FindString(address_key);
        bool found = address_value && *address_value == address;

        if (found && check_token_id) {
          const std::string* token_id_ptr = dict->FindString("token_id");
          found = token_id_ptr && *token_id_ptr == token_id;
        }

        return found;
      });

  return iter;
}

base::Value::Dict GetEthNativeAssetFromChain(
    const brave_wallet::mojom::NetworkInfoPtr& chain) {
  base::Value::Dict native_asset;
  native_asset.Set("address", "");
  native_asset.Set("name", chain->symbol_name);
  native_asset.Set("symbol", chain->symbol);
  native_asset.Set("is_erc20", false);
  native_asset.Set("is_erc721", false);
  native_asset.Set("is_erc1155", false);
  native_asset.Set("is_nft", false);
  native_asset.Set("is_spam", false);
  native_asset.Set("decimals", chain->decimals);
  native_asset.Set("visible", true);
  return native_asset;
}

bool ShouldCheckTokenId(const brave_wallet::mojom::BlockchainTokenPtr& token) {
  return token->is_erc721 || token->is_erc1155;
}

}  // namespace

namespace brave_wallet {

namespace {
bool AccountMatchesCoinAndChain(const mojom::AccountId& account_id,
                                mojom::CoinType coin,
                                const std::string& chain_id) {
  return base::Contains(GetSupportedKeyringsForNetwork(coin, chain_id),
                        account_id.keyring_id);
}

}  // namespace

struct PendingDecryptRequest {
  url::Origin origin;
  mojom::DecryptRequestPtr decrypt_request;
  mojom::EthereumProvider::RequestCallback decrypt_callback;
  base::Value decrypt_id;
};

struct PendingGetEncryptPublicKeyRequest {
  url::Origin origin;
  mojom::GetEncryptionPublicKeyRequestPtr encryption_public_key_request;
  mojom::EthereumProvider::RequestCallback encryption_public_key_callback;
  base::Value encryption_public_key_id;
};

BraveWalletService::BraveWalletService(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    std::unique_ptr<BraveWalletServiceDelegate> delegate,
    KeyringService* keyring_service,
    JsonRpcService* json_rpc_service,
    TxService* tx_service,
    PrefService* profile_prefs,
    PrefService* local_state)
    : delegate_(std::move(delegate)),
      keyring_service_(keyring_service),
      json_rpc_service_(json_rpc_service),
      tx_service_(tx_service),
      profile_prefs_(profile_prefs),
      brave_wallet_p3a_(this, keyring_service, profile_prefs, local_state),
      eth_allowance_manager_(
          std::make_unique<EthAllowanceManager>(json_rpc_service,
                                                keyring_service,
                                                profile_prefs)),
      weak_ptr_factory_(this) {
  simple_hash_client_ = std::make_unique<SimpleHashClient>(url_loader_factory);
  asset_discovery_manager_ = std::make_unique<AssetDiscoveryManager>(
      url_loader_factory, this, json_rpc_service, keyring_service,
      simple_hash_client_.get(), profile_prefs);

  CHECK(delegate_);
  delegate_->AddObserver(this);

  DCHECK(profile_prefs_);

  pref_change_registrar_.Init(profile_prefs_);
  pref_change_registrar_.Add(
      kBraveWalletLastUnlockTime,
      base::BindRepeating(&BraveWalletService::OnWalletUnlockPreferenceChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kDefaultEthereumWallet,
      base::BindRepeating(&BraveWalletService::OnDefaultEthereumWalletChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kDefaultSolanaWallet,
      base::BindRepeating(&BraveWalletService::OnDefaultSolanaWalletChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kDefaultBaseCurrency,
      base::BindRepeating(&BraveWalletService::OnDefaultBaseCurrencyChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kDefaultBaseCryptocurrency,
      base::BindRepeating(
          &BraveWalletService::OnDefaultBaseCryptocurrencyChanged,
          base::Unretained(this)));
  pref_change_registrar_.Add(
      kBraveWalletCustomNetworks,
      base::BindRepeating(&BraveWalletService::OnNetworkListChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kBraveWalletHiddenNetworks,
      base::BindRepeating(&BraveWalletService::OnNetworkListChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kBraveWalletNftDiscoveryEnabled,
      base::BindRepeating(&BraveWalletService::OnBraveWalletNftDiscoveryEnabled,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kBraveWalletSelectedNetworks,
      base::BindRepeating(&BraveWalletService::OnNetworkChanged,
                          weak_ptr_factory_.GetWeakPtr()));
}

BraveWalletService::BraveWalletService() : weak_ptr_factory_(this) {}

BraveWalletService::~BraveWalletService() = default;

mojo::PendingRemote<mojom::BraveWalletService>
BraveWalletService::MakeRemote() {
  mojo::PendingRemote<mojom::BraveWalletService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

// For unit tests
void BraveWalletService::RemovePrefListenersForTests() {
  pref_change_registrar_.RemoveAll();
}

void BraveWalletService::Bind(
    mojo::PendingReceiver<mojom::BraveWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

// Get the address to be used in user assets API.
// For EVM, convert the address to a checksum address.
// For Solana, verify if address is a base58 encoded address, if so, return it.
// static
absl::optional<std::string> BraveWalletService::GetUserAssetAddress(
    const std::string& address,
    mojom::CoinType coin,
    const std::string& chain_id) {
  if (address.empty()) {  // native asset
    return address;
  }

  if (coin == mojom::CoinType::ETH) {
    return GetChecksumAddress(address, chain_id);
  }

  if (coin == mojom::CoinType::SOL) {
    std::vector<uint8_t> bytes;
    if (!::brave_wallet::IsBase58EncodedSolanaPubkey(address)) {
      return absl::nullopt;
    }
    return address;
  }

  // TODO(spylogsster): Handle Filecoin here when if we need to support tokens
  // other than the native asset in the future.

  return absl::nullopt;
}

// static
absl::optional<std::string> BraveWalletService::GetChecksumAddress(
    const std::string& contract_address,
    const std::string& chain_id) {
  if (contract_address.empty()) {
    return "";
  }

  const auto eth_addr = EthAddress::FromHex(contract_address);
  if (eth_addr.IsEmpty()) {
    return absl::nullopt;
  }
  uint256_t chain;
  if (!HexValueToUint256(chain_id, &chain)) {
    return absl::nullopt;
  }

  return eth_addr.ToChecksumAddress(chain);
}

// static
std::vector<mojom::BlockchainTokenPtr> BraveWalletService::GetUserAssets(
    PrefService* profile_prefs) {
  std::vector<mojom::BlockchainTokenPtr> result;
  const auto& user_assets_dict = profile_prefs->GetDict(kBraveWalletUserAssets);
  for (auto coin_it : user_assets_dict) {
    auto coin = GetCoinTypeFromPrefKey(coin_it.first);
    if (!coin) {
      continue;
    }

    for (auto network_it : coin_it.second.GetDict()) {
      auto chain_id = GetChainId(profile_prefs, coin.value(), network_it.first);

      if (!chain_id) {
        continue;
      }

      for (const auto& item : network_it.second.GetList()) {
        const auto* token = item.GetIfDict();
        if (!token) {
          continue;
        }
        mojom::BlockchainTokenPtr token_ptr =
            ValueToBlockchainToken(*token, chain_id.value(), coin.value());
        if (token_ptr) {
          result.push_back(std::move(token_ptr));
        }
      }
    }
  }
  return result;
}

// static
std::vector<mojom::BlockchainTokenPtr> BraveWalletService::GetUserAssets(
    const std::string& chain_id,
    mojom::CoinType coin,
    PrefService* profile_prefs) {
  std::vector<mojom::BlockchainTokenPtr> result;
  const std::string network_id = GetNetworkId(profile_prefs, coin, chain_id);
  if (network_id.empty()) {
    return result;
  }

  const auto& user_assets_dict = profile_prefs->GetDict(kBraveWalletUserAssets);
  const auto* tokens = user_assets_dict.FindListByDottedPath(
      base::StrCat({GetPrefKeyForCoinType(coin), ".", network_id}));
  if (!tokens) {
    return result;
  }

  for (const auto& item : *tokens) {
    const auto* token = item.GetIfDict();
    if (!token) {
      continue;
    }

    mojom::BlockchainTokenPtr token_ptr =
        ValueToBlockchainToken(*token, chain_id, coin);
    if (token_ptr) {
      result.push_back(std::move(token_ptr));
    }
  }

  return result;
}

// static
bool BraveWalletService::AddUserAsset(mojom::BlockchainTokenPtr token,
                                      bool visible,
                                      PrefService* profile_prefs) {
  absl::optional<std::string> address = GetUserAssetAddress(
      token->contract_address, token->coin, token->chain_id);
  if (!address) {
    return false;
  }

  const std::string network_id =
      GetNetworkId(profile_prefs, token->coin, token->chain_id);
  if (network_id.empty()) {
    return false;
  }

  bool check_token_id = ShouldCheckTokenId(token);
  if (check_token_id) {
    uint256_t token_id_uint = 0;
    if (!HexValueToUint256(token->token_id, &token_id_uint)) {
      return false;
    }
  }

  ScopedDictPrefUpdate update(profile_prefs, kBraveWalletUserAssets);
  base::Value::Dict& user_assets_pref = update.Get();

  const auto path =
      base::StrCat({GetPrefKeyForCoinType(token->coin), ".", network_id});
  auto* user_assets_list = user_assets_pref.FindListByDottedPath(path);
  if (!user_assets_list) {
    user_assets_list =
        user_assets_pref.SetByDottedPath(path, base::Value::List())
            ->GetIfList();
  }
  DCHECK(user_assets_list);

  auto it =
      FindAsset(user_assets_list, *address, token->token_id, check_token_id);
  if (it != user_assets_list->end()) {
    return false;
  }

  base::Value::Dict value;
  value.Set("address", *address);
  value.Set("name", token->name);
  value.Set("symbol", token->symbol);
  value.Set("logo", token->logo);
  value.Set("is_erc20", token->is_erc20);
  value.Set("is_erc721", token->is_erc721);
  value.Set("is_erc1155", token->is_erc1155);
  value.Set("is_nft", token->is_nft);
  value.Set("is_spam", token->is_spam);
  value.Set("decimals", token->decimals);
  value.Set("visible", visible);
  value.Set("token_id", token->token_id);
  value.Set("coingecko_id", token->coingecko_id);

  user_assets_list->Append(std::move(value));

  return true;
}

void BraveWalletService::GetUserAssets(const std::string& chain_id,
                                       mojom::CoinType coin,
                                       GetUserAssetsCallback callback) {
  std::vector<mojom::BlockchainTokenPtr> result =
      GetUserAssets(chain_id, coin, profile_prefs_);
  std::move(callback).Run(std::move(result));
}

void BraveWalletService::GetAllUserAssets(GetUserAssetsCallback callback) {
  std::vector<mojom::BlockchainTokenPtr> result = GetUserAssets(profile_prefs_);
  std::move(callback).Run(std::move(result));
}

bool BraveWalletService::AddUserAsset(mojom::BlockchainTokenPtr token,
                                      bool visible) {
  mojom::BlockchainTokenPtr clone = token.Clone();
  bool result = BraveWalletService::AddUserAsset(std::move(token), visible,
                                                 profile_prefs_);

  if (result) {
    for (const auto& observer : token_observers_) {
      observer->OnTokenAdded(clone.Clone());
    }
  }
  return result;
}

void BraveWalletService::AddUserAsset(mojom::BlockchainTokenPtr token,
                                      AddUserAssetCallback callback) {
  const auto& interfaces_to_check = GetEthSupportedNftInterfaces();
  if (token->is_nft && token->coin == mojom::CoinType::ETH) {
    const std::string contract_address = token->contract_address;
    const std::string chain_id = token->chain_id;
    json_rpc_service_->GetEthNftStandard(
        contract_address, chain_id, interfaces_to_check,
        base::BindOnce(&BraveWalletService::OnGetEthNftStandard,
                       weak_ptr_factory_.GetWeakPtr(), std::move(token),
                       std::move(callback)));
    return;
  }

  std::move(callback).Run(AddUserAsset(std::move(token)));
}

void BraveWalletService::OnGetEthNftStandard(
    mojom::BlockchainTokenPtr token,
    AddUserAssetCallback callback,
    const absl::optional<std::string>& standard,
    mojom::ProviderError error,
    const std::string& error_message) {
  if (error != mojom::ProviderError::kSuccess || !standard) {
    std::move(callback).Run(false);
    return;
  }

  if (standard.value() == kERC721InterfaceId) {
    token->is_erc721 = true;
    token->is_erc1155 = false;
  } else if (standard.value() == kERC1155InterfaceId) {
    token->is_erc721 = false;
    token->is_erc1155 = true;
  } else {
    // Unsupported NFT standard.
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(AddUserAsset(std::move(token)));
}

void BraveWalletService::RemoveUserAsset(mojom::BlockchainTokenPtr token,
                                         RemoveUserAssetCallback callback) {
  std::move(callback).Run(RemoveUserAsset(std::move(token)));
}

bool BraveWalletService::RemoveUserAsset(mojom::BlockchainTokenPtr token) {
  absl::optional<std::string> address = GetUserAssetAddress(
      token->contract_address, token->coin, token->chain_id);
  if (!address) {
    return false;
  }

  const std::string network_id =
      GetNetworkId(profile_prefs_, token->coin, token->chain_id);
  if (network_id.empty()) {
    return false;
  }

  ScopedDictPrefUpdate update(profile_prefs_, kBraveWalletUserAssets);

  auto* user_assets_list = update->FindListByDottedPath(
      base::StrCat({GetPrefKeyForCoinType(token->coin), ".", network_id}));
  if (!user_assets_list) {
    return false;
  }

  auto it = FindAsset(user_assets_list, *address, token->token_id,
                      ShouldCheckTokenId(token));
  if (it != user_assets_list->end()) {
    user_assets_list->erase(it);
  }

  for (const auto& observer : token_observers_) {
    observer->OnTokenRemoved(token.Clone());
  }

  return true;
}

void BraveWalletService::SetUserAssetVisible(
    mojom::BlockchainTokenPtr token,
    bool visible,
    SetUserAssetVisibleCallback callback) {
  std::move(callback).Run(SetUserAssetVisible(std::move(token), visible));
}

bool BraveWalletService::SetUserAssetVisible(mojom::BlockchainTokenPtr token,
                                             bool visible) {
  DCHECK(token);

  absl::optional<std::string> address = GetUserAssetAddress(
      token->contract_address, token->coin, token->chain_id);
  if (!address) {
    return false;
  }

  const std::string network_id =
      GetNetworkId(profile_prefs_, token->coin, token->chain_id);
  if (network_id.empty()) {
    return false;
  }

  ScopedDictPrefUpdate update(profile_prefs_, kBraveWalletUserAssets);
  auto* user_assets_list = update->FindListByDottedPath(
      base::StrCat({GetPrefKeyForCoinType(token->coin), ".", network_id}));
  if (!user_assets_list) {
    return false;
  }

  auto it = FindAsset(user_assets_list, *address, token->token_id,
                      ShouldCheckTokenId(token));
  if (it == user_assets_list->end()) {
    return false;
  }

  it->GetDict().Set("visible", visible);
  return true;
}

void BraveWalletService::SetAssetSpamStatus(
    mojom::BlockchainTokenPtr token,
    bool is_spam,
    SetAssetSpamStatusCallback callback) {
  std::move(callback).Run(SetAssetSpamStatus(std::move(token), is_spam));
}

bool BraveWalletService::SetAssetSpamStatus(mojom::BlockchainTokenPtr token,
                                            bool is_spam) {
  DCHECK(token);

  absl::optional<std::string> address = GetUserAssetAddress(
      token->contract_address, token->coin, token->chain_id);
  if (!address) {
    return false;
  }

  const std::string network_id =
      GetNetworkId(profile_prefs_, token->coin, token->chain_id);
  if (network_id.empty()) {
    return false;
  }

  ScopedDictPrefUpdate update(profile_prefs_, kBraveWalletUserAssets);
  auto* user_assets_list = update->FindListByDottedPath(
      base::StrCat({GetPrefKeyForCoinType(token->coin), ".", network_id}));
  if (!user_assets_list) {
    return false;
  }

  auto it = FindAsset(user_assets_list, *address, token->token_id,
                      ShouldCheckTokenId(token));
  if (it == user_assets_list->end()) {
    // If the asset is not in the user's asset list, we automatically add it and
    // set the spam status.
    token->is_spam = is_spam;
    token->visible = !is_spam;
    return AddUserAsset(token.Clone(), token->visible);
  }

  it->GetDict().Set("is_spam", is_spam);

  // Marking a token as spam makes it not visible and vice-versa
  it->GetDict().Set("visible", !is_spam);

  return true;
}

mojom::BlockchainTokenPtr BraveWalletService::GetUserAsset(
    const std::string& raw_address,
    const std::string& token_id,
    bool check_token_id,
    const std::string& chain_id,
    mojom::CoinType coin) {
  absl::optional<std::string> address =
      GetUserAssetAddress(raw_address, coin, chain_id);
  if (!address) {
    return nullptr;
  }

  const std::string network_id = GetNetworkId(profile_prefs_, coin, chain_id);
  if (network_id.empty()) {
    return nullptr;
  }

  const auto& user_assets_dict =
      profile_prefs_->GetDict(kBraveWalletUserAssets);
  const auto* user_assets_list = user_assets_dict.FindListByDottedPath(
      base::StrCat({GetPrefKeyForCoinType(coin), ".", network_id}));
  if (!user_assets_list) {
    return nullptr;
  }

  auto it = FindAsset(user_assets_list, *address, token_id, check_token_id);
  if (it == user_assets_list->end()) {
    return nullptr;
  }

  return ValueToBlockchainToken(it->GetDict(), chain_id, coin);
}

void BraveWalletService::IsExternalWalletInstalled(
    mojom::ExternalWalletType type,
    IsExternalWalletInstalledCallback callback) {
  delegate_->IsExternalWalletInstalled(type, std::move(callback));
}

void BraveWalletService::IsExternalWalletInitialized(
    mojom::ExternalWalletType type,
    IsExternalWalletInitializedCallback callback) {
  delegate_->IsExternalWalletInitialized(type, std::move(callback));
}

void BraveWalletService::ImportFromExternalWallet(
    mojom::ExternalWalletType type,
    const std::string& password,
    const std::string& new_password,
    ImportFromExternalWalletCallback callback) {
  delegate_->GetImportInfoFromExternalWallet(
      type, password,
      base::BindOnce(&BraveWalletService::OnGetImportInfo,
                     weak_ptr_factory_.GetWeakPtr(), new_password,
                     std::move(callback)));
}

void BraveWalletService::GetDefaultEthereumWallet(
    GetDefaultEthereumWalletCallback callback) {
  std::move(callback).Run(
      ::brave_wallet::GetDefaultEthereumWallet(profile_prefs_));
}

void BraveWalletService::GetDefaultSolanaWallet(
    GetDefaultSolanaWalletCallback callback) {
  std::move(callback).Run(
      ::brave_wallet::GetDefaultSolanaWallet(profile_prefs_));
}

void BraveWalletService::SetDefaultEthereumWallet(
    mojom::DefaultWallet default_wallet) {
  auto old_default_wallet =
      ::brave_wallet::GetDefaultEthereumWallet(profile_prefs_);
  if (old_default_wallet != default_wallet) {
    ::brave_wallet::SetDefaultEthereumWallet(profile_prefs_, default_wallet);
  }
}

void BraveWalletService::SetDefaultSolanaWallet(
    mojom::DefaultWallet default_wallet) {
  auto old_default_wallet =
      ::brave_wallet::GetDefaultSolanaWallet(profile_prefs_);
  if (old_default_wallet != default_wallet) {
    ::brave_wallet::SetDefaultSolanaWallet(profile_prefs_, default_wallet);
  }
}

void BraveWalletService::GetDefaultBaseCurrency(
    GetDefaultBaseCurrencyCallback callback) {
  std::move(callback).Run(
      ::brave_wallet::GetDefaultBaseCurrency(profile_prefs_));
}

void BraveWalletService::SetDefaultBaseCurrency(const std::string& currency) {
  auto old_default_currency =
      ::brave_wallet::GetDefaultBaseCurrency(profile_prefs_);
  if (old_default_currency != currency) {
    ::brave_wallet::SetDefaultBaseCurrency(profile_prefs_, currency);
  }
}

void BraveWalletService::GetDefaultBaseCryptocurrency(
    GetDefaultBaseCryptocurrencyCallback callback) {
  std::move(callback).Run(
      ::brave_wallet::GetDefaultBaseCryptocurrency(profile_prefs_));
}

void BraveWalletService::SetDefaultBaseCryptocurrency(
    const std::string& cryptocurrency) {
  auto old_default_cryptocurrency =
      ::brave_wallet::GetDefaultBaseCryptocurrency(profile_prefs_);
  if (old_default_cryptocurrency != cryptocurrency) {
    ::brave_wallet::SetDefaultBaseCryptocurrency(profile_prefs_,
                                                 cryptocurrency);
  }
}

void BraveWalletService::EnsureSelectedAccountForChain(
    mojom::CoinType coin,
    const std::string& chain_id,
    EnsureSelectedAccountForChainCallback callback) {
  std::move(callback).Run(EnsureSelectedAccountForChainSync(coin, chain_id));
}

mojom::AccountIdPtr BraveWalletService::EnsureSelectedAccountForChainSync(
    mojom::CoinType coin,
    const std::string& chain_id) {
  auto all_accounts = keyring_service_->GetAllAccountsSync();
  // Selected account already matches coin/chain_id pair, just return.
  if (AccountMatchesCoinAndChain(*all_accounts->selected_account->account_id,
                                 coin, chain_id)) {
    return all_accounts->selected_account->account_id.Clone();
  }

  mojom::AccountIdPtr acc_to_select;

  // Prefer currently dapp selected account if available. This matches legacy
  // behaior.
  // TODO(apaymyshev): implement account selection history pref to prefer
  // picking recently used matching account in a general way.
  if (coin == mojom::CoinType::ETH && all_accounts->eth_dapp_selected_account) {
    acc_to_select =
        all_accounts->eth_dapp_selected_account->account_id->Clone();
    DCHECK(AccountMatchesCoinAndChain(*acc_to_select, coin, chain_id));
  } else if (coin == mojom::CoinType::SOL &&
             all_accounts->sol_dapp_selected_account) {
    acc_to_select =
        all_accounts->sol_dapp_selected_account->account_id->Clone();
    DCHECK(AccountMatchesCoinAndChain(*acc_to_select, coin, chain_id));
  }

  if (!acc_to_select) {
    // Find any account that matches coin/chain_id
    for (auto& acc : all_accounts->accounts) {
      if (AccountMatchesCoinAndChain(*acc->account_id, coin, chain_id)) {
        acc_to_select = acc->account_id.Clone();
        break;
      }
    }
  }

  if (!acc_to_select) {
    return {};
  }

  keyring_service_->SetSelectedAccountSync(acc_to_select.Clone());
  return acc_to_select.Clone();
}

void BraveWalletService::GetNetworkForSelectedAccountOnActiveOrigin(
    GetNetworkForSelectedAccountOnActiveOriginCallback callback) {
  std::move(callback).Run(GetNetworkForSelectedAccountOnActiveOriginSync());
}

mojom::NetworkInfoPtr
BraveWalletService::GetNetworkForSelectedAccountOnActiveOriginSync() {
  auto selected_account = keyring_service_->GetSelectedWalletAccount();
  // This may happen in tests.
  if (!selected_account) {
    return {};
  }

  if (selected_account->account_id->coin == mojom::CoinType::BTC) {
    if (IsBitcoinMainnetKeyring(selected_account->account_id->keyring_id)) {
      return GetChain(profile_prefs_, mojom::kBitcoinMainnet,
                      mojom::CoinType::BTC);
    } else {
      return GetChain(profile_prefs_, mojom::kBitcoinTestnet,
                      mojom::CoinType::BTC);
    }
  }

  return json_rpc_service_->GetNetworkSync(selected_account->account_id->coin,
                                           delegate_->GetActiveOrigin());
}

void BraveWalletService::SetNetworkForSelectedAccountOnActiveOrigin(
    const std::string& chain_id,
    SetNetworkForSelectedAccountOnActiveOriginCallback callback) {
  auto selected_account = keyring_service_->GetSelectedWalletAccount();
  CHECK(selected_account);
  if (!AccountMatchesCoinAndChain(*selected_account->account_id,
                                  selected_account->account_id->coin,
                                  chain_id)) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(json_rpc_service_->SetNetwork(
      chain_id, selected_account->account_id->coin,
      delegate_->GetActiveOrigin()));
}

bool BraveWalletService::HasPendingDecryptRequestForOrigin(
    const url::Origin& origin) const {
  return base::ranges::any_of(pending_decrypt_requests_, [origin](auto& req) {
    return req.second.origin == origin;
  });
}

bool BraveWalletService::HasPendingGetEncryptionPublicKeyRequestForOrigin(
    const url::Origin& origin) const {
  return base::ranges::any_of(
      pending_get_encryption_public_key_requests_,
      [origin](auto& req) { return req.second.origin == origin; });
}

void BraveWalletService::OnDefaultEthereumWalletChanged() {
  auto default_wallet =
      ::brave_wallet::GetDefaultEthereumWallet(profile_prefs_);
  for (const auto& observer : observers_) {
    observer->OnDefaultEthereumWalletChanged(default_wallet);
  }
}

void BraveWalletService::OnDefaultSolanaWalletChanged() {
  auto default_wallet = ::brave_wallet::GetDefaultSolanaWallet(profile_prefs_);
  for (const auto& observer : observers_) {
    observer->OnDefaultSolanaWalletChanged(default_wallet);
  }
}

void BraveWalletService::OnDefaultBaseCurrencyChanged() {
  auto value = ::brave_wallet::GetDefaultBaseCurrency(profile_prefs_);
  for (const auto& observer : observers_) {
    observer->OnDefaultBaseCurrencyChanged(value);
  }
}

void BraveWalletService::OnDefaultBaseCryptocurrencyChanged() {
  auto value = ::brave_wallet::GetDefaultBaseCryptocurrency(profile_prefs_);
  for (const auto& observer : observers_) {
    observer->OnDefaultBaseCryptocurrencyChanged(value);
  }
}

void BraveWalletService::OnNetworkListChanged() {
  for (const auto& observer : observers_) {
    observer->OnNetworkListChanged();
  }
}

void BraveWalletService::OnBraveWalletNftDiscoveryEnabled() {
  if (profile_prefs_->GetBoolean(kBraveWalletNftDiscoveryEnabled)) {
    DiscoverAssetsOnAllSupportedChains(true);
  }
}

void BraveWalletService::AddPermission(mojom::AccountIdPtr account_id,
                                       AddPermissionCallback callback) {
  auto origin = delegate_->GetActiveOrigin();
  if (!origin) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(
      delegate_->AddPermission(account_id->coin, *origin, account_id->address));
}

void BraveWalletService::HasPermission(
    std::vector<mojom::AccountIdPtr> accounts,
    HasPermissionCallback callback) {
  auto origin = delegate_->GetActiveOrigin();
  if (!origin) {
    std::move(callback).Run(false, {});
    return;
  }

  std::vector<mojom::AccountIdPtr> result;
  for (auto& account_id : accounts) {
    if (delegate_->HasPermission(account_id->coin, *origin,
                                 account_id->address)) {
      result.push_back(account_id->Clone());
    }
  }
  std::move(callback).Run(true, std::move(result));
}

void BraveWalletService::ResetPermission(mojom::AccountIdPtr account_id,
                                         ResetPermissionCallback callback) {
  auto origin = delegate_->GetActiveOrigin();
  if (!origin) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(delegate_->ResetPermission(account_id->coin, *origin,
                                                     account_id->address));
}

void BraveWalletService::IsPermissionDenied(
    mojom::CoinType coin,
    IsPermissionDeniedCallback callback) {
  auto origin = delegate_->GetActiveOrigin();
  if (!origin) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(delegate_->IsPermissionDenied(coin, *origin));
}

void BraveWalletService::GetWebSitesWithPermission(
    mojom::CoinType coin,
    GetWebSitesWithPermissionCallback callback) {
  delegate_->GetWebSitesWithPermission(coin, std::move(callback));
}

void BraveWalletService::ResetWebSitePermission(
    mojom::CoinType coin,
    const std::string& formed_website,
    ResetWebSitePermissionCallback callback) {
  delegate_->ResetWebSitePermission(coin, formed_website, std::move(callback));
}

// static
void BraveWalletService::MigrateUserAssetEthContractAddress(
    PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletUserAssetEthContractAddressMigrated)) {
    return;
  }

  if (!prefs->HasPrefPath(kBraveWalletUserAssetsDeprecated)) {
    prefs->SetBoolean(kBraveWalletUserAssetEthContractAddressMigrated, true);
    return;
  }

  ScopedDictPrefUpdate update(prefs, kBraveWalletUserAssetsDeprecated);
  auto& user_assets_pref = update.Get();

  for (auto user_asset_list : user_assets_pref) {
    auto& item = user_asset_list.second.GetList();
    auto it = FindAsset(&item, "eth", "", false, "contract_address");
    if (it == item.end()) {
      continue;
    }

    auto* asset = it->GetIfDict();
    if (asset) {
      const std::string* contract_address =
          asset->FindString("contract_address");
      if (contract_address && *contract_address == "eth") {
        asset->Set("contract_address", "");
        break;
      }
    }
  }

  prefs->SetBoolean(kBraveWalletUserAssetEthContractAddressMigrated, true);
}

// static
void BraveWalletService::MigrateMultichainUserAssets(PrefService* prefs) {
  if (!prefs->HasPrefPath(kBraveWalletUserAssetsDeprecated)) {
    return;
  }

  auto eth_user_assets =
      prefs->GetDict(kBraveWalletUserAssetsDeprecated).Clone();

  // Update contract_address key to address.
  for (auto user_asset_list : eth_user_assets) {
    for (auto& item : user_asset_list.second.GetList()) {
      auto& asset = item.GetDict();
      const std::string* address = asset.FindString("contract_address");
      if (address) {
        asset.Set("address", *address);
        asset.Remove("contract_address");
      }
    }
  }

  base::Value::Dict new_user_assets;
  new_user_assets.Set(kEthereumPrefKey, std::move(eth_user_assets));
  new_user_assets.Set(kSolanaPrefKey, GetDefaultSolanaAssets());
  new_user_assets.Set(kFilecoinPrefKey, GetDefaultFilecoinAssets());

  prefs->Set(kBraveWalletUserAssets, base::Value(std::move(new_user_assets)));
  prefs->ClearPref(kBraveWalletUserAssetsDeprecated);
}

// static
void BraveWalletService::MigrateUserAssetsAddPreloadingNetworks(
    PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletUserAssetsAddPreloadingNetworksMigrated)) {
    return;
  }

  if (!prefs->HasPrefPath(kBraveWalletUserAssets)) {
    prefs->SetBoolean(kBraveWalletUserAssetsAddPreloadingNetworksMigrated,
                      true);
    return;
  }

  ScopedDictPrefUpdate update(prefs, kBraveWalletUserAssets);
  auto& user_assets_pref = update.Get();

  // For each user asset list in ethereum known chains, check if it has the
  // native token (address is empty) in the list already, if not, insert a
  // native asset at the beginning based on the network info.
  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::ETH)) {
    const std::string network_id = GetKnownEthNetworkId(chain->chain_id);
    DCHECK(!network_id.empty());
    const auto path = base::StrCat({kEthereumPrefKey, ".", network_id});
    auto* user_assets_list = user_assets_pref.FindListByDottedPath(path);
    if (!user_assets_list) {
      user_assets_list =
          user_assets_pref.SetByDottedPath(path, base::Value::List())
              ->GetIfList();
      user_assets_list->Append(GetEthNativeAssetFromChain(chain));
      continue;
    }

    auto it = FindAsset(user_assets_list, "", "", false);
    if (it == user_assets_list->end()) {
      user_assets_list->Insert(user_assets_list->begin(),
                               base::Value(GetEthNativeAssetFromChain(chain)));
    }
  }

  prefs->SetBoolean(kBraveWalletUserAssetsAddPreloadingNetworksMigrated, true);
  if (prefs->HasPrefPath(
          kBraveWalletUserAssetsAddPreloadingNetworksMigratedDeprecated)) {
    prefs->ClearPref(
        kBraveWalletUserAssetsAddPreloadingNetworksMigratedDeprecated);
  }
  if (prefs->HasPrefPath(
          kBraveWalletUserAssetsAddPreloadingNetworksMigratedDeprecated2)) {
    prefs->ClearPref(
        kBraveWalletUserAssetsAddPreloadingNetworksMigratedDeprecated2);
  }
}

// static
void BraveWalletService::MigrateUserAssetsAddIsNFT(PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletUserAssetsAddIsNFTMigrated)) {
    return;
  }

  if (!prefs->HasPrefPath(kBraveWalletUserAssets)) {
    prefs->SetBoolean(kBraveWalletUserAssetsAddIsNFTMigrated, true);
    return;
  }

  ScopedDictPrefUpdate update(prefs, kBraveWalletUserAssets);
  base::Value::Dict& user_assets_pref = update.Get();

  for (auto user_asset_dict_per_cointype : user_assets_pref) {
    if (!user_asset_dict_per_cointype.second.is_dict()) {
      continue;
    }
    for (auto user_asset_list_per_chain :
         user_asset_dict_per_cointype.second.GetDict()) {
      if (!user_asset_list_per_chain.second.is_list()) {
        continue;
      }
      for (auto& user_asset : user_asset_list_per_chain.second.GetList()) {
        auto* asset = user_asset.GetIfDict();
        if (!asset) {
          continue;
        }

        auto is_erc721 = asset->FindBool("is_erc721");
        if (is_erc721 && *is_erc721 == true) {
          asset->Set("is_nft", true);
        } else {
          asset->Set("is_nft", false);
        }
      }
    }
  }
  prefs->SetBoolean(kBraveWalletUserAssetsAddIsNFTMigrated, true);
}

// static
void BraveWalletService::MigrateHiddenNetworks(PrefService* prefs) {
  auto previous_version_code =
      prefs->GetInteger(kBraveWalletDefaultHiddenNetworksVersion);
  if (previous_version_code >= 1) {
    return;
  }
  {
    // Default hidden networks
    ScopedDictPrefUpdate update(prefs, kBraveWalletHiddenNetworks);
    auto& hidden_networks_pref = update.Get();
    base::Value::List* hidden_eth_networks =
        hidden_networks_pref.EnsureList(kEthereumPrefKey);

    auto value = base::Value(mojom::kFilecoinEthereumTestnetChainId);
    if (std::find_if(hidden_eth_networks->begin(), hidden_eth_networks->end(),
                     [&value](auto& v) { return value == v; }) ==
        hidden_eth_networks->end()) {
      hidden_eth_networks->Append(std::move(value));
    }
  }

  prefs->SetInteger(kBraveWalletDefaultHiddenNetworksVersion, 1);
}

void BraveWalletService::MigrateUserAssetsAddIsERC1155(PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletUserAssetsAddIsERC1155Migrated)) {
    return;
  }

  if (!prefs->HasPrefPath(kBraveWalletUserAssets)) {
    prefs->SetBoolean(kBraveWalletUserAssetsAddIsERC1155Migrated, true);
    return;
  }

  ScopedDictPrefUpdate update(prefs, kBraveWalletUserAssets);
  base::Value::Dict& user_assets_pref = update.Get();

  for (auto user_asset_dict_per_cointype : user_assets_pref) {
    if (!user_asset_dict_per_cointype.second.is_dict()) {
      continue;
    }
    for (auto user_asset_list_per_chain :
         user_asset_dict_per_cointype.second.GetDict()) {
      if (!user_asset_list_per_chain.second.is_list()) {
        continue;
      }
      for (auto& user_asset : user_asset_list_per_chain.second.GetList()) {
        auto* asset = user_asset.GetIfDict();
        if (!asset) {
          continue;
        }
        if (!asset->FindBool("is_erc1155")) {
          asset->Set("is_erc1155", false);
        }
      }
    }
  }
  prefs->SetBoolean(kBraveWalletUserAssetsAddIsERC1155Migrated, true);
}

void BraveWalletService::MigrateUserAssetsAddIsSpam(PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletUserAssetsAddIsSpamMigrated)) {
    return;
  }

  if (!prefs->HasPrefPath(kBraveWalletUserAssets)) {
    prefs->SetBoolean(kBraveWalletUserAssetsAddIsSpamMigrated, true);
    return;
  }

  ScopedDictPrefUpdate update(prefs, kBraveWalletUserAssets);
  base::Value::Dict& user_assets_pref = update.Get();

  for (auto user_asset_dict_per_cointype : user_assets_pref) {
    if (!user_asset_dict_per_cointype.second.is_dict()) {
      continue;
    }
    for (auto user_asset_list_per_chain :
         user_asset_dict_per_cointype.second.GetDict()) {
      if (!user_asset_list_per_chain.second.is_list()) {
        continue;
      }
      for (auto& user_asset : user_asset_list_per_chain.second.GetList()) {
        auto* asset = user_asset.GetIfDict();
        if (!asset) {
          continue;
        }
        if (!asset->FindBool("is_spam")) {
          asset->Set("is_spam", false);
        }
      }
    }
  }
  prefs->SetBoolean(kBraveWalletUserAssetsAddIsSpamMigrated, true);
}

bool ShouldMigrateRemovedPreloadedNetwork(PrefService* prefs,
                                          mojom::CoinType coin,
                                          const std::string& chain_id) {
  if (CustomChainExists(prefs, chain_id, coin)) {
    return false;
  }

  const auto& selected_networks =
      prefs->GetDict(kBraveWalletSelectedNetworksPerOrigin);

  const auto* coin_dict =
      selected_networks.FindDict(GetPrefKeyForCoinType(coin));
  if (!coin_dict) {
    return false;
  }

  for (auto origin : *coin_dict) {
    const auto* chain_id_each = origin.second.GetIfString();
    if (!chain_id_each) {
      continue;
    }

    if (base::ToLowerASCII(*chain_id_each) == chain_id) {
      return true;
    }
  }

  return GetCurrentChainId(prefs, coin, absl::nullopt) == chain_id;
}

void BraveWalletService::MigrateFantomMainnetAsCustomNetwork(
    PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated)) {
    return;
  }

  if (ShouldMigrateRemovedPreloadedNetwork(prefs, mojom::CoinType::ETH,
                                           mojom::kFantomMainnetChainId)) {
    AddCustomNetwork(
        prefs, {mojom::kFantomMainnetChainId,
                "Fantom Opera",
                {"https://ftmscan.com"},
                {},
                0,
                {GURL("https://rpc.ftm.tools")},
                "FTM",
                "Fantom",
                18,
                mojom::CoinType::ETH,
                GetSupportedKeyringsForNetwork(mojom::CoinType::ETH,
                                               mojom::kFantomMainnetChainId),
                true});
  }

  prefs->SetBoolean(kBraveWalletCustomNetworksFantomMainnetMigrated, true);
}

// static
base::Value::Dict BraveWalletService::GetDefaultEthereumAssets() {
  base::Value::Dict user_assets;

  base::Value::Dict bat;
  bat.Set("address", "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  bat.Set("name", "Basic Attention Token");
  bat.Set("symbol", "BAT");
  bat.Set("is_erc20", true);
  bat.Set("is_erc721", false);
  bat.Set("is_erc1155", false);
  bat.Set("is_nft", false);
  bat.Set("is_spam", false);
  bat.Set("decimals", 18);
  bat.Set("visible", true);
  bat.Set("logo", "bat.png");

  // Show ETH and BAT by default for mainnet, and the native token for other
  // known networks.
  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::ETH)) {
    const std::string network_id = GetKnownEthNetworkId(chain->chain_id);
    DCHECK(!network_id.empty());
    base::Value::List user_assets_list;
    user_assets_list.Append(GetEthNativeAssetFromChain(chain));
    if (chain->chain_id == mojom::kMainnetChainId) {
      user_assets_list.Append(bat.Clone());
    }

    user_assets.Set(network_id, std::move(user_assets_list));
  }

  return user_assets;
}

// static
base::Value::Dict BraveWalletService::GetDefaultSolanaAssets() {
  base::Value::Dict user_assets;

  base::Value::Dict sol;
  sol.Set("address", "");
  sol.Set("name", "Solana");
  sol.Set("symbol", "SOL");
  sol.Set("decimals", 9);
  sol.Set("is_erc20", false);
  sol.Set("is_erc721", false);
  sol.Set("is_erc1155", false);
  sol.Set("is_spam", false);
  sol.Set("is_nft", false);
  sol.Set("visible", true);
  sol.Set("logo", "sol.png");

  std::vector<std::string> network_ids = GetAllKnownSolNetworkIds();
  for (const auto& network_id : network_ids) {
    base::Value::List user_assets_list;
    user_assets_list.Append(sol.Clone());
    user_assets.Set(network_id, std::move(user_assets_list));
  }

  return user_assets;
}

// static
base::Value::Dict BraveWalletService::GetDefaultFilecoinAssets() {
  base::Value::Dict user_assets;

  base::Value::Dict fil;
  fil.Set("address", "");
  fil.Set("name", "Filecoin");
  fil.Set("symbol", "FIL");
  fil.Set("decimals", 18);
  fil.Set("is_erc20", false);
  fil.Set("is_erc721", false);
  fil.Set("is_erc1155", false);
  fil.Set("is_spam", false);
  fil.Set("is_nft", false);
  fil.Set("visible", true);
  fil.Set("logo", "fil.png");

  std::vector<std::string> network_ids = GetAllKnownFilNetworkIds();
  for (const auto& network_id : network_ids) {
    base::Value::List user_assets_list;
    user_assets_list.Append(fil.Clone());
    user_assets.Set(network_id, std::move(user_assets_list));
  }

  return user_assets;
}

// static
base::Value::Dict BraveWalletService::GetDefaultBitcoinAssets() {
  base::Value::Dict user_assets;

  base::Value::Dict btc;
  btc.Set("address", "");
  btc.Set("name", "Bitcoin");
  btc.Set("decimals", 8);
  btc.Set("is_erc20", false);
  btc.Set("is_erc721", false);
  btc.Set("is_erc1155", false);
  btc.Set("is_spam", false);
  btc.Set("is_nft", false);
  btc.Set("visible", true);
  btc.Set("logo", "btc.png");

  std::vector<std::string> network_ids = GetAllKnownBtcNetworkIds();
  for (const auto& network_id : network_ids) {
    base::Value::List user_assets_list;
    auto asset = btc.Clone();
    if (network_id == "mainnet") {
      asset.Set("symbol", "BTC");
      asset.Set("coingecko_id", "btc");
    } else if (network_id == "testnet") {
      asset.Set("symbol", "tBTC");
      asset.Set("coingecko_id", "btc");
    } else {
      NOTREACHED();
    }
    user_assets_list.Append(std::move(asset));
    user_assets.Set(network_id, std::move(user_assets_list));
  }

  return user_assets;
}

void BraveWalletService::OnWalletUnlockPreferenceChanged(
    const std::string& pref_name) {
  brave_wallet_p3a_.ReportUsage(true);
}

BraveWalletP3A* BraveWalletService::GetBraveWalletP3A() {
  return &brave_wallet_p3a_;
}

void BraveWalletService::GetActiveOrigin(GetActiveOriginCallback callback) {
  std::move(callback).Run(GetActiveOriginSync());
}

mojom::OriginInfoPtr BraveWalletService::GetActiveOriginSync() {
  return MakeOriginInfo(delegate_->GetActiveOrigin().value_or(url::Origin()));
}

void BraveWalletService::GetPendingSignMessageRequests(
    GetPendingSignMessageRequestsCallback callback) {
  std::vector<mojom::SignMessageRequestPtr> requests;
  if (sign_message_requests_.empty()) {
    std::move(callback).Run(std::move(requests));
    return;
  }

  for (const auto& request : sign_message_requests_) {
    requests.push_back(request.Clone());
  }

  std::move(callback).Run(std::move(requests));
}

void BraveWalletService::NotifySignMessageRequestProcessed(
    bool approved,
    int id,
    mojom::ByteArrayStringUnionPtr signature,
    const absl::optional<std::string>& error) {
  if (sign_message_requests_.empty() ||
      sign_message_requests_.front()->id != id) {
    VLOG(1) << "id: " << id << " is not expected, should be "
            << sign_message_requests_.front()->id;
    return;
  }
  auto callback = std::move(sign_message_callbacks_.front());
  sign_message_requests_.pop_front();
  sign_message_callbacks_.pop_front();

  std::move(callback).Run(approved, std::move(signature), error);
}

void BraveWalletService::GetPendingSignTransactionRequests(
    GetPendingSignTransactionRequestsCallback callback) {
  std::vector<mojom::SignTransactionRequestPtr> requests;
  if (sign_transaction_requests_.empty()) {
    std::move(callback).Run(std::move(requests));
    return;
  }

  for (const auto& request : sign_transaction_requests_) {
    requests.push_back(request.Clone());
  }

  std::move(callback).Run(std::move(requests));
}

void BraveWalletService::NotifySignTransactionRequestProcessed(
    bool approved,
    int id,
    mojom::ByteArrayStringUnionPtr signature,
    const absl::optional<std::string>& error) {
  if (sign_transaction_requests_.empty() ||
      sign_transaction_requests_.front()->id != id) {
    VLOG(1) << "id: " << id << " is not expected, should be "
            << sign_transaction_requests_.front()->id;
    return;
  }
  auto callback = std::move(sign_transaction_callbacks_.front());
  sign_transaction_requests_.pop_front();
  sign_transaction_callbacks_.pop_front();

  std::move(callback).Run(approved, std::move(signature), error);
}

void BraveWalletService::GetPendingSignAllTransactionsRequests(
    GetPendingSignAllTransactionsRequestsCallback callback) {
  std::vector<mojom::SignAllTransactionsRequestPtr> requests;
  if (sign_all_transactions_requests_.empty()) {
    std::move(callback).Run(std::move(requests));
    return;
  }

  for (const auto& request : sign_all_transactions_requests_) {
    requests.push_back(request.Clone());
  }

  std::move(callback).Run(std::move(requests));
}

void BraveWalletService::NotifySignAllTransactionsRequestProcessed(
    bool approved,
    int id,
    absl::optional<std::vector<mojom::ByteArrayStringUnionPtr>> signatures,
    const absl::optional<std::string>& error) {
  if (sign_all_transactions_requests_.empty() ||
      sign_all_transactions_requests_.front()->id != id) {
    VLOG(1) << "id: " << id << " is not expected, should be "
            << sign_all_transactions_requests_.front()->id;
    return;
  }
  auto callback = std::move(sign_all_transactions_callbacks_.front());
  sign_all_transactions_requests_.pop_front();
  sign_all_transactions_callbacks_.pop_front();

  std::move(callback).Run(approved, std::move(signatures), error);
}

void BraveWalletService::AddObserver(
    ::mojo::PendingRemote<mojom::BraveWalletServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

void BraveWalletService::AddTokenObserver(
    ::mojo::PendingRemote<mojom::BraveWalletServiceTokenObserver> observer) {
  token_observers_.Add(std::move(observer));
}

void BraveWalletService::OnActiveOriginChanged(
    const mojom::OriginInfoPtr& origin_info) {
  for (const auto& observer : observers_) {
    observer->OnActiveOriginChanged(origin_info.Clone());
  }
}

void BraveWalletService::OnDiscoverAssetsStarted() {
  for (const auto& observer : observers_) {
    observer->OnDiscoverAssetsStarted();
  }
}

void BraveWalletService::OnDiscoverAssetsCompleted(
    std::vector<mojom::BlockchainTokenPtr> discovered_assets) {
  for (const auto& observer : observers_) {
    std::vector<mojom::BlockchainTokenPtr> discovered_assets_copy;
    for (auto& asset : discovered_assets) {
      discovered_assets_copy.push_back(asset.Clone());
    }
    observer->OnDiscoverAssetsCompleted(std::move(discovered_assets_copy));
  }
}

void BraveWalletService::OnGetImportInfo(
    const std::string& new_password,
    base::OnceCallback<void(bool, const absl::optional<std::string>&)> callback,
    bool result,
    ImportInfo info,
    ImportError error) {
  if (!result) {
    switch (error) {
      case ImportError::kJsonError:
        std::move(callback).Run(false, l10n_util::GetStringUTF8(
                                           IDS_BRAVE_WALLET_IMPORT_JSON_ERROR));
        break;
      case ImportError::kPasswordError:
        std::move(callback).Run(
            false,
            l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_IMPORT_PASSWORD_ERROR));
        break;
      case ImportError::kInternalError:
        std::move(callback).Run(
            false,
            l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_IMPORT_INTERNAL_ERROR));
        break;
      case ImportError::kNone:
      default:
        NOTREACHED();
    }
    return;
  }

  keyring_service_->RestoreWallet(
      info.mnemonic, new_password, info.is_legacy_crypto_wallets,
      base::BindOnce(
          [](ImportFromExternalWalletCallback callback,
             size_t number_of_accounts, KeyringService* keyring_service,
             bool is_valid_mnemonic) {
            if (!is_valid_mnemonic) {
              std::move(callback).Run(
                  false,
                  l10n_util::GetStringUTF8(IDS_WALLET_INVALID_MNEMONIC_ERROR));
              return;
            }
            if (number_of_accounts > 1) {
              keyring_service->AddAccountsWithDefaultName(
                  mojom::CoinType::ETH, mojom::kDefaultKeyringId,
                  number_of_accounts - 1);
            }
            std::move(callback).Run(is_valid_mnemonic, absl::nullopt);
          },
          std::move(callback), info.number_of_accounts, keyring_service_));
}

void BraveWalletService::AddSignMessageRequest(
    mojom::SignMessageRequestPtr request,
    SignMessageRequestCallback callback) {
  if (request->id < 0) {
    request->id = sign_message_id_++;
  }
  sign_message_requests_.push_back(std::move(request));
  sign_message_callbacks_.push_back(std::move(callback));
}

void BraveWalletService::AddSignTransactionRequest(
    mojom::SignTransactionRequestPtr request,
    SignTransactionRequestCallback callback) {
  if (request->id < 0) {
    request->id = sign_transaction_id_++;
  }
  sign_transaction_requests_.push_back(std::move(request));
  sign_transaction_callbacks_.push_back(std::move(callback));
  if (sign_tx_request_added_cb_for_testing_) {
    std::move(sign_tx_request_added_cb_for_testing_).Run();
  }
}

void BraveWalletService::AddSignAllTransactionsRequest(
    mojom::SignAllTransactionsRequestPtr request,
    SignAllTransactionsRequestCallback callback) {
  if (request->id < 0) {
    request->id = sign_all_transactions_id_++;
  }
  sign_all_transactions_requests_.push_back(std::move(request));
  sign_all_transactions_callbacks_.push_back(std::move(callback));
  if (sign_all_txs_request_added_cb_for_testing_) {
    std::move(sign_all_txs_request_added_cb_for_testing_).Run();
  }
}

void BraveWalletService::AddSuggestTokenRequest(
    mojom::AddSuggestTokenRequestPtr request,
    mojom::EthereumProvider::RequestCallback callback,
    base::Value id) {
  // wallet_watchAsset currently only expect non-empty contract address and
  // only ERC20 type.
  DCHECK(!request->token->contract_address.empty());
  DCHECK(request->token->is_erc20 && !request->token->is_erc721);

  if (add_suggest_token_requests_.contains(request->token->contract_address)) {
    bool reject = true;
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  const std::string addr = request->token->contract_address;

  // Priority of token source:
  //     1. User asset list
  //     2. BlockchainRegistry
  //     3. wallet_watchAsset request
  mojom::BlockchainTokenPtr token =
      GetUserAsset(request->token->contract_address, request->token->token_id,
                   ShouldCheckTokenId(request->token), request->token->chain_id,
                   request->token->coin);

  if (!token) {
    token = BlockchainRegistry::GetInstance()->GetTokenByAddress(
        request->token->chain_id, request->token->coin, addr);
  }

  if (token) {
    request->token = std::move(token);
  }

  if (request->token->coingecko_id.empty()) {
    absl::optional<std::string> coingecko_id =
        BlockchainRegistry::GetInstance()->GetCoingeckoId(
            request->token->chain_id, request->token->contract_address);
    if (coingecko_id) {
      request->token->coingecko_id = *coingecko_id;
    }
  }

  add_suggest_token_requests_[addr] = std::move(request);
  add_suggest_token_callbacks_[addr] = std::move(callback);
  add_suggest_token_ids_[addr] = std::move(id);
}

void BraveWalletService::AddGetPublicKeyRequest(
    const std::string& address,
    const url::Origin& origin,
    mojom::EthereumProvider::RequestCallback callback,
    base::Value id) {
  // There can be only 1 request per origin
  if (HasPendingGetEncryptionPublicKeyRequestForOrigin(origin)) {
    bool reject = true;
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  auto request_id = GenerateRandomHexString();
  auto& pending_request =
      pending_get_encryption_public_key_requests_[request_id];
  pending_request.origin = origin;
  pending_request.encryption_public_key_request =
      mojom::GetEncryptionPublicKeyRequest::New(
          request_id, MakeOriginInfo(origin), address);
  pending_request.encryption_public_key_callback = std::move(callback);
  pending_request.encryption_public_key_id = std::move(id);
}

void BraveWalletService::AddDecryptRequest(
    const url::Origin& origin,
    const std::string& address,
    std::string unsafe_message,
    mojom::EthereumProvider::RequestCallback callback,
    base::Value id) {
  // There can be only 1 request per origin
  if (HasPendingDecryptRequestForOrigin(origin)) {
    bool reject = true;
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }

  auto request_id = GenerateRandomHexString();
  auto& pending_request = pending_decrypt_requests_[request_id];
  pending_request.origin = origin;
  pending_request.decrypt_request = mojom::DecryptRequest::New(
      request_id, MakeOriginInfo(origin), address, std::move(unsafe_message));
  pending_request.decrypt_callback = std::move(callback);
  pending_request.decrypt_id = std::move(id);
}

void BraveWalletService::GetPendingGetEncryptionPublicKeyRequests(
    GetPendingGetEncryptionPublicKeyRequestsCallback callback) {
  std::vector<mojom::GetEncryptionPublicKeyRequestPtr> requests;
  for (const auto& request : pending_get_encryption_public_key_requests_) {
    requests.push_back(request.second.encryption_public_key_request.Clone());
  }
  std::move(callback).Run(std::move(requests));
}

void BraveWalletService::GetPendingAddSuggestTokenRequests(
    GetPendingAddSuggestTokenRequestsCallback callback) {
  std::vector<mojom::AddSuggestTokenRequestPtr> requests;
  for (const auto& request : add_suggest_token_requests_) {
    requests.push_back(request.second.Clone());
  }
  std::move(callback).Run(std::move(requests));
}

void BraveWalletService::GetPendingDecryptRequests(
    GetPendingDecryptRequestsCallback callback) {
  std::vector<mojom::DecryptRequestPtr> requests;
  for (const auto& request : pending_decrypt_requests_) {
    requests.push_back(request.second.decrypt_request.Clone());
  }
  std::move(callback).Run(std::move(requests));
}

void BraveWalletService::NotifyAddSuggestTokenRequestsProcessed(
    bool approved,
    const std::vector<std::string>& contract_addresses) {
  for (const auto& addr : contract_addresses) {
    if (add_suggest_token_requests_.contains(addr) &&
        add_suggest_token_callbacks_.contains(addr) &&
        add_suggest_token_ids_.contains(addr)) {
      auto callback = std::move(add_suggest_token_callbacks_[addr]);
      base::Value id = std::move(add_suggest_token_ids_[addr]);

      bool reject = false;
      if (approved &&
          !AddUserAsset(add_suggest_token_requests_[addr]->token.Clone()) &&
          !SetUserAssetVisible(add_suggest_token_requests_[addr]->token.Clone(),
                               true)) {
        add_suggest_token_requests_.erase(addr);
        add_suggest_token_callbacks_.erase(addr);
        add_suggest_token_ids_.erase(addr);

        base::Value formed_response = GetProviderErrorDictionary(
            mojom::ProviderError::kInternalError,
            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
        reject = true;
        std::move(callback).Run(std::move(id), std::move(formed_response),
                                reject, "", false);
        continue;
      }

      add_suggest_token_requests_.erase(addr);
      add_suggest_token_callbacks_.erase(addr);
      add_suggest_token_ids_.erase(addr);
      reject = false;
      std::move(callback).Run(std::move(id), base::Value(approved), reject, "",
                              false);
    }
  }
}

void BraveWalletService::NotifyGetPublicKeyRequestProcessed(
    const std::string& request_id,
    bool approved) {
  if (!pending_get_encryption_public_key_requests_.contains(request_id)) {
    return;
  }

  auto request =
      std::move(pending_get_encryption_public_key_requests_[request_id]);
  pending_get_encryption_public_key_requests_.erase(request_id);

  auto address = std::move(request.encryption_public_key_request->address);
  auto callback = std::move(request.encryption_public_key_callback);
  base::Value id = std::move(request.encryption_public_key_id);

  bool reject = true;
  if (approved) {
    std::string key;
    if (!keyring_service_
             ->GetPublicKeyFromX25519_XSalsa20_Poly1305ByDefaultKeyring(address,
                                                                        &key)) {
      base::Value formed_response = GetProviderErrorDictionary(
          mojom::ProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                              "", false);
      return;
    }

    reject = false;
    std::move(callback).Run(std::move(id), base::Value(key), reject, "", false);
  } else {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
  }
}

void BraveWalletService::NotifyDecryptRequestProcessed(
    const std::string& request_id,
    bool approved) {
  if (!pending_decrypt_requests_.contains(request_id)) {
    return;
  }

  auto request = std::move(pending_decrypt_requests_[request_id]);
  pending_decrypt_requests_.erase(request_id);

  std::string unsafe_message =
      std::move(request.decrypt_request->unsafe_message);
  auto callback = std::move(request.decrypt_callback);
  base::Value id = std::move(request.decrypt_id);

  bool reject = true;
  if (approved) {
    std::string key;
    reject = false;
    std::move(callback).Run(std::move(id), base::Value(unsafe_message), reject,
                            "", false);
  } else {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
  }
}

void BraveWalletService::IsBase58EncodedSolanaPubkey(
    const std::string& key,
    IsBase58EncodedSolanaPubkeyCallback callback) {
  std::move(callback).Run(::brave_wallet::IsBase58EncodedSolanaPubkey(key));
}

void BraveWalletService::Base58Encode(
    const std::vector<std::vector<std::uint8_t>>& addresses,
    Base58EncodeCallback callback) {
  std::vector<std::string> encoded_addresses;
  for (const auto& address : addresses) {
    encoded_addresses.push_back(brave_wallet::Base58Encode(address));
  }
  std::move(callback).Run(std::move(encoded_addresses));
}

void BraveWalletService::DiscoverAssetsOnAllSupportedChains() {
  DiscoverAssetsOnAllSupportedChains(false);
}

void BraveWalletService::DiscoverAssetsOnAllSupportedChains(
    bool bypass_rate_limit) {
  std::map<mojom::CoinType, std::vector<std::string>> addresses;
  // Fetch ETH addresses
  mojom::KeyringInfoPtr keyring_info = keyring_service_->GetKeyringInfoSync(
      brave_wallet::mojom::kDefaultKeyringId);
  std::vector<std::string> eth_account_addresses;
  for (auto& account_info : keyring_info->account_infos) {
    eth_account_addresses.push_back(account_info->address);
  }
  addresses[mojom::CoinType::ETH] = std::move(eth_account_addresses);

  // Fetch SOL addresses
  keyring_info = keyring_service_->GetKeyringInfoSync(
      brave_wallet::mojom::kSolanaKeyringId);
  std::vector<std::string> sol_account_addresses;
  for (const auto& account_info : keyring_info->account_infos) {
    sol_account_addresses.push_back(account_info->address);
  }
  addresses[mojom::CoinType::SOL] = std::move(sol_account_addresses);

  // Discover assets owned by the SOL and ETH addresses on all supported chains
  asset_discovery_manager_->DiscoverAssetsOnAllSupportedChains(
      addresses, bypass_rate_limit);
}

void BraveWalletService::GetNftDiscoveryEnabled(
    GetNftDiscoveryEnabledCallback callback) {
  std::move(callback).Run(
      profile_prefs_->GetBoolean(kBraveWalletNftDiscoveryEnabled));
}

void BraveWalletService::SetNftDiscoveryEnabled(bool enabled) {
  profile_prefs_->SetBoolean(kBraveWalletNftDiscoveryEnabled, enabled);
}

void BraveWalletService::GetBalanceScannerSupportedChains(
    GetBalanceScannerSupportedChainsCallback callback) {
  const auto& contract_addresses = GetEthBalanceScannerContractAddresses();

  std::vector<std::string> chain_ids;
  for (const auto& entry : contract_addresses) {
    chain_ids.push_back(entry.first);
  }

  std::move(callback).Run(chain_ids);
}

void BraveWalletService::ConvertFEVMToFVMAddress(
    bool is_mainnet,
    const std::vector<std::string>& fevm_addresses,
    ConvertFEVMToFVMAddressCallback callback) {
  base::flat_map<std::string, std::string> result;
  for (const auto& fevm_address : fevm_addresses) {
    auto address = FilAddress::FromFEVMAddress(is_mainnet, fevm_address);
    DCHECK(result.find(fevm_address) == result.end());
    if (!address.IsEmpty()) {
      result[fevm_address] = address.EncodeAsString();
    }
  }
  std::move(callback).Run(std::move(result));
}

void BraveWalletService::GetSimpleHashSpamNFTs(
    const std::string& wallet_address,
    const std::vector<std::string>& chain_ids,
    mojom::CoinType coin,
    const absl::optional<std::string>& cursor,
    GetSimpleHashSpamNFTsCallback callback) {
  // Do not make requests to SimpleHash unless the user has
  // opted in to NFT discovery.
  if (!profile_prefs_->GetBoolean(kBraveWalletNftDiscoveryEnabled)) {
    std::move(callback).Run({}, absl::nullopt);
    return;
  }
  simple_hash_client_->FetchNFTsFromSimpleHash(
      wallet_address, chain_ids, coin, cursor, false /* skip_spam */,
      true /* only_spam */, std::move(callback));
}

void BraveWalletService::CancelAllSuggestedTokenCallbacks() {
  add_suggest_token_requests_.clear();
  // Reject pending suggest token requests when network changed.
  bool reject = true;
  for (auto& callback : add_suggest_token_callbacks_) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    std::move(callback.second)
        .Run(std::move(add_suggest_token_ids_[callback.first]),
             std::move(formed_response), reject, "", false);
  }
  add_suggest_token_callbacks_.clear();
  add_suggest_token_ids_.clear();
}

void BraveWalletService::CancelAllSignMessageCallbacks() {
  while (!sign_message_requests_.empty()) {
    auto callback = std::move(sign_message_callbacks_.front());
    sign_message_requests_.pop_front();
    sign_message_callbacks_.pop_front();
    std::move(callback).Run(false, nullptr, absl::nullopt);
  }
}

void BraveWalletService::CancelAllSignTransactionCallbacks() {
  while (!sign_transaction_requests_.empty()) {
    auto callback = std::move(sign_transaction_callbacks_.front());
    sign_transaction_requests_.pop_front();
    sign_transaction_callbacks_.pop_front();
    std::move(callback).Run(false, nullptr, absl::nullopt);
  }
}

void BraveWalletService::CancelAllSignAllTransactionsCallbacks() {
  while (!sign_all_transactions_requests_.empty()) {
    auto callback = std::move(sign_all_transactions_callbacks_.front());
    sign_all_transactions_requests_.pop_front();
    sign_all_transactions_callbacks_.pop_front();
    std::move(callback).Run(false, absl::nullopt, absl::nullopt);
  }
}

void BraveWalletService::CancelAllGetEncryptionPublicKeyCallbacks() {
  base::Value formed_response = GetProviderErrorDictionary(
      mojom::ProviderError::kUserRejectedRequest,
      l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  bool reject = true;
  for (auto& request : pending_get_encryption_public_key_requests_) {
    std::move(request.second.encryption_public_key_callback)
        .Run(std::move(request.second.encryption_public_key_id),
             formed_response.Clone(), reject, "", false);
  }
  pending_get_encryption_public_key_requests_.clear();
}

void BraveWalletService::CancelAllDecryptCallbacks() {
  base::Value formed_response = GetProviderErrorDictionary(
      mojom::ProviderError::kUserRejectedRequest,
      l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));

  bool reject = true;
  for (auto& request : pending_decrypt_requests_) {
    std::move(request.second.decrypt_callback)
        .Run(std::move(request.second.decrypt_id), formed_response.Clone(),
             reject, "", false);
  }
  pending_decrypt_requests_.clear();
}

void BraveWalletService::OnNetworkChanged() {
  CancelAllSuggestedTokenCallbacks();
}

void BraveWalletService::Reset() {
  delegate_->ClearWalletUIStoragePartition();

  if (eth_allowance_manager_) {
    eth_allowance_manager_->Reset();
  }

  if (tx_service_) {
    tx_service_->Reset();
  }
  if (json_rpc_service_) {
    json_rpc_service_->Reset();
  }

  // Clear BraveWalletService
  ClearBraveWalletServicePrefs(profile_prefs_);
  CancelAllSuggestedTokenCallbacks();
  CancelAllSignMessageCallbacks();
  CancelAllSignTransactionCallbacks();
  CancelAllSignAllTransactionsCallbacks();
  CancelAllGetEncryptionPublicKeyCallbacks();
  CancelAllDecryptCallbacks();

  if (keyring_service_) {
    keyring_service_->Reset();
  }

  for (const auto& observer : observers_) {
    observer->OnResetWallet();
  }
}

void BraveWalletService::DiscoverEthAllowances(
    DiscoverEthAllowancesCallback callback) {
  eth_allowance_manager_->DiscoverEthAllowancesOnAllSupportedChains(
      std::move(callback));
}

}  // namespace brave_wallet
