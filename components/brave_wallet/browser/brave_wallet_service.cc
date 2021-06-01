/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_service.h"

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
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
  registry->RegisterBooleanPref(kShowWalletIconOnToolbar, true);
  registry->RegisterBooleanPref(kBraveWalletBackupComplete, false);
}

brave_wallet::EthJsonRpcController* BraveWalletService::rpc_controller() const {
  return rpc_controller_.get();
}

brave_wallet::KeyringController* BraveWalletService::keyring_controller()
    const {
  return keyring_controller_.get();
}

bool BraveWalletService::IsWalletBackedUp() const {
  return prefs_->GetBoolean(kBraveWalletBackupComplete);
}

void BraveWalletService::NotifyWalletBackupComplete() {
  prefs_->SetBoolean(kBraveWalletBackupComplete, true);
}

}  // namespace brave_wallet
