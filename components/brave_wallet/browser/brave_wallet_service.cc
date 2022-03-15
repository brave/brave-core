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
#include "brave/components/brave_wallet/browser/blockchain_registry.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_service.h"
#include "brave/components/brave_wallet/common/brave_wallet_response_helpers.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/value_conversion_utils.h"
#include "brave/components/brave_wallet/common/web3_provider_constants.h"
#include "brave/components/weekly_storage/weekly_storage.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "ui/base/l10n/l10n_util.h"

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
constexpr int kRefreshP3AFrequencyHours = 24;

// T could be base::Value or const base::Value
template <typename T>
base::CheckedContiguousIterator<T> FindAsset(
    T* user_assets_list,
    const std::string& contract_address,
    const std::string& token_id,
    bool is_erc721) {
  DCHECK(user_assets_list && user_assets_list->is_list());

  auto iter = std::find_if(
      user_assets_list->GetList().begin(), user_assets_list->GetList().end(),
      [&](const base::Value& value) {
        if (!value.is_dict()) {
          return false;
        }
        const std::string* address = value.FindStringKey("contract_address");
        bool found = address && *address == contract_address;

        if (found && is_erc721) {
          const std::string* token_id_ptr = value.FindStringKey("token_id");
          found = token_id_ptr && *token_id_ptr == token_id;
        }

        return found;
      });

  return iter;
}

}  // namespace

namespace brave_wallet {

BraveWalletService::BraveWalletService(
    std::unique_ptr<BraveWalletServiceDelegate> delegate,
    KeyringService* keyring_service,
    JsonRpcService* json_rpc_service,
    TxService* tx_service,
    PrefService* prefs)
    : delegate_(std::move(delegate)),
      keyring_service_(keyring_service),
      json_rpc_service_(json_rpc_service),
      tx_service_(tx_service),
      prefs_(prefs),
      brave_wallet_p3a_(this, keyring_service, prefs),
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
      kDefaultWallet2,
      base::BindRepeating(&BraveWalletService::OnDefaultWalletChanged,
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
      kBraveWalletSelectedNetworks,
      base::BindRepeating(&BraveWalletService::OnNetworkChanged,
                          weak_ptr_factory_.GetWeakPtr()));

  p3a_periodic_timer_.Start(FROM_HERE, base::Hours(kRefreshP3AFrequencyHours),
                            this, &BraveWalletService::OnP3ATimerFired);
  OnP3ATimerFired();  // Also call on startup
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
  const std::string network_id =
      GetNetworkId(prefs_, mojom::CoinType::ETH, chain_id);
  if (network_id.empty()) {
    std::move(callback).Run(std::vector<mojom::BlockchainTokenPtr>());
    return;
  }

  const base::Value* user_assets_dict =
      prefs_->GetDictionary(kBraveWalletUserAssets);
  if (!user_assets_dict) {
    std::move(callback).Run(std::vector<mojom::BlockchainTokenPtr>());
    return;
  }

  const base::Value* tokens = user_assets_dict->FindKey(network_id);
  if (!tokens) {
    std::move(callback).Run(std::vector<mojom::BlockchainTokenPtr>());
    return;
  }

  std::vector<mojom::BlockchainTokenPtr> result;
  for (const auto& token : tokens->GetList()) {
    mojom::BlockchainTokenPtr tokenPtr = ValueToBlockchainToken(token);
    if (tokenPtr)
      result.push_back(std::move(tokenPtr));
  }

  std::move(callback).Run(std::move(result));
}

bool BraveWalletService::AddUserAsset(mojom::BlockchainTokenPtr token,
                                      const std::string& chain_id) {
  absl::optional<std::string> optional_checksum_address =
      GetChecksumAddress(token->contract_address, chain_id);
  if (!optional_checksum_address)
    return false;

  // Can be empty string for native asset.
  const std::string checksum_address = optional_checksum_address.value();

  const std::string network_id =
      GetNetworkId(prefs_, mojom::CoinType::ETH, chain_id);
  if (network_id.empty())
    return false;

  // Verify input token ID for ERC721.
  if (token->is_erc721) {
    uint256_t token_id_uint = 0;
    if (!HexValueToUint256(token->token_id, &token_id_uint)) {
      return false;
    }
  }

  DictionaryPrefUpdate update(prefs_, kBraveWalletUserAssets);
  base::Value* user_assets_pref = update.Get();

  base::Value* user_assets_list = user_assets_pref->FindKey(network_id);
  if (!user_assets_list) {
    user_assets_list = user_assets_pref->SetKey(
        network_id, base::Value(base::Value::Type::LIST));
  }

  auto it = FindAsset(user_assets_list, checksum_address, token->token_id,
                      token->is_erc721);
  if (it != user_assets_list->GetList().end())
    return false;

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
  value.SetStringKey("coingecko_id", token->coingecko_id);

  user_assets_list->Append(std::move(value));
  return true;
}

