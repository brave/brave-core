/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include <utility>
#include <vector>

#include "base/metrics/histogram_macros.h"
#include "base/ranges/algorithm.h"
#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
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
    bool is_erc721,
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

        if (found && is_erc721) {
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
  native_asset.Set("is_nft", false);
  native_asset.Set("decimals", chain->decimals);
  native_asset.Set("visible", true);
  return native_asset;
}

}  // namespace

namespace brave_wallet {

BraveWalletService::BraveWalletService(
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
      asset_discovery_manager_(this,
                               json_rpc_service,
                               keyring_service,
                               profile_prefs),
      weak_ptr_factory_(this) {
  if (delegate_)
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
      kBraveWalletSelectedNetworks,
      base::BindRepeating(&BraveWalletService::OnNetworkChanged,
                          weak_ptr_factory_.GetWeakPtr()));
}

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
  if (address.empty())  // native asset
    return address;

  if (coin == mojom::CoinType::ETH) {
    return GetChecksumAddress(address, chain_id);
  }

  if (coin == mojom::CoinType::SOL) {
    std::vector<uint8_t> bytes;
    if (!::brave_wallet::IsBase58EncodedSolanaPubkey(address))
      return absl::nullopt;
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
    if (!token)
      continue;

    mojom::BlockchainTokenPtr tokenPtr =
        ValueToBlockchainToken(*token, chain_id, coin);
    if (tokenPtr)
      result.push_back(std::move(tokenPtr));
  }

  return result;
}

