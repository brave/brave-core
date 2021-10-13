/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include <utility>
#include <vector>

#include "base/metrics/histogram_macros.h"
#include "base/strings/string_util.h"
#include "base/values.h"
#include "brave/components/brave_stats/browser/brave_stats_updater_util.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_address.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

// kBraveWalletUserAssets
// {
//    "mainnet": {  // network_id
//      [
//        {
//          "contract_address": "",
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
constexpr int kRefreshP3AFrequencyHours = 3;

base::CheckedContiguousIterator<base::Value> FindAsset(
    base::Value* user_assets_list,
    const std::string& contract_address) {
  DCHECK(user_assets_list && user_assets_list->is_list());

  auto iter = std::find_if(
      user_assets_list->GetList().begin(), user_assets_list->GetList().end(),
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
    std::unique_ptr<BraveWalletServiceDelegate> delegate,
    KeyringController* keyring_controller,
    PrefService* prefs)
    : delegate_(std::move(delegate)),
      keyring_controller_(keyring_controller),
      prefs_(prefs),
      weak_ptr_factory_(this) {
  if (delegate_)
    delegate_->AddObserver(this);
  DCHECK(prefs_);

  pref_change_registrar_.Init(prefs_);
  pref_change_registrar_.Add(
      kBraveWalletLastUnlockTime,
      base::BindRepeating(&BraveWalletService::OnWalletUnlockPreferenceChanged,
                          base::Unretained(this)));
  pref_change_registrar_.Add(
      kBraveWalletWeb3Provider,
      base::BindRepeating(&BraveWalletService::OnDefaultWalletChanged,
                          base::Unretained(this)));
  p3a_periodic_timer_.Start(
      FROM_HERE, base::TimeDelta::FromHours(kRefreshP3AFrequencyHours), this,
      &BraveWalletService::OnP3ATimerFired);
  OnP3ATimerFired();  // Also call on startup
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

    value = token.FindKey("token_id");
    if (value && value->is_string()) {
      tokenPtr->token_id = value->GetString();
    }

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
  if (checksum_address.empty() && base::ToLowerASCII(token->symbol) != "eth") {
    std::move(callback).Run(false);
    return;
  }

  const std::string network_id = GetNetworkId(prefs_, chain_id);
  if (network_id.empty()) {
    std::move(callback).Run(false);
    return;
  }

  DictionaryPrefUpdate update(prefs_, kBraveWalletUserAssets);
  base::DictionaryValue* user_assets_pref = update.Get();

  base::Value* user_assets_list = user_assets_pref->FindKey(network_id);
  if (!user_assets_list) {
    user_assets_list = user_assets_pref->SetKey(
        network_id, base::Value(base::Value::Type::LIST));
  }

  auto it = FindAsset(user_assets_list, checksum_address);
  if (it != user_assets_list->GetList().end()) {
    std::move(callback).Run(false);
    return;
  }

  base::Value value(base::Value::Type::DICTIONARY);
  value.SetStringKey("contract_address", checksum_address);
  value.SetStringKey("name", token->name);
  value.SetStringKey("symbol", token->symbol);
  value.SetStringKey("logo", token->logo);
  value.SetBoolKey("is_erc20", token->is_erc20);
  value.SetBoolKey("is_erc721", token->is_erc721);
  value.SetIntKey("decimals", token->decimals);
  value.SetBoolKey("visible", true);
  value.SetStringKey("token_id", token->token_id);

  user_assets_list->Append(std::move(value));
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

  base::Value* user_assets_list = user_assets_pref->FindKey(network_id);
  if (!user_assets_list) {
    std::move(callback).Run(false);
    return;
  }

  user_assets_list->EraseListIter(
      FindAsset(user_assets_list, checksum_address));
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

  base::Value* user_assets_list = user_assets_pref->FindKey(network_id);
  if (!user_assets_list) {
    std::move(callback).Run(false);
    return;
  }

  auto it = FindAsset(user_assets_list, checksum_address);
  if (it == user_assets_list->GetList().end()) {
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
    delegate_->GetImportInfoFromCryptoWallets(
        password, base::BindOnce(&BraveWalletService::OnGetImportInfo,
                                 weak_ptr_factory_.GetWeakPtr(), new_password,
                                 std::move(callback)));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::ImportFromMetaMask(
    const std::string& password,
    const std::string& new_password,
    ImportFromMetaMaskCallback callback) {
  if (delegate_)
    delegate_->GetImportInfoFromMetaMask(
        password, base::BindOnce(&BraveWalletService::OnGetImportInfo,
                                 weak_ptr_factory_.GetWeakPtr(), new_password,
                                 std::move(callback)));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::GetDefaultWallet(GetDefaultWalletCallback callback) {
  std::move(callback).Run(::brave_wallet::GetDefaultWallet(prefs_));
}

void BraveWalletService::SetDefaultWallet(mojom::DefaultWallet default_wallet) {
  auto old_default_wallet = ::brave_wallet::GetDefaultWallet(prefs_);
  if (old_default_wallet != default_wallet) {
    ::brave_wallet::SetDefaultWallet(prefs_, default_wallet);
  }
}

void BraveWalletService::OnDefaultWalletChanged() {
  auto default_wallet = ::brave_wallet::GetDefaultWallet(prefs_);
  for (const auto& observer : observers_) {
    observer->OnDefaultWalletChanged(default_wallet);
  }
}

void BraveWalletService::HasEthereumPermission(
    const std::string& origin_spec,
    const std::string& account,
    HasEthereumPermissionCallback callback) {
  if (delegate_)
    delegate_->HasEthereumPermission(origin_spec, account, std::move(callback));
}

void BraveWalletService::ResetEthereumPermission(
    const std::string& origin_spec,
    const std::string& account,
    ResetEthereumPermissionCallback callback) {
  if (delegate_)
    delegate_->ResetEthereumPermission(origin_spec, account,
                                       std::move(callback));
}

// static
void BraveWalletService::MigrateUserAssetEthContractAddress(
    PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletUserAssetEthContractAddressMigrated))
    return;

  DictionaryPrefUpdate update(prefs, kBraveWalletUserAssets);
  base::DictionaryValue* user_assets_pref = update.Get();

  for (auto user_asset_list : user_assets_pref->DictItems()) {
    auto it = FindAsset(&user_asset_list.second, "eth");
    if (it == user_asset_list.second.GetList().end())
      continue;

    base::DictionaryValue* asset = nullptr;
    if (it->GetAsDictionary(&asset)) {
      const std::string* contract_address =
          asset->FindStringKey("contract_address");
      if (contract_address && *contract_address == "eth") {
        asset->SetStringKey("contract_address", "");
        break;
      }
    }
  }

  prefs->SetBoolean(kBraveWalletUserAssetEthContractAddressMigrated, true);
}

void BraveWalletService::OnP3ATimerFired() {
  base::Time wallet_last_used = prefs_->GetTime(kBraveWalletLastUnlockTime);
  RecordWalletUsage(wallet_last_used);
}

void BraveWalletService::OnWalletUnlockPreferenceChanged(
    const std::string& pref_name) {
  base::Time wallet_last_used = prefs_->GetTime(kBraveWalletLastUnlockTime);
  RecordWalletUsage(wallet_last_used);
}

void BraveWalletService::RecordWalletUsage(base::Time wallet_last_used) {
  uint8_t usage = brave_stats::UsageBitstringFromTimestamp(wallet_last_used);

  bool daily = !!(usage & brave_stats::kIsDailyUser);
  UMA_HISTOGRAM_BOOLEAN(kBraveWalletDailyHistogramName, daily);

  bool weekly = !!(usage & brave_stats::kIsWeeklyUser);
  UMA_HISTOGRAM_BOOLEAN(kBraveWalletWeeklyHistogramName, weekly);

  bool monthly = !!(usage & brave_stats::kIsMonthlyUser);
  UMA_HISTOGRAM_BOOLEAN(kBraveWalletMonthlyHistogramName, monthly);
}

void BraveWalletService::GetActiveOrigin(GetActiveOriginCallback callback) {
  if (delegate_)
    delegate_->GetActiveOrigin(std::move(callback));
  else
    std::move(callback).Run("");
}

void BraveWalletService::GetPendingSignMessageRequest(
    GetPendingSignMessageRequestCallback callback) {
  if (sign_message_requests_.empty()) {
    std::move(callback).Run(-1, "", "");
    return;
  }

  auto request = sign_message_requests_.front();
  std::move(callback).Run(request.id, std::move(request.address),
                          std::move(request.message));
}

void BraveWalletService::NotifySignMessageRequestProcessed(bool approved,
                                                           int id) {
  if (sign_message_requests_.front().id != id) {
    VLOG(1) << "id: " << id << " is not expected, should be "
            << sign_message_requests_.front().id;
    return;
  }
  auto callback = std::move(sign_message_callbacks_.front());
  sign_message_requests_.pop();
  sign_message_callbacks_.pop();

  std::move(callback).Run(approved);
}

void BraveWalletService::AddObserver(
    ::mojo::PendingRemote<mojom::BraveWalletServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

void BraveWalletService::OnActiveOriginChanged(const std::string& origin) {
  for (const auto& observer : observers_) {
    observer->OnActiveOriginChanged(origin);
  }
}

void BraveWalletService::OnGetImportInfo(
    const std::string& new_password,
    base::OnceCallback<void(bool)> callback,
    bool result,
    BraveWalletServiceDelegate::ImportInfo info) {
  if (!result) {
    std::move(callback).Run(false);
    return;
  }

  keyring_controller_->RestoreWallet(
      info.mnemonic, new_password, info.is_legacy_crypto_wallets,
      base::BindOnce(
          [](ImportFromCryptoWalletsCallback callback,
             size_t number_of_accounts, KeyringController* keyring_controller,
             bool is_valid_mnemonic) {
            if (number_of_accounts > 1) {
              keyring_controller->AddAccountsWithDefaultName(
                  number_of_accounts - 1);
            }
            std::move(callback).Run(is_valid_mnemonic);
          },
          std::move(callback), info.number_of_accounts, keyring_controller_));
}

void BraveWalletService::AddSignMessageRequest(
    SignMessageRequest&& request,
    base::OnceCallback<void(bool)> callback) {
  sign_message_requests_.push(std::move(request));
  sign_message_callbacks_.push(std::move(callback));
}

}  // namespace brave_wallet
