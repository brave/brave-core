/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include <map>
#include <optional>
#include <vector>

#include "base/check_is_test.h"
#include "base/containers/contains.h"
#include "base/notreached.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/account_discovery_manager.h"
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_allowance_manager.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
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

bool ShouldCheckTokenId(const brave_wallet::mojom::BlockchainTokenPtr& token) {
  return token->is_erc721 || token->is_erc1155;
}

bool TokenMatchesDict(const brave_wallet::mojom::BlockchainTokenPtr& token,
                      const base::Value::Dict* dict) {
  if (!dict) {
    return false;
  }

  std::optional<int> coin = dict->FindInt("coin");
  if (!coin || *coin != static_cast<int>(token->coin)) {
    return false;
  }

  const std::string* chain_id = dict->FindString("chain_id");
  if (!chain_id || *chain_id != token->chain_id) {
    return false;
  }

  const std::string* address_value = dict->FindString("address");
  if (!address_value || !base::EqualsCaseInsensitiveASCII(
                            *address_value, token->contract_address)) {
    return false;
  }

  if (ShouldCheckTokenId(token)) {
    const std::string* token_id_ptr = dict->FindString("token_id");
    return token_id_ptr && *token_id_ptr == token->token_id;
  } else {
    return true;
  }
}

base::Value::Dict GetEthNativeAssetFromChain(
    const brave_wallet::mojom::NetworkInfoPtr& chain) {
  base::Value::Dict native_asset;
  native_asset.Set("chain_id", chain->chain_id);
  native_asset.Set("coin", static_cast<int>(chain->coin));
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

}  // namespace

namespace brave_wallet {

namespace {
bool AccountMatchesCoinAndChain(const mojom::AccountId& account_id,
                                mojom::CoinType coin,
                                const std::string& chain_id) {
  return base::Contains(GetSupportedKeyringsForNetwork(coin, chain_id),
                        account_id.keyring_id);
}

// Ensure token list contains native tokens when appears empty. Only for BTC
// and ZEC by now.
std::vector<mojom::BlockchainTokenPtr> EnsureNativeTokens(
    const std::string& chain_id,
    mojom::CoinType coin,
    std::vector<mojom::BlockchainTokenPtr> tokens) {
  if (coin != mojom::CoinType::BTC && coin != mojom::CoinType::ZEC) {
    return tokens;
  }

  if (!tokens.empty()) {
    return tokens;
  }

  if (coin == mojom::CoinType::BTC && IsBitcoinNetwork(chain_id)) {
    tokens.push_back(GetBitcoinNativeToken(chain_id));
  }

  if (coin == mojom::CoinType::ZEC && IsZCashNetwork(chain_id)) {
    tokens.push_back(GetZcashNativeToken(chain_id));
  }

  return tokens;
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
    BitcoinWalletService* bitcoin_wallet_service,
    ZCashWalletService* zcash_wallet_service,
    PrefService* profile_prefs,
    PrefService* local_state)
    : delegate_(std::move(delegate)),
      keyring_service_(keyring_service),
      json_rpc_service_(json_rpc_service),
      tx_service_(tx_service),
      bitcoin_wallet_service_(bitcoin_wallet_service),
      zcash_wallet_service_(zcash_wallet_service),
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

  if (!delegate_) {
    CHECK_IS_TEST();
  } else {
    delegate_->AddObserver(this);
  }

  keyring_service_->AddObserver(
      keyring_observer_receiver_.BindNewPipeAndPassRemote());

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
std::optional<std::string> BraveWalletService::GetUserAssetAddress(
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
      return std::nullopt;
    }
    return address;
  }

  return std::nullopt;
}

// static
bool BraveWalletService::ValidateAndFixAssetAddress(
    mojom::BlockchainTokenPtr& token) {
  if (auto address = GetUserAssetAddress(token->contract_address, token->coin,
                                         token->chain_id)) {
    token->contract_address = *address;
    return true;
  }

  return false;
}