// static
bool BraveWalletService::AddUserAsset(mojom::BlockchainTokenPtr token,
                                      PrefService* profile_prefs) {
  absl::optional<std::string> address = GetUserAssetAddress(
      token->contract_address, token->coin, token->chain_id);
  if (!address)
    return false;

  const std::string network_id =
      GetNetworkId(profile_prefs, token->coin, token->chain_id);
  if (network_id.empty())
    return false;

  // Verify input token ID for ERC721.
  if (token->is_erc721) {
    uint256_t token_id_uint = 0;
    if (!HexValueToUint256(token->token_id, &token_id_uint)) {
      return false;
    }
  }

  DictionaryPrefUpdate update(profile_prefs, kBraveWalletUserAssets);
  auto* user_assets_pref = update.Get()->GetIfDict();
  DCHECK(user_assets_pref);

  const auto path =
      base::StrCat({GetPrefKeyForCoinType(token->coin), ".", network_id});
  auto* user_assets_list = user_assets_pref->FindListByDottedPath(path);
  if (!user_assets_list) {
    user_assets_list =
        user_assets_pref
            ->SetByDottedPath(path, base::Value(base::Value::Type::LIST))
            ->GetIfList();
  }
  DCHECK(user_assets_list);

  auto it =
      FindAsset(user_assets_list, *address, token->token_id, token->is_erc721);
  if (it != user_assets_list->end())
    return false;

  base::Value::Dict value;
  value.Set("address", *address);
  value.Set("name", token->name);
  value.Set("symbol", token->symbol);
  value.Set("logo", token->logo);
  value.Set("is_erc20", token->is_erc20);
  value.Set("is_erc721", token->is_erc721);
  value.Set("is_nft", token->is_nft);
  value.Set("decimals", token->decimals);
  value.Set("visible", true);
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

bool BraveWalletService::AddUserAsset(mojom::BlockchainTokenPtr token) {
  return BraveWalletService::AddUserAsset(std::move(token), profile_prefs_);
}

void BraveWalletService::AddUserAsset(mojom::BlockchainTokenPtr token,
                                      AddUserAssetCallback callback) {
  std::move(callback).Run(AddUserAsset(std::move(token)));
}

void BraveWalletService::RemoveUserAsset(mojom::BlockchainTokenPtr token,
                                         RemoveUserAssetCallback callback) {
  std::move(callback).Run(RemoveUserAsset(std::move(token)));
}

bool BraveWalletService::RemoveUserAsset(mojom::BlockchainTokenPtr token) {
  absl::optional<std::string> address = GetUserAssetAddress(
      token->contract_address, token->coin, token->chain_id);
  if (!address)
    return false;

  const std::string network_id =
      GetNetworkId(profile_prefs_, token->coin, token->chain_id);
  if (network_id.empty())
    return false;

  DictionaryPrefUpdate update(profile_prefs_, kBraveWalletUserAssets);
  auto* user_assets_pref = update.Get()->GetIfDict();
  DCHECK(user_assets_pref);

  auto* user_assets_list = user_assets_pref->FindListByDottedPath(
      base::StrCat({GetPrefKeyForCoinType(token->coin), ".", network_id}));
  if (!user_assets_list)
    return false;

  auto it =
      FindAsset(user_assets_list, *address, token->token_id, token->is_erc721);
  if (it != user_assets_list->end())
    user_assets_list->erase(it);
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
  if (!address)
    return false;

  const std::string network_id =
      GetNetworkId(profile_prefs_, token->coin, token->chain_id);
  if (network_id.empty())
    return false;

  DictionaryPrefUpdate update(profile_prefs_, kBraveWalletUserAssets);
  auto* user_assets_pref = update.Get()->GetIfDict();
  DCHECK(user_assets_pref);

  auto* user_assets_list = user_assets_pref->FindListByDottedPath(
      base::StrCat({GetPrefKeyForCoinType(token->coin), ".", network_id}));
  if (!user_assets_list)
    return false;

  auto it =
      FindAsset(user_assets_list, *address, token->token_id, token->is_erc721);
  if (it == user_assets_list->end())
    return false;

  it->GetDict().Set("visible", visible);
  return true;
}

mojom::BlockchainTokenPtr BraveWalletService::GetUserAsset(
    const std::string& raw_address,
    const std::string& token_id,
    bool is_erc721,
    const std::string& chain_id,
    mojom::CoinType coin) {
  absl::optional<std::string> address =
      GetUserAssetAddress(raw_address, coin, chain_id);
  if (!address)
    return nullptr;

  const std::string network_id = GetNetworkId(profile_prefs_, coin, chain_id);
  if (network_id.empty())
    return nullptr;

  const auto& user_assets_dict =
      profile_prefs_->GetDict(kBraveWalletUserAssets);
  const auto* user_assets_list = user_assets_dict.FindListByDottedPath(
      base::StrCat({GetPrefKeyForCoinType(coin), ".", network_id}));
  if (!user_assets_list)
    return nullptr;

  auto it = FindAsset(user_assets_list, *address, token_id, is_erc721);
  if (it == user_assets_list->end())
    return nullptr;

  return ValueToBlockchainToken(it->GetDict(), chain_id, coin);
}

void BraveWalletService::IsExternalWalletInstalled(
    mojom::ExternalWalletType type,
    IsExternalWalletInstalledCallback callback) {
  if (delegate_)
    delegate_->IsExternalWalletInstalled(type, std::move(callback));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::IsExternalWalletInitialized(
    mojom::ExternalWalletType type,
    IsExternalWalletInitializedCallback callback) {
  if (delegate_)
    delegate_->IsExternalWalletInitialized(type, std::move(callback));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::ImportFromExternalWallet(
    mojom::ExternalWalletType type,
    const std::string& password,
    const std::string& new_password,
    ImportFromExternalWalletCallback callback) {
  if (delegate_)
    delegate_->GetImportInfoFromExternalWallet(
        type, password,
        base::BindOnce(&BraveWalletService::OnGetImportInfo,
                       weak_ptr_factory_.GetWeakPtr(), new_password,
                       std::move(callback)));
  else
    std::move(callback).Run(false, l10n_util::GetStringUTF8(
                                       IDS_BRAVE_WALLET_IMPORT_INTERNAL_ERROR));
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

void BraveWalletService::GetShowWalletTestNetworks(
    GetShowWalletTestNetworksCallback callback) {
  std::move(callback).Run(
      ::brave_wallet::GetShowWalletTestNetworks(profile_prefs_));
}

void BraveWalletService::GetSelectedCoin(GetSelectedCoinCallback callback) {
  std::move(callback).Run(::brave_wallet::GetSelectedCoin(profile_prefs_));
}

void BraveWalletService::SetSelectedCoin(mojom::CoinType coin) {
  ::brave_wallet::SetSelectedCoin(profile_prefs_, coin);
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

void BraveWalletService::AddPermission(mojom::CoinType coin,
                                       const url::Origin& origin,
                                       const std::string& account,
                                       AddPermissionCallback callback) {
  if (delegate_)
    delegate_->AddPermission(coin, origin, account, std::move(callback));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::HasPermission(mojom::CoinType coin,
                                       const url::Origin& origin,
                                       const std::string& account,
                                       HasPermissionCallback callback) {
  if (delegate_)
    delegate_->HasPermission(coin, origin, account, std::move(callback));
  else
    std::move(callback).Run(false, false);
}

void BraveWalletService::ResetPermission(mojom::CoinType coin,
                                         const url::Origin& origin,
                                         const std::string& account,
                                         ResetPermissionCallback callback) {
  if (delegate_)
    delegate_->ResetPermission(coin, origin, account, std::move(callback));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::IsPermissionDenied(
    mojom::CoinType coin,
    const url::Origin& origin,
    IsPermissionDeniedCallback callback) {
  if (delegate_)
    delegate_->IsPermissionDenied(coin, origin, std::move(callback));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::GetWebSitesWithPermission(
    mojom::CoinType coin,
    GetWebSitesWithPermissionCallback callback) {
  if (delegate_)
    delegate_->GetWebSitesWithPermission(coin, std::move(callback));
  else
    std::move(callback).Run(std::vector<std::string>());
}

void BraveWalletService::ResetWebSitePermission(
    mojom::CoinType coin,
    const std::string& formed_website,
    ResetWebSitePermissionCallback callback) {
  if (delegate_)
    delegate_->ResetWebSitePermission(coin, formed_website,
                                      std::move(callback));
  else
    std::move(callback).Run(false);
}

// static
void BraveWalletService::MigrateUserAssetEthContractAddress(
    PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletUserAssetEthContractAddressMigrated))
    return;

  if (!prefs->HasPrefPath(kBraveWalletUserAssetsDeprecated)) {
    prefs->SetBoolean(kBraveWalletUserAssetEthContractAddressMigrated, true);
    return;
  }

  DictionaryPrefUpdate update(prefs, kBraveWalletUserAssetsDeprecated);
  auto& user_assets_pref = update.Get()->GetDict();

  for (auto user_asset_list : user_assets_pref) {
    auto& item = user_asset_list.second.GetList();
    auto it = FindAsset(&item, "eth", "", false, "contract_address");
    if (it == item.end())
      continue;

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
  if (!prefs->HasPrefPath(kBraveWalletUserAssetsDeprecated))
    return;

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
  if (prefs->GetBoolean(kBraveWalletUserAssetsAddPreloadingNetworksMigrated))
    return;

  if (!prefs->HasPrefPath(kBraveWalletUserAssets)) {
    prefs->SetBoolean(kBraveWalletUserAssetsAddPreloadingNetworksMigrated,
                      true);
    return;
  }

  DictionaryPrefUpdate update(prefs, kBraveWalletUserAssets);
  auto& user_assets_pref = update.Get()->GetDict();

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
          user_assets_pref
              .SetByDottedPath(path, base::Value(base::Value::Type::LIST))
              ->GetIfList();
      user_assets_list->Append(GetEthNativeAssetFromChain(chain));
      continue;
    }

    auto it = FindAsset(user_assets_list, "", "", false);
    if (it == user_assets_list->end())
      user_assets_list->Insert(user_assets_list->begin(),
                               base::Value(GetEthNativeAssetFromChain(chain)));
  }

  prefs->SetBoolean(kBraveWalletUserAssetsAddPreloadingNetworksMigrated, true);
}

// static
void BraveWalletService::MigrateUserAssetsAddIsNFT(PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletUserAssetsAddIsNFTMigrated))
    return;

  if (!prefs->HasPrefPath(kBraveWalletUserAssets)) {
    prefs->SetBoolean(kBraveWalletUserAssetsAddIsNFTMigrated, true);
    return;
  }

  DictionaryPrefUpdate update(prefs, kBraveWalletUserAssets);
  auto& user_assets_pref = update.Get()->GetDict();

  for (auto user_asset_dict_per_cointype : user_assets_pref) {
    if (!user_asset_dict_per_cointype.second.is_dict())
      continue;
    for (auto user_asset_list_per_chain :
         user_asset_dict_per_cointype.second.GetDict()) {
      if (!user_asset_list_per_chain.second.is_list())
        continue;
      for (auto& user_asset : user_asset_list_per_chain.second.GetList()) {
        auto* asset = user_asset.GetIfDict();
        if (!asset)
          continue;

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
base::Value::Dict BraveWalletService::GetDefaultEthereumAssets() {
  base::Value::Dict user_assets;

  base::Value::Dict bat;
  bat.Set("address", "0x0D8775F648430679A709E98d2b0Cb6250d2887EF");
  bat.Set("name", "Basic Attention Token");
  bat.Set("symbol", "BAT");
  bat.Set("is_erc20", true);
  bat.Set("is_erc721", false);
  bat.Set("is_nft", false);
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
    if (chain->chain_id == mojom::kMainnetChainId)
      user_assets_list.Append(bat.Clone());

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

void BraveWalletService::OnWalletUnlockPreferenceChanged(
    const std::string& pref_name) {
  brave_wallet_p3a_.ReportUsage(true);
}

BraveWalletP3A* BraveWalletService::GetBraveWalletP3A() {
  return &brave_wallet_p3a_;
}

void BraveWalletService::GetActiveOrigin(GetActiveOriginCallback callback) {
  if (delegate_)
    delegate_->GetActiveOrigin(std::move(callback));
  else
    std::move(callback).Run(MakeOriginInfo(url::Origin()));
}

void BraveWalletService::GeteTLDPlusOneFromOrigin(
    const url::Origin& origin,
    GetActiveOriginCallback callback) {
  std::move(callback).Run(MakeOriginInfo(origin));
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

void BraveWalletService::OnActiveOriginChanged(
    const mojom::OriginInfoPtr& origin_info) {
  for (const auto& observer : observers_) {
    observer->OnActiveOriginChanged(origin_info.Clone());
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
              keyring_service->AddAccountsWithDefaultName(number_of_accounts -
                                                          1);
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
}

void BraveWalletService::AddSignAllTransactionsRequest(
    mojom::SignAllTransactionsRequestPtr request,
    SignAllTransactionsRequestCallback callback) {
  if (request->id < 0) {
    request->id = sign_all_transactions_id_++;
  }
  sign_all_transactions_requests_.push_back(std::move(request));
  sign_all_transactions_callbacks_.push_back(std::move(callback));
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
                   request->token->is_erc721, request->token->chain_id,
                   request->token->coin);

  if (!token)
    token = BlockchainRegistry::GetInstance()->GetTokenByAddress(
        request->token->chain_id, request->token->coin, addr);

  if (token)
    request->token = std::move(token);

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
  if (add_get_encryption_public_key_requests_.contains(origin)) {
    bool reject = true;
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }
  add_get_encryption_public_key_requests_[origin] = address;
  add_get_encryption_public_key_callbacks_[origin] = std::move(callback);
  get_encryption_public_key_ids_[origin] = std::move(id);
}

void BraveWalletService::AddDecryptRequest(
    mojom::DecryptRequestPtr request,
    mojom::EthereumProvider::RequestCallback callback,
    base::Value id) {
  url::Origin origin = request->origin_info->origin;
  // There can be only 1 request per origin
  if (decrypt_requests_.contains(origin)) {
    bool reject = true;
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                            "", false);
    return;
  }
  decrypt_requests_[origin] = std::move(request);
  decrypt_callbacks_[origin] = std::move(callback);
  decrypt_ids_[origin] = std::move(id);
}

void BraveWalletService::GetPendingGetEncryptionPublicKeyRequests(
    GetPendingGetEncryptionPublicKeyRequestsCallback callback) {
  std::vector<mojom::GetEncryptionPublicKeyRequestPtr> requests;
  for (const auto& request : add_get_encryption_public_key_requests_) {
    requests.push_back(mojom::GetEncryptionPublicKeyRequest::New(
        MakeOriginInfo(request.first), request.second));
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
  for (const auto& request : decrypt_requests_) {
    requests.push_back(request.second.Clone());
  }
  std::move(callback).Run(std::move(requests));
}

void BraveWalletService::NotifyAddSuggestTokenRequestsProcessed(
    bool approved,
    const std::vector<std::string>& contract_addresses) {
  const std::string chain_id =
      GetCurrentChainId(profile_prefs_, mojom::CoinType::ETH);
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
    bool approved,
    const url::Origin& origin) {
  if (!add_get_encryption_public_key_requests_.contains(origin) ||
      !add_get_encryption_public_key_callbacks_.contains(origin) ||
      !get_encryption_public_key_ids_.contains(origin)) {
    return;
  }
  auto callback = std::move(add_get_encryption_public_key_callbacks_[origin]);
  base::Value id = std::move(get_encryption_public_key_ids_[origin]);

  std::string address = add_get_encryption_public_key_requests_[origin];
  add_get_encryption_public_key_requests_.erase(origin);
  add_get_encryption_public_key_callbacks_.erase(origin);
  get_encryption_public_key_ids_.erase(origin);

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
    bool approved,
    const url::Origin& origin) {
  if (!decrypt_requests_.contains(origin) ||
      !decrypt_callbacks_.contains(origin) || !decrypt_ids_.contains(origin)) {
    return;
  }
  auto callback = std::move(decrypt_callbacks_[origin]);
  base::Value id = std::move(decrypt_ids_[origin]);

  mojom::DecryptRequestPtr request = std::move(decrypt_requests_[origin]);
  decrypt_requests_.erase(origin);
  decrypt_callbacks_.erase(origin);
  decrypt_ids_.erase(origin);

  bool reject = true;
  if (approved) {
    std::string key;
    if (!request->unsafe_message) {
      base::Value formed_response = GetProviderErrorDictionary(
          mojom::ProviderError::kInternalError,
          l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
      std::move(callback).Run(std::move(id), std::move(formed_response), reject,
                              "", false);
      return;
    }

    reject = false;
    std::move(callback).Run(std::move(id),
                            base::Value(*request->unsafe_message), reject, "",
                            false);
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
  mojom::KeyringInfoPtr keyring_info = keyring_service_->GetKeyringInfoSync(
      brave_wallet::mojom::kDefaultKeyringId);

  std::vector<std::string> account_addresses;
  for (auto& account_info : keyring_info->account_infos) {
    account_addresses.push_back(account_info->address);
  }

  asset_discovery_manager_.DiscoverAssetsOnAllSupportedChainsRefresh(
      account_addresses);
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
  add_get_encryption_public_key_requests_.clear();
  bool reject = true;
  for (auto& callback : add_get_encryption_public_key_callbacks_) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    std::move(callback.second)
        .Run(std::move(get_encryption_public_key_ids_[callback.first]),
             std::move(formed_response), reject, "", false);
  }
  add_get_encryption_public_key_callbacks_.clear();
  get_encryption_public_key_ids_.clear();
}

void BraveWalletService::CancelAllDecryptCallbacks() {
  decrypt_requests_.clear();
  bool reject = true;
  for (auto& callback : decrypt_callbacks_) {
    base::Value formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    std::move(callback.second)
        .Run(std::move(decrypt_ids_[callback.first]),
             std::move(formed_response), reject, "", false);
  }
  decrypt_callbacks_.clear();
  decrypt_ids_.clear();
}

void BraveWalletService::OnNetworkChanged() {
  CancelAllSuggestedTokenCallbacks();
}

void BraveWalletService::Reset() {
  if (tx_service_)
    tx_service_->Reset();
  if (json_rpc_service_)
    json_rpc_service_->Reset();

  // Clear BraveWalletService
  ClearBraveWalletServicePrefs(profile_prefs_);
  CancelAllSuggestedTokenCallbacks();
  CancelAllSignMessageCallbacks();
  CancelAllSignTransactionCallbacks();
  CancelAllSignAllTransactionsCallbacks();
  CancelAllGetEncryptionPublicKeyCallbacks();
  CancelAllDecryptCallbacks();

  if (keyring_service_)
    keyring_service_->Reset();
}

}  // namespace brave_wallet