void BraveWalletService::AddUserAsset(mojom::BlockchainTokenPtr token,
                                      const std::string& chain_id,
                                      AddUserAssetCallback callback) {
  std::move(callback).Run(AddUserAsset(std::move(token), chain_id));
}

void BraveWalletService::RemoveUserAsset(mojom::BlockchainTokenPtr token,
                                         const std::string& chain_id,
                                         RemoveUserAssetCallback callback) {
  std::move(callback).Run(RemoveUserAsset(std::move(token), chain_id));
}

bool BraveWalletService::RemoveUserAsset(mojom::BlockchainTokenPtr token,
                                         const std::string& chain_id) {
  absl::optional<std::string> optional_checksum_address =
      GetChecksumAddress(token->contract_address, chain_id);
  if (!optional_checksum_address)
    return false;
  const std::string checksum_address = optional_checksum_address.value();

  const std::string network_id =
      GetNetworkId(prefs_, mojom::CoinType::ETH, chain_id);
  if (network_id.empty())
    return false;

  DictionaryPrefUpdate update(prefs_, kBraveWalletUserAssets);
  base::Value* user_assets_pref = update.Get();

  base::Value* user_assets_list = user_assets_pref->FindKey(network_id);
  if (!user_assets_list)
    return false;

  user_assets_list->EraseListIter(FindAsset(user_assets_list, checksum_address,
                                            token->token_id, token->is_erc721));
  return true;
}

void BraveWalletService::SetUserAssetVisible(
    mojom::BlockchainTokenPtr token,
    const std::string& chain_id,
    bool visible,
    SetUserAssetVisibleCallback callback) {
  std::move(callback).Run(
      SetUserAssetVisible(std::move(token), chain_id, visible));
}

bool BraveWalletService::SetUserAssetVisible(mojom::BlockchainTokenPtr token,
                                             const std::string& chain_id,
                                             bool visible) {
  DCHECK(token);

  absl::optional<std::string> optional_checksum_address =
      GetChecksumAddress(token->contract_address, chain_id);
  if (!optional_checksum_address)
    return false;

  const std::string checksum_address = optional_checksum_address.value();

  const std::string network_id =
      GetNetworkId(prefs_, mojom::CoinType::ETH, chain_id);
  if (network_id.empty())
    return false;

  DictionaryPrefUpdate update(prefs_, kBraveWalletUserAssets);
  base::Value* user_assets_pref = update.Get();

  base::Value* user_assets_list = user_assets_pref->FindKey(network_id);
  if (!user_assets_list)
    return false;

  auto it = FindAsset(user_assets_list, checksum_address, token->token_id,
                      token->is_erc721);
  if (it == user_assets_list->GetList().end())
    return false;

  it->SetKey("visible", base::Value(visible));
  return true;
}