// static
std::optional<std::string> BraveWalletService::GetChecksumAddress(
    const std::string& contract_address,
    const std::string& chain_id) {
  if (contract_address.empty()) {
    return "";
  }

  const auto eth_addr = EthAddress::FromHex(contract_address);
  if (eth_addr.IsEmpty()) {
    return std::nullopt;
  }
  uint256_t chain;
  if (!HexValueToUint256(chain_id, &chain)) {
    return std::nullopt;
  }

  return eth_addr.ToChecksumAddress(chain);
}

// static
std::vector<mojom::BlockchainTokenPtr> BraveWalletService::GetUserAssets(
    PrefService* profile_prefs) {
  std::vector<mojom::BlockchainTokenPtr> result;
  const auto& user_assets_list =
      profile_prefs->GetList(kBraveWalletUserAssetsList);
  for (auto& asset : user_assets_list) {
    auto* token_dict = asset.GetIfDict();
    if (!token_dict) {
      continue;
    }

    if (auto token_ptr = ValueToBlockchainToken(*token_dict)) {
      result.push_back(std::move(token_ptr));
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
  for (auto& item : GetUserAssets(profile_prefs)) {
    if (base::EqualsCaseInsensitiveASCII(item->chain_id, chain_id) &&
        item->coin == coin) {
      result.push_back(std::move(item));
    }
  }

  return result;
}

// static
mojom::BlockchainTokenPtr BraveWalletService::AddUserAsset(
    mojom::BlockchainTokenPtr token,
    PrefService* profile_prefs) {
  if (!GetChain(profile_prefs, token->chain_id, token->coin)) {
    return nullptr;
  }

  if (!ValidateAndFixAssetAddress(token)) {
    return nullptr;
  }

  if (ShouldCheckTokenId(token)) {
    uint256_t token_id_uint = 0;
    if (!HexValueToUint256(token->token_id, &token_id_uint)) {
      return nullptr;
    }
  }

  ScopedListPrefUpdate update(profile_prefs, kBraveWalletUserAssetsList);

  for (auto& existing_asset : *update) {
    if (TokenMatchesDict(token, existing_asset.GetIfDict())) {
      return nullptr;
    }
  }

  update->Append(BlockchainTokenToValue(token));

  return token;
}

void BraveWalletService::GetUserAssets(const std::string& chain_id,
                                       mojom::CoinType coin,
                                       GetUserAssetsCallback callback) {
  std::vector<mojom::BlockchainTokenPtr> result =
      GetUserAssets(chain_id, coin, profile_prefs_);
  std::move(callback).Run(
      EnsureNativeTokens(chain_id, coin, std::move(result)));
}

void BraveWalletService::GetAllUserAssets(GetUserAssetsCallback callback) {
  std::vector<mojom::BlockchainTokenPtr> result = GetUserAssets(profile_prefs_);
  std::move(callback).Run(std::move(result));
}

bool BraveWalletService::AddUserAssetInternal(mojom::BlockchainTokenPtr token) {
  auto added_asset =
      BraveWalletService::AddUserAsset(std::move(token), profile_prefs_);
  if (!added_asset) {
    return false;
  }

  for (const auto& observer : token_observers_) {
    observer->OnTokenAdded(added_asset.Clone());
  }
  return true;
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

  std::move(callback).Run(AddUserAssetInternal(std::move(token)));
}

void BraveWalletService::OnGetEthNftStandard(
    mojom::BlockchainTokenPtr token,
    AddUserAssetCallback callback,
    const std::optional<std::string>& standard,
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

  std::move(callback).Run(AddUserAssetInternal(std::move(token)));
}

void BraveWalletService::RemoveUserAsset(mojom::BlockchainTokenPtr token,
                                         RemoveUserAssetCallback callback) {
  std::move(callback).Run(RemoveUserAsset(std::move(token)));
}

bool BraveWalletService::RemoveUserAsset(mojom::BlockchainTokenPtr token) {
  ScopedListPrefUpdate update(profile_prefs_, kBraveWalletUserAssetsList);

  auto removed = update->EraseIf([&token](const base::Value& value) {
    return TokenMatchesDict(token, value.GetIfDict());
  });

  if (!removed) {
    return false;
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
  CHECK(token);

  ScopedListPrefUpdate update(profile_prefs_, kBraveWalletUserAssetsList);

  for (auto& token_value : *update) {
    if (TokenMatchesDict(token, token_value.GetIfDict())) {
      token_value.GetDict().Set("visible", visible);
      return true;
    }
  }

  return false;
}

void BraveWalletService::SetAssetSpamStatus(
    mojom::BlockchainTokenPtr token,
    bool is_spam,
    SetAssetSpamStatusCallback callback) {
  std::move(callback).Run(SetAssetSpamStatus(std::move(token), is_spam));
}

bool BraveWalletService::SetAssetSpamStatus(mojom::BlockchainTokenPtr token,
                                            bool is_spam) {
  CHECK(token);

  {
    ScopedListPrefUpdate update(profile_prefs_, kBraveWalletUserAssetsList);

    for (auto& token_value : *update) {
      if (TokenMatchesDict(token, token_value.GetIfDict())) {
        token_value.GetDict().Set("is_spam", is_spam);
        // Marking a token as spam makes it not visible and vice-versa
        token_value.GetDict().Set("visible", !is_spam);
        return true;
      }
    }
  }

  // If the asset is not in the user's asset list, we automatically add it
  // and set the spam status.
  token->is_spam = is_spam;
  token->visible = !is_spam;
  return AddUserAssetInternal(token.Clone());
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
  // behavior.
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

  return GetCurrentChainId(prefs, coin, std::nullopt) == chain_id;
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

void BraveWalletService::MigrateAssetsPrefToList(PrefService* prefs) {
  if (!prefs->HasPrefPath(kBraveWalletUserAssetsDeprecated)) {
    return;
  }

  base::Value::List assets_list;

  const auto& user_assets_dict =
      prefs->GetDict(kBraveWalletUserAssetsDeprecated);
  for (auto coin_it : user_assets_dict) {
    auto coin = GetCoinTypeFromPrefKey_DEPRECATED(coin_it.first);
    if (!coin) {
      continue;
    }

    for (auto network_it : coin_it.second.GetDict()) {
      auto chain_id = GetChainIdByNetworkId_DEPRECATED(prefs, coin.value(),
                                                       network_it.first);

      if (!chain_id) {
        continue;
      }

      for (const auto& item : network_it.second.GetList()) {
        const auto* token_legacy = item.GetIfDict();
        if (!token_legacy) {
          continue;
        }

        auto token = token_legacy->Clone();
        token.Set("chain_id", *chain_id);
        token.Set("coin", static_cast<int>(*coin));

        assets_list.Append(std::move(token));
      }
    }
  }

  prefs->SetList(kBraveWalletUserAssetsList, std::move(assets_list));

  prefs->ClearPref(kBraveWalletUserAssetsDeprecated);
}

// static
base::Value::List BraveWalletService::GetDefaultEthereumAssets() {
  base::Value::List user_assets_list;

  base::Value::Dict bat;
  bat.Set("chain_id", mojom::kMainnetChainId);
  bat.Set("coin", static_cast<int>(mojom::CoinType::ETH));
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
    user_assets_list.Append(GetEthNativeAssetFromChain(chain));
    if (chain->chain_id == mojom::kMainnetChainId) {
      user_assets_list.Append(bat.Clone());
    }
  }

  return user_assets_list;
}

// static
base::Value::List BraveWalletService::GetDefaultSolanaAssets() {
  base::Value::List user_assets_list;

  base::Value::Dict sol;
  sol.Set("coin", static_cast<int>(mojom::CoinType::SOL));
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

  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::SOL)) {
    auto asset = sol.Clone();
    asset.Set("chain_id", chain->chain_id);
    user_assets_list.Append(std::move(asset));
  }

  return user_assets_list;
}

// static
base::Value::List BraveWalletService::GetDefaultFilecoinAssets() {
  base::Value::List user_assets_list;

  base::Value::Dict fil;
  fil.Set("coin", static_cast<int>(mojom::CoinType::FIL));
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

  for (const auto& chain : GetAllKnownChains(nullptr, mojom::CoinType::FIL)) {
    auto asset = fil.Clone();
    asset.Set("chain_id", chain->chain_id);
    user_assets_list.Append(std::move(asset));
  }

  return user_assets_list;
}

// static
base::Value::List BraveWalletService::GetDefaultBitcoinAssets() {
  base::Value::List user_assets_list;

  user_assets_list.Append(
      BlockchainTokenToValue(GetBitcoinNativeToken(mojom::kBitcoinMainnet)));
  user_assets_list.Append(
      BlockchainTokenToValue(GetBitcoinNativeToken(mojom::kBitcoinTestnet)));

  return user_assets_list;
}

// static
base::Value::List BraveWalletService::GetDefaultZCashAssets() {
  base::Value::List user_assets_list;

  user_assets_list.Append(
      BlockchainTokenToValue(GetZcashNativeToken(mojom::kZCashMainnet)));
  user_assets_list.Append(
      BlockchainTokenToValue(GetZcashNativeToken(mojom::kZCashTestnet)));

  return user_assets_list;
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
    const std::optional<std::string>& error) {
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

void BraveWalletService::GetPendingSignMessageErrors(
    GetPendingSignMessageErrorsCallback callback) {
  std::vector<mojom::SignMessageErrorPtr> errors;
  if (sign_message_errors_.empty()) {
    std::move(callback).Run(std::move(errors));
    return;
  }

  for (const auto& error : sign_message_errors_) {
    errors.push_back(error.Clone());
  }

  std::move(callback).Run(std::move(errors));
}

void BraveWalletService::NotifySignMessageErrorProcessed(
    const std::string& id) {
  if (sign_message_errors_.empty() || sign_message_errors_.front()->id != id) {
    VLOG(1) << "id: " << id << " is not expected, should be "
            << sign_message_errors_.front()->id;
    return;
  }
  sign_message_errors_.pop_front();
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
    const std::optional<std::string>& error) {
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
    std::optional<std::vector<mojom::ByteArrayStringUnionPtr>> signatures,
    const std::optional<std::string>& error) {
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

void BraveWalletService::WalletRestored() {
  account_discovery_manager_ = std::make_unique<AccountDiscoveryManager>(
      json_rpc_service_.get(), keyring_service_.get(),
      bitcoin_wallet_service_.get());
  account_discovery_manager_->StartDiscovery();
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
    base::OnceCallback<void(bool, const std::optional<std::string>&)> callback,
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

  bool is_valid_mnemonic = keyring_service_->RestoreWalletSync(
      info.mnemonic, new_password, info.is_legacy_crypto_wallets);
  if (!is_valid_mnemonic) {
    std::move(callback).Run(
        false, l10n_util::GetStringUTF8(IDS_WALLET_INVALID_MNEMONIC_ERROR));
    return;
  }
  if (info.number_of_accounts > 1) {
    keyring_service_->AddAccountsWithDefaultName(mojom::CoinType::ETH,
                                                 mojom::kDefaultKeyringId,
                                                 info.number_of_accounts - 1);
  }

  // Only register the component if the import is successful.
  CHECK(is_valid_mnemonic);
  WalletDataFilesInstaller::GetInstance()
      .MaybeRegisterWalletDataFilesComponentOnDemand(base::BindOnce(
          [](base::OnceCallback<void(bool, const std::optional<std::string>&)>
                 callback) {
            std::move(callback).Run(true /* is_valid_mnemonic */, std::nullopt);
          },
          std::move(callback)));
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

void BraveWalletService::AddSignMessageError(mojom::SignMessageErrorPtr error) {
  sign_message_errors_.push_back(std::move(error));
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
  DCHECK_EQ(request->token->coin, mojom::CoinType::ETH);
  DCHECK(!request->token->contract_address.empty());
  DCHECK(request->token->is_erc20);
  DCHECK(!request->token->is_erc721);
  DCHECK(!request->token->is_erc1155);
  DCHECK_EQ(request->token->token_id, "");

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
  mojom::BlockchainTokenPtr token;
  for (auto& user_asset : GetUserAssets(request->token->chain_id,
                                        mojom::CoinType::ETH, profile_prefs_)) {
    if (base::EqualsCaseInsensitiveASCII(request->token->contract_address,
                                         user_asset->contract_address)) {
      token = user_asset->Clone();
      break;
    }
  }

  if (!token) {
    token = BlockchainRegistry::GetInstance()->GetTokenByAddress(
        request->token->chain_id, request->token->coin, addr);
  }

  if (token) {
    request->token = std::move(token);
  }

  if (request->token->coingecko_id.empty()) {
    std::optional<std::string> coingecko_id =
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
    const mojom::AccountIdPtr& account_id,
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
          request_id, MakeOriginInfo(origin), account_id.Clone());
  pending_request.encryption_public_key_callback = std::move(callback);
  pending_request.encryption_public_key_id = std::move(id);
}

void BraveWalletService::AddDecryptRequest(
    const mojom::AccountIdPtr& account_id,
    const url::Origin& origin,
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
  pending_request.decrypt_request =
      mojom::DecryptRequest::New(request_id, MakeOriginInfo(origin),
                                 account_id.Clone(), std::move(unsafe_message));
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

      auto token = std::move(add_suggest_token_requests_[addr]->token);
      bool reject = false;
      if (approved && !AddUserAssetInternal(token->Clone()) &&
          !SetUserAssetVisible(token.Clone(), true)) {
        add_suggest_token_requests_.erase(addr);
        add_suggest_token_callbacks_.erase(addr);
        add_suggest_token_ids_.erase(addr);

        base::Value formed_response = GetProviderErrorDictionary(
            mojom::ProviderError::kInternalError, WalletInternalErrorMessage());
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

  auto account_id =
      std::move(request.encryption_public_key_request->account_id);
  auto callback = std::move(request.encryption_public_key_callback);
  base::Value id = std::move(request.encryption_public_key_id);

  bool reject = true;
  if (approved) {
    std::string key;
    if (!keyring_service_
             ->GetPublicKeyFromX25519_XSalsa20_Poly1305ByDefaultKeyring(
                 account_id, &key)) {
      base::Value formed_response = GetProviderErrorDictionary(
          mojom::ProviderError::kInternalError, WalletInternalErrorMessage());
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
  const auto& all_accounts = keyring_service_->GetAllAccountInfos();

  std::vector<std::string> eth_account_addresses;
  std::vector<std::string> sol_account_addresses;
  for (auto& account_info : all_accounts) {
    if (account_info->account_id->coin == mojom::CoinType::ETH) {
      eth_account_addresses.push_back(account_info->address);
    }
    if (account_info->account_id->coin == mojom::CoinType::SOL) {
      sol_account_addresses.push_back(account_info->address);
    }
  }
  addresses[mojom::CoinType::ETH] = std::move(eth_account_addresses);
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

void BraveWalletService::GenerateReceiveAddress(
    mojom::AccountIdPtr account_id,
    GenerateReceiveAddressCallback callback) {
  if (account_id->coin == mojom::CoinType::BTC) {
    if (!bitcoin_wallet_service_) {
      std::move(callback).Run("", WalletInternalErrorMessage());
      return;
    }
    bitcoin_wallet_service_->RunDiscovery(
        std::move(account_id), false,
        base::BindOnce(&BraveWalletService::OnGenerateBtcReceiveAddress,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }

  if (account_id->coin == mojom::CoinType::ZEC) {
    if (!zcash_wallet_service_) {
      std::move(callback).Run("", WalletInternalErrorMessage());
      return;
    }
    zcash_wallet_service_->DiscoverNextUnusedAddress(
        std::move(account_id), false,
        base::BindOnce(&BraveWalletService::OnGenerateZecReceiveAddress,
                       weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
    return;
  }

  if (account_id->coin == mojom::CoinType::ETH ||
      account_id->coin == mojom::CoinType::SOL ||
      account_id->coin == mojom::CoinType::FIL) {
    const auto& accounts = keyring_service_->GetAllAccountInfos();
    for (auto& account : accounts) {
      if (account->account_id == account_id) {
        std::move(callback).Run(account->address, std::nullopt);
        return;
      }
    }
    std::move(callback).Run("", WalletInternalErrorMessage());
    return;
  }

  NOTREACHED() << account_id->coin;
  std::move(callback).Run("", WalletInternalErrorMessage());
}

void BraveWalletService::OnGenerateBtcReceiveAddress(
    GenerateReceiveAddressCallback callback,
    mojom::BitcoinAddressPtr address,
    const std::optional<std::string>& error_message) {
  if (address) {
    std::move(callback).Run(address->address_string, std::nullopt);
    return;
  }

  std::move(callback).Run(std::nullopt,
                          error_message.value_or(WalletInternalErrorMessage()));
}

void BraveWalletService::OnGenerateZecReceiveAddress(
    GenerateReceiveAddressCallback callback,
    base::expected<mojom::ZCashAddressPtr, std::string> result) {
  if (result.has_value()) {
    std::move(callback).Run(result.value()->address_string, std::nullopt);
    return;
  }

  std::move(callback).Run(std::nullopt, result.error());
}

void BraveWalletService::GetSimpleHashSpamNFTs(
    const std::string& wallet_address,
    const std::vector<std::string>& chain_ids,
    mojom::CoinType coin,
    const std::optional<std::string>& cursor,
    GetSimpleHashSpamNFTsCallback callback) {
  // Do not make requests to SimpleHash unless the user has
  // opted in to NFT discovery.
  if (!profile_prefs_->GetBoolean(kBraveWalletNftDiscoveryEnabled)) {
    std::move(callback).Run({}, std::nullopt);
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
    std::move(callback).Run(false, nullptr, std::nullopt);
  }
}

void BraveWalletService::CancelAllSignTransactionCallbacks() {
  while (!sign_transaction_requests_.empty()) {
    auto callback = std::move(sign_transaction_callbacks_.front());
    sign_transaction_requests_.pop_front();
    sign_transaction_callbacks_.pop_front();
    std::move(callback).Run(false, nullptr, std::nullopt);
  }
}

void BraveWalletService::CancelAllSignAllTransactionsCallbacks() {
  while (!sign_all_transactions_requests_.empty()) {
    auto callback = std::move(sign_all_transactions_callbacks_.front());
    sign_all_transactions_requests_.pop_front();
    sign_all_transactions_callbacks_.pop_front();
    std::move(callback).Run(false, std::nullopt, std::nullopt);
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
  delegate_->ResetAllPermissions();

  if (eth_allowance_manager_) {
    eth_allowance_manager_->Reset();
  }

  if (tx_service_) {
    tx_service_->Reset();
  }
  if (json_rpc_service_) {
    json_rpc_service_->Reset();
  }
  if (bitcoin_wallet_service_) {
    bitcoin_wallet_service_->Reset();
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
  account_discovery_manager_.reset();

  for (const auto& observer : observers_) {
    observer->OnResetWallet();
  }
}

void BraveWalletService::DiscoverEthAllowances(
    DiscoverEthAllowancesCallback callback) {
  eth_allowance_manager_->DiscoverEthAllowancesOnAllSupportedChains(
      std::move(callback));
}

void BraveWalletService::GetAnkrSupportedChainIds(
    GetAnkrSupportedChainIdsCallback callback) {
  const auto& blockchains = GetAnkrBlockchains();

  std::vector<std::string> chain_ids;
  for (const auto& entry : blockchains) {
    chain_ids.push_back(entry.first);
  }

  std::move(callback).Run(std::move(chain_ids));
}

}  // namespace brave_wallet
