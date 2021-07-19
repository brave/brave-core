/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include <string>
#include <vector>

#include "brave/components/brave_wallet/browser/asset_ratio_controller.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_observer.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/eth_tx_controller.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/swap_controller.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_wallet {

BraveWalletService::BraveWalletService(
    PrefService* prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : prefs_(prefs) {
  rpc_controller_ = std::make_unique<brave_wallet::EthJsonRpcController>(
      brave_wallet::Network::kMainnet, url_loader_factory);
  keyring_controller_ =
      std::make_unique<brave_wallet::KeyringController>(prefs);
  tx_controller_ = std::make_unique<brave_wallet::EthTxController>(
      rpc_controller_.get(), keyring_controller_.get(), prefs);
  asset_ratio_controller_ =
      std::make_unique<brave_wallet::AssetRatioController>(url_loader_factory);
  swap_controller_ =
      std::make_unique<brave_wallet::SwapController>(url_loader_factory);
}

BraveWalletService::~BraveWalletService() {}

void BraveWalletService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(
      kBraveWalletWeb3Provider,
      static_cast<int>(brave_wallet::IsNativeWalletEnabled()
                           ? brave_wallet::Web3ProviderTypes::BRAVE_WALLET
                           : brave_wallet::Web3ProviderTypes::ASK));
  registry->RegisterStringPref(kBraveWalletPasswordEncryptorSalt, "");
  registry->RegisterStringPref(kBraveWalletPasswordEncryptorNonce, "");
  registry->RegisterStringPref(kBraveWalletEncryptedMnemonic, "");
  registry->RegisterIntegerPref(kBraveWalletDefaultKeyringAccountNum, 0);
  registry->RegisterDictionaryPref(kBraveWalletTransactions);
  registry->RegisterBooleanPref(kShowWalletIconOnToolbar, true);
  registry->RegisterBooleanPref(kBraveWalletBackupComplete, false);
  registry->RegisterTimePref(kBraveWalletLastUnlockTime, base::Time());
  registry->RegisterListPref(kBraveWalletAccountNames);
}

brave_wallet::EthJsonRpcController* BraveWalletService::rpc_controller() const {
  return rpc_controller_.get();
}

brave_wallet::KeyringController* BraveWalletService::keyring_controller()
    const {
  return keyring_controller_.get();
}
brave_wallet::EthTxController* BraveWalletService::tx_controller() const {
  return tx_controller_.get();
}

brave_wallet::AssetRatioController* BraveWalletService::asset_ratio_controller()
    const {
  return asset_ratio_controller_.get();
}

brave_wallet::SwapController* BraveWalletService::swap_controller() const {
  return swap_controller_.get();
}

std::vector<std::string> BraveWalletService::WalletAccountNames() const {
  std::vector<std::string> account_names;
  for (const auto& account_name_value :
       prefs_->Get(kBraveWalletAccountNames)->GetList()) {
    const std::string* account_name = account_name_value.GetIfString();
    DCHECK(account_name) << "account name type should be string";
    account_names.push_back(*account_name);
  }
  return account_names;
}

void BraveWalletService::SetInitialAccountNames(
    const std::vector<std::string>& account_names) {
  std::vector<base::Value> account_names_list;
  for (const std::string& name : account_names) {
    account_names_list.push_back(base::Value(name));
  }
  prefs_->Set(kBraveWalletAccountNames, base::Value(account_names_list));
}

void BraveWalletService::AddNewAccountName(const std::string& account_name) {
  ListPrefUpdate update(prefs_, kBraveWalletAccountNames);
  update->Append(base::Value(account_name));
}

bool BraveWalletService::IsWalletBackedUp() const {
  return prefs_->GetBoolean(kBraveWalletBackupComplete);
}

void BraveWalletService::NotifyWalletBackupComplete() {
  prefs_->SetBoolean(kBraveWalletBackupComplete, true);
}

void BraveWalletService::AddObserver(BraveWalletServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void BraveWalletService::RemoveObserver(BraveWalletServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void BraveWalletService::NotifyShowEthereumPermissionPrompt(
    int32_t tab_id,
    const std::vector<std::string>& accounts,
    const std::string& origin) {
  if (!panel_handler_ready_) {
    // Only need to save the latest request since we only need to show the
    // latest one once the panel become ready.
    pending_permission_request_ =
        std::make_unique<PermissionRequest>(tab_id, accounts, origin);
    return;
  }

  for (auto& observer : observers_)
    observer.OnShowEthereumPermissionPrompt(tab_id, accounts, origin);
}

BraveWalletService::PermissionRequest::PermissionRequest(
    int32_t tab_id,
    const std::vector<std::string>& accounts,
    const std::string& origin)
    : tab_id(tab_id), accounts(accounts), origin(origin) {}

BraveWalletService::PermissionRequest::~PermissionRequest() = default;

void BraveWalletService::SetPanelHandlerReady(bool ready) {
  panel_handler_ready_ = ready;

  if (ready && pending_permission_request_) {
    NotifyShowEthereumPermissionPrompt(pending_permission_request_->tab_id,
                                       pending_permission_request_->accounts,
                                       pending_permission_request_->origin);
    pending_permission_request_.reset();
  }
}

}  // namespace brave_wallet