mojom::BlockchainTokenPtr BraveWalletService::GetUserAsset(
    const std::string& contract_address,
    const std::string& token_id,
    bool is_erc721,
    const std::string& chain_id) {
  absl::optional<std::string> optional_checksum_address =
      GetChecksumAddress(contract_address, chain_id);
  if (!optional_checksum_address)
    return nullptr;
  const std::string checksum_address = optional_checksum_address.value();
  const std::string network_id =
      GetNetworkId(prefs_, mojom::CoinType::ETH, chain_id);
  if (network_id.empty())
    return nullptr;

  const base::Value* user_assets_dict =
      prefs_->GetDictionary(kBraveWalletUserAssets);
  if (!user_assets_dict)
    return nullptr;

  const base::Value* user_assets_list =
      user_assets_dict->FindListKey(network_id);
  if (!user_assets_list)
    return nullptr;

  auto it = FindAsset(user_assets_list, checksum_address, token_id, is_erc721);
  if (it == user_assets_list->GetList().end())
    return nullptr;

  return ValueToBlockchainToken(*it);
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

void BraveWalletService::GetDefaultWallet(GetDefaultWalletCallback callback) {
  std::move(callback).Run(::brave_wallet::GetDefaultWallet(prefs_));
}

void BraveWalletService::SetDefaultWallet(mojom::DefaultWallet default_wallet) {
  auto old_default_wallet = ::brave_wallet::GetDefaultWallet(prefs_);
  if (old_default_wallet != default_wallet) {
    ::brave_wallet::SetDefaultWallet(prefs_, default_wallet);
  }
}

void BraveWalletService::GetDefaultBaseCurrency(
    GetDefaultBaseCurrencyCallback callback) {
  std::move(callback).Run(::brave_wallet::GetDefaultBaseCurrency(prefs_));
}

void BraveWalletService::SetDefaultBaseCurrency(const std::string& currency) {
  auto old_default_currency = ::brave_wallet::GetDefaultBaseCurrency(prefs_);
  if (old_default_currency != currency) {
    ::brave_wallet::SetDefaultBaseCurrency(prefs_, currency);
  }
}

void BraveWalletService::GetDefaultBaseCryptocurrency(
    GetDefaultBaseCryptocurrencyCallback callback) {
  std::move(callback).Run(::brave_wallet::GetDefaultBaseCryptocurrency(prefs_));
}

void BraveWalletService::SetDefaultBaseCryptocurrency(
    const std::string& cryptocurrency) {
  auto old_default_cryptocurrency =
      ::brave_wallet::GetDefaultBaseCryptocurrency(prefs_);
  if (old_default_cryptocurrency != cryptocurrency) {
    ::brave_wallet::SetDefaultBaseCryptocurrency(prefs_, cryptocurrency);
  }
}

void BraveWalletService::OnDefaultWalletChanged() {
  auto default_wallet = ::brave_wallet::GetDefaultWallet(prefs_);
  for (const auto& observer : observers_) {
    observer->OnDefaultWalletChanged(default_wallet);
  }
}

void BraveWalletService::OnDefaultBaseCurrencyChanged() {
  auto value = ::brave_wallet::GetDefaultBaseCurrency(prefs_);
  for (const auto& observer : observers_) {
    observer->OnDefaultBaseCurrencyChanged(value);
  }
}

void BraveWalletService::OnDefaultBaseCryptocurrencyChanged() {
  auto value = ::brave_wallet::GetDefaultBaseCryptocurrency(prefs_);
  for (const auto& observer : observers_) {
    observer->OnDefaultBaseCryptocurrencyChanged(value);
  }
}

void BraveWalletService::OnNetworkListChanged() {
  for (const auto& observer : observers_) {
    observer->OnNetworkListChanged();
  }
}

void BraveWalletService::AddEthereumPermission(
    const std::string& origin_spec,
    const std::string& account,
    AddEthereumPermissionCallback callback) {
  if (delegate_)
    delegate_->AddEthereumPermission(origin_spec, account, std::move(callback));
  else
    std::move(callback).Run(false);
}

void BraveWalletService::HasEthereumPermission(
    const std::string& origin_spec,
    const std::string& account,
    HasEthereumPermissionCallback callback) {
  if (delegate_)
    delegate_->HasEthereumPermission(origin_spec, account, std::move(callback));
  else
    std::move(callback).Run(false, false);
}

void BraveWalletService::ResetEthereumPermission(
    const std::string& origin_spec,
    const std::string& account,
    ResetEthereumPermissionCallback callback) {
  if (delegate_)
    delegate_->ResetEthereumPermission(origin_spec, account,
                                       std::move(callback));
  else
    std::move(callback).Run(false);
}

// static
void BraveWalletService::MigrateUserAssetEthContractAddress(
    PrefService* prefs) {
  if (prefs->GetBoolean(kBraveWalletUserAssetEthContractAddressMigrated))
    return;

  DictionaryPrefUpdate update(prefs, kBraveWalletUserAssets);
  base::Value* user_assets_pref = update.Get();

  for (auto user_asset_list : user_assets_pref->DictItems()) {
    auto it = FindAsset(&user_asset_list.second, "eth", "", false);
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
  RecordWalletUsage();
}

void BraveWalletService::OnWalletUnlockPreferenceChanged(
    const std::string& pref_name) {
  RecordWalletUsage();
}

void BraveWalletService::RecordWalletUsage() {
  VLOG(1) << "Wallet P3A: starting report";
  base::Time wallet_last_used = prefs_->GetTime(kBraveWalletLastUnlockTime);
  base::Time first_p3a_report = prefs_->GetTime(kBraveWalletP3AFirstReportTime);
  base::Time last_p3a_report = prefs_->GetTime(kBraveWalletP3ALastReportTime);

  VLOG(1) << "Wallet P3A: first report: " << first_p3a_report
          << " last_report: " << last_p3a_report;

  WeeklyStorage weekly_store(prefs_, kBraveWalletP3AWeeklyStorage);
  if (wallet_last_used > last_p3a_report) {
    weekly_store.ReplaceTodaysValueIfGreater(1);
    VLOG(1) << "Wallet P3A: Reporting day in week, curr days in week val: "
            << weekly_store.GetWeeklySum();
  }

  WriteStatsToHistogram(wallet_last_used, first_p3a_report, last_p3a_report,
                        weekly_store.GetWeeklySum());

  prefs_->SetTime(kBraveWalletP3ALastReportTime, base::Time::Now());
  if (first_p3a_report.is_null())
    prefs_->SetTime(kBraveWalletP3AFirstReportTime, base::Time::Now());
}

void BraveWalletService::WriteStatsToHistogram(base::Time wallet_last_used,
                                               base::Time first_p3a_report,
                                               base::Time last_p3a_report,
                                               unsigned use_days_in_week) {
  base::Time::Exploded now_exp;
  base::Time::Exploded last_report_exp;
  base::Time::Exploded last_used_exp;
  base::Time::Now().LocalExplode(&now_exp);
  last_p3a_report.LocalExplode(&last_report_exp);
  wallet_last_used.LocalExplode(&last_used_exp);

  bool new_month_detected =
      !last_p3a_report.is_null() && (now_exp.year != last_report_exp.year ||
                                     now_exp.month != last_report_exp.month);

  if (new_month_detected) {
    bool used_last_month = !wallet_last_used.is_null() &&
                           last_report_exp.month == last_used_exp.month &&
                           last_report_exp.year == last_used_exp.year;
    VLOG(1) << "Wallet P3A: New month detected. used last month: "
            << used_last_month;
    UMA_HISTOGRAM_BOOLEAN(kBraveWalletMonthlyHistogramName, used_last_month);
  }

  bool week_passed_since_install =
      !first_p3a_report.is_null() &&
      (base::Time::Now() - first_p3a_report).InDays() >= 7;
  if (week_passed_since_install) {
    VLOG(1) << "Wallet P3A: recording daily/weekly. weekly_sum: "
            << use_days_in_week;
    UMA_HISTOGRAM_EXACT_LINEAR(kBraveWalletWeeklyHistogramName,
                               use_days_in_week, 8);
  } else {
    VLOG(1) << "Wallet P3A: Need 7 days of reports before recording "
               "daily/weekly, skipping";
  }
}

void BraveWalletService::GetActiveOrigin(GetActiveOriginCallback callback) {
  if (delegate_)
    delegate_->GetActiveOrigin(std::move(callback));
  else
    std::move(callback).Run("");
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

void BraveWalletService::NotifySignMessageRequestProcessed(bool approved,
                                                           int id) {
  if (sign_message_requests_.empty() ||
      sign_message_requests_.front()->id != id) {
    VLOG(1) << "id: " << id << " is not expected, should be "
            << sign_message_requests_.front()->id;
    return;
  }
  auto callback = std::move(sign_message_callbacks_.front());
  sign_message_requests_.pop_front();
  sign_message_callbacks_.pop_front();

  std::move(callback).Run(approved, std::string(), std::string());
}

void BraveWalletService::NotifySignMessageHardwareRequestProcessed(
    bool approved,
    int id,
    const std::string& signature,
    const std::string& error) {
  if (sign_message_requests_.empty() ||
      sign_message_requests_.front()->id != id) {
    VLOG(1) << "id: " << id << " is not expected, should be "
            << sign_message_requests_.front()->id;
    return;
  }
  auto callback = std::move(sign_message_callbacks_.front());
  sign_message_requests_.pop_front();
  sign_message_callbacks_.pop_front();

  std::move(callback).Run(approved, signature, error);
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
  sign_message_requests_.push_back(std::move(request));
  sign_message_callbacks_.push_back(std::move(callback));
}

void BraveWalletService::AddSuggestTokenRequest(
    mojom::AddSuggestTokenRequestPtr request,
    RequestNewCallback callback,
    base::Value id) {
  // wallet_watchAsset currently only expect non-empty contract address and
  // only ERC20 type.
  DCHECK(!request->token->contract_address.empty());
  DCHECK(request->token->is_erc20 && !request->token->is_erc721);

  if (add_suggest_token_requests_.contains(request->token->contract_address)) {
    std::unique_ptr<base::Value> formed_response;
    bool reject = true;
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kInvalidParams,
        l10n_util::GetStringUTF8(IDS_WALLET_ALREADY_IN_PROGRESS_ERROR));
    std::move(callback).Run(std::move(id), std::move(*formed_response), reject,
                            "", false);
    return;
  }

  const std::string addr = request->token->contract_address;
  const std::string chain_id = GetCurrentChainId(prefs_, mojom::CoinType::ETH);

  // Priority of token source:
  //     1. User asset list
  //     2. BlockchainRegistry
  //     3. wallet_watchAsset request
  mojom::BlockchainTokenPtr token =
      GetUserAsset(request->token->contract_address, request->token->token_id,
                   request->token->is_erc721, chain_id);

  if (!token)
    token =
        BlockchainRegistry::GetInstance()->GetTokenByContract(chain_id, addr);

  if (token)
    request->token = std::move(token);

  add_suggest_token_requests_[addr] = std::move(request);
  add_suggest_token_callbacks_[addr] = std::move(callback);
  add_suggest_token_ids_[addr] = std::move(id);
}

void BraveWalletService::GetPendingAddSuggestTokenRequests(
    GetPendingAddSuggestTokenRequestsCallback callback) {
  std::vector<mojom::AddSuggestTokenRequestPtr> requests;
  for (const auto& request : add_suggest_token_requests_) {
    requests.push_back(request.second.Clone());
  }
  std::move(callback).Run(std::move(requests));
}

void BraveWalletService::NotifyAddSuggestTokenRequestsProcessed(
    bool approved,
    const std::vector<std::string>& contract_addresses) {
  const std::string chain_id = GetCurrentChainId(prefs_, mojom::CoinType::ETH);
  for (const auto& addr : contract_addresses) {
    if (add_suggest_token_requests_.contains(addr) &&
        add_suggest_token_callbacks_.contains(addr) &&
        add_suggest_token_ids_.contains(addr)) {
      auto callback = std::move(add_suggest_token_callbacks_[addr]);

      std::unique_ptr<base::Value> formed_response;
      bool reject = false;
      if (approved &&
          !AddUserAsset(add_suggest_token_requests_[addr]->token.Clone(),
                        chain_id) &&
          !SetUserAssetVisible(add_suggest_token_requests_[addr]->token.Clone(),
                               chain_id, true)) {
        add_suggest_token_requests_.erase(addr);
        add_suggest_token_callbacks_.erase(addr);
        add_suggest_token_ids_.erase(addr);

        formed_response = GetProviderErrorDictionary(
            mojom::ProviderError::kInternalError,
            l10n_util::GetStringUTF8(IDS_WALLET_INTERNAL_ERROR));
        reject = true;
        std::move(callback).Run(std::move(add_suggest_token_ids_[addr]),
                                std::move(*formed_response), reject, "", false);
        continue;
      }

      add_suggest_token_requests_.erase(addr);
      add_suggest_token_callbacks_.erase(addr);
      add_suggest_token_ids_.erase(addr);
      formed_response = base::Value::ToUniquePtrValue(base::Value(approved));
      reject = false;
      std::move(callback).Run(std::move(add_suggest_token_ids_[addr]),
                              std::move(*formed_response), reject, "", false);
    }
  }
}

void BraveWalletService::CancelAllSuggestedTokenCallbacks() {
  add_suggest_token_requests_.clear();
  // Reject pending suggest token requests when network changed.
  std::unique_ptr<base::Value> formed_response;
  bool reject = true;
  for (auto& callback : add_suggest_token_callbacks_) {
    formed_response = GetProviderErrorDictionary(
        mojom::ProviderError::kUserRejectedRequest,
        l10n_util::GetStringUTF8(IDS_WALLET_USER_REJECTED_REQUEST));
    std::move(callback.second)
        .Run(std::move(add_suggest_token_ids_[callback.first]),
             std::move(*formed_response), reject, "", false);
  }
  add_suggest_token_callbacks_.clear();
  add_suggest_token_ids_.clear();
}

void BraveWalletService::CancelAllSignMessageCallbacks() {
  while (!sign_message_requests_.empty()) {
    auto callback = std::move(sign_message_callbacks_.front());
    sign_message_requests_.pop_front();
    sign_message_callbacks_.pop_front();
    std::move(callback).Run(false, std::string(), std::string());
  }
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
  ClearBraveWalletServicePrefs(prefs_);
  CancelAllSuggestedTokenCallbacks();
  CancelAllSignMessageCallbacks();

  if (keyring_service_)
    keyring_service_->Reset();
}

}  // namespace brave_wallet
