/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include <utility>
#include <vector>

#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_importer_delegate.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

// kBraveWalletUserAssets
// {
//    "mainnet": {  // network_id
//      [
//        {
//          "contract_address": "eth",
//          "name": "Ethereum",
//          "symbol": "ETH",
//          "is_erc20": false,
//          "is_erc721": false,
//          "decimals": 18,
//          "visible": true
//          ...
//        },
//        {
//          "contract_address": "0x0D8775F648430679A709E98d2b0Cb6250d2887EF",
//          "name": "Basic Attention Token",
//          "symbol": "BAT",
//          "is_erc20": true,
//          "is_erc721": false,
//          "decimals": 18,
//          "visible": true
//          ...
//        },
//        {
//          "contract_address": "0x4729c2017edD1BaDf768595378c668955b537197",
//          "name": "MOR",
//          "symbol": "MOR",
//          "is_erc20": true,
//          "is_erc721": false,
//          "decimals": 18,
//          "visible": true
//          ...
//        },
//        ...
//      ]
//    },
//    "rinkeby": {
//    ...
//    },
//    ...
//    }
// }
//
//
namespace {

base::CheckedContiguousIterator<base::Value> FindAsset(
    base::Value* user_assets_dict,
    const std::string& contract_address) {
  DCHECK(user_assets_dict && user_assets_dict->is_list() &&
         !contract_address.empty());

  auto iter = std::find_if(
      user_assets_dict->GetList().begin(), user_assets_dict->GetList().end(),
      [&](const base::Value& value) {
        if (!value.is_dict()) {
          return false;
        }
        const std::string* address = value.FindStringKey("contract_address");
        return address && *address == contract_address;
      });

  return iter;
}

}  // namespace

