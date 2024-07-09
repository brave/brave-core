/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_WALLET_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_WALLET_HANDLER_H_

#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class PrefService;
class Profile;
class TestBraveWalletHandler;

class BraveWalletHandler : public settings::SettingsPageUIHandler {
 public:
  BraveWalletHandler();
  ~BraveWalletHandler() override;
  BraveWalletHandler(const BraveWalletHandler&) = delete;
  BraveWalletHandler& operator=(const BraveWalletHandler&) = delete;

  void SetChainCallbackForTesting(base::OnceClosure callback) {
    chain_callback_for_testing_ = std::move(callback);
  }

 private:
  friend TestBraveWalletHandler;
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void GetAutoLockMinutes(const base::Value::List& args);
  void GetSolanaProviderOptions(const base::Value::List& args);
  void GetTransactionSimulationOptInStatusOptions(
      const base::Value::List& args);
  void RemoveChain(const base::Value::List& args);
  void ResetChain(const base::Value::List& args);
  void GetNetworksList(const base::Value::List& args);
  void GetPrepopulatedNetworksList(const base::Value::List& args);
  void AddChain(const base::Value::List& args);
  void SetDefaultNetwork(const base::Value::List& args);
  void AddHiddenNetwork(const base::Value::List& args);
  void RemoveHiddenNetwork(const base::Value::List& args);
  void IsBitcoinEnabled(const base::Value::List& args);
  void IsZCashEnabled(const base::Value::List& args);
  void IsTransactionSimulationsEnabled(const base::Value::List& args);
  void SetWalletInPrivateWindowsEnabled(const base::Value::List& args);
  void GetWalletInPrivateWindowsEnabled(const base::Value::List& args);

  PrefService* GetPrefs();

  void OnAddChain(base::Value javascript_callback,
                  const std::string& chain_id,
                  brave_wallet::mojom::ProviderError error,
                  const std::string& error_message);

  base::OnceClosure chain_callback_for_testing_;
  base::WeakPtrFactory<BraveWalletHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_WALLET_HANDLER_H_