namespace brave_wallet {

BraveWalletService::BraveWalletService(
    std::unique_ptr<BraveWalletImporterDelegate> delegate,
    PrefService* prefs)
    : delegate_(std::move(delegate)), prefs_(prefs) {
  DCHECK(prefs_);
}

BraveWalletService::~BraveWalletService() = default;

mojo::PendingRemote<mojom::BraveWalletService>
BraveWalletService::MakeRemote() {
  mojo::PendingRemote<mojom::BraveWalletService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void BraveWalletService::Bind(
    mojo::PendingReceiver<mojom::BraveWalletService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

absl::optional<std::string> BraveWalletService::GetChecksumAddress(
    const std::string& contract_address,
    const std::string& chain_id) {
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

void BraveWalletService::GetUserAssets(const std::string& chain_id,
                                       GetUserAssetsCallback callback) {
  const std::string network_id = GetNetworkId(prefs_, chain_id);
  if (network_id.empty()) {
    std::move(callback).Run(std::vector<mojom::ERCTokenPtr>());
    return;
  }

  const base::DictionaryValue* user_assets_dict =
      prefs_->GetDictionary(kBraveWalletUserAssets);
  if (!user_assets_dict) {
    std::move(callback).Run(std::vector<mojom::ERCTokenPtr>());
    return;
  }

  const base::Value* tokens = user_assets_dict->FindKey(network_id);
  if (!tokens) {
    std::move(callback).Run(std::vector<mojom::ERCTokenPtr>());
    return;
  }

  std::vector<mojom::ERCTokenPtr> result;
  for (const auto& token : tokens->GetList()) {
    mojom::ERCTokenPtr tokenPtr = mojom::ERCToken::New();

    const base::Value* value = token.FindKey("contract_address");
    if (!value || !value->is_string()) {
      continue;
    }
    tokenPtr->contract_address = value->GetString();

    value = token.FindKey("name");
    if (!value || !value->is_string()) {
      continue;
    }
    tokenPtr->name = value->GetString();

    value = token.FindKey("symbol");
    if (!value || !value->is_string()) {
      continue;
    }
    tokenPtr->symbol = value->GetString();

    value = token.FindKey("logo");
    if (value && value->is_string()) {
      tokenPtr->logo = value->GetString();
    }

    value = token.FindKey("is_erc20");
    if (!value || !value->is_bool()) {
      continue;
    }
    tokenPtr->is_erc20 = value->GetBool();

    value = token.FindKey("is_erc721");
    if (!value || !value->is_bool()) {
      continue;
    }
    tokenPtr->is_erc721 = value->GetBool();

    value = token.FindKey("decimals");
    if (!value || !value->is_int()) {
      continue;
    }
    tokenPtr->decimals = value->GetInt();

    value = token.FindKey("visible");
    if (!value || !value->is_bool()) {
      continue;
    }
    tokenPtr->visible = value->GetBool();

    result.push_back(std::move(tokenPtr));
  }

  std::move(callback).Run(std::move(result));
}

void BraveWalletService::AddUserAsset(mojom::ERCTokenPtr token,
                                      const std::string& chain_id,
                                      AddUserAssetCallback callback) {
  absl::optional<std::string> optional_checksum_address =
      GetChecksumAddress(token->contract_address, chain_id);
  if (!optional_checksum_address) {
    std::move(callback).Run(false);
    return;
  }
  const std::string checksum_address = optional_checksum_address.value();

  const std::string network_id = GetNetworkId(prefs_, chain_id);
  if (network_id.empty()) {
    std::move(callback).Run(false);
    return;
  }

  DictionaryPrefUpdate update(prefs_, kBraveWalletUserAssets);
  base::DictionaryValue* user_assets_pref = update.Get();

  base::Value* user_assets_dict = user_assets_pref->FindKey(network_id);
  if (!user_assets_dict) {
    user_assets_dict = user_assets_pref->SetKey(
        network_id, base::Value(base::Value::Type::LIST));
  }

  auto it = FindAsset(user_assets_dict, checksum_address);
  if (it != user_assets_dict->GetList().end()) {
    std::move(callback).Run(false);
    return;
  }

  base::Value value(base::Value::Type::DICTIONARY);
  value.SetKey("contract_address", base::Value(checksum_address));
  value.SetKey("name", base::Value(token->name));
  value.SetKey("symbol", base::Value(token->symbol));
  value.SetKey("logo", base::Value(token->logo));
  value.SetKey("is_erc20", base::Value(token->is_erc20));
  value.SetKey("is_erc721", base::Value(token->is_erc721));
  value.SetKey("decimals", base::Value(token->decimals));
  value.SetKey("visible", base::Value(true));

  user_assets_dict->Append(std::move(value));
  std::move(callback).Run(true);
}

void BraveWalletService::RemoveUserAsset(const std::string& contract_address,
                                         const std::string& chain_id,
                                         RemoveUserAssetCallback callback) {
  absl::optional<std::string> optional_checksum_address =
      GetChecksumAddress(contract_address, chain_id);
  if (!optional_checksum_address) {
    std::move(callback).Run(false);
    return;
  }
  const std::string checksum_address = optional_checksum_address.value();

  const std::string network_id = GetNetworkId(prefs_, chain_id);
  if (network_id.empty()) {
    std::move(callback).Run(false);
    return;
  }

  DictionaryPrefUpdate update(prefs_, kBraveWalletUserAssets);
  base::DictionaryValue* user_assets_pref = update.Get();

  base::Value* user_assets_dict = user_assets_pref->FindKey(network_id);
  if (!user_assets_dict) {
    std::move(callback).Run(false);
    return;
  }

  user_assets_dict->EraseListIter(
      FindAsset(user_assets_dict, checksum_address));
  std::move(callback).Run(true);
}

void BraveWalletService::SetUserAssetVisible(
    const std::string& contract_address,
    const std::string& chain_id,
    bool visible,
    SetUserAssetVisibleCallback callback) {
  absl::optional<std::string> optional_checksum_address =
      GetChecksumAddress(contract_address, chain_id);
  if (!optional_checksum_address) {
    std::move(callback).Run(false);
    return;
  }
  const std::string checksum_address = optional_checksum_address.value();

  const std::string network_id = GetNetworkId(prefs_, chain_id);
  if (network_id.empty()) {
    std::move(callback).Run(false);
    return;
  }

  DictionaryPrefUpdate update(prefs_, kBraveWalletUserAssets);
  base::DictionaryValue* user_assets_pref = update.Get();

  base::Value* user_assets_dict = user_assets_pref->FindKey(network_id);
  if (!user_assets_dict) {
    std::move(callback).Run(false);
    return;
  }

  auto it = FindAsset(user_assets_dict, checksum_address);
  if (it == user_assets_dict->GetList().end()) {
    std::move(callback).Run(false);
    return;
  }

  it->SetKey("visible", base::Value(visible));
  std::move(callback).Run(true);
}

void BraveWalletService::IsCryptoWalletsInstalled(
    IsCryptoWalletsInstalledCallback callback) {
  if (delegate_)
    delegate_->IsCryptoWalletsInstalled(std::move(callback));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::IsMetaMaskInstalled(
    IsMetaMaskInstalledCallback callback) {
  if (delegate_)
    delegate_->IsMetaMaskInstalled(std::move(callback));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::ImportFromCryptoWallets(
    const std::string& password,
    const std::string& new_password,
    ImportFromCryptoWalletsCallback callback) {
  if (delegate_)
    delegate_->ImportFromCryptoWallets(password, new_password,
                                       std::move(callback));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::ImportFromMetaMask(
    const std::string& password,
    const std::string& new_password,
    ImportFromMetaMaskCallback callback) {
  if (delegate_)
    delegate_->ImportFromMetaMask(password, new_password, std::move(callback));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::GetDefaultWallet(GetDefaultWalletCallback callback) {
  std::move(callback).Run(::brave_wallet::GetDefaultWallet(prefs_));
}

void BraveWalletService::SetDefaultWallet(mojom::DefaultWallet default_wallet) {
  ::brave_wallet::SetDefaultWallet(prefs_, default_wallet);
}

}  // namespace brave_wallet
