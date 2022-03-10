/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_WALLET_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_WALLET_HANDLER_H_

#include <string>
#include <utility>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class Profile;
class TestBraveWalletHandler;

class BraveWalletHandler : public settings::SettingsPageUIHandler {
 public:
  BraveWalletHandler();
  ~BraveWalletHandler() override;

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
  void RemoveEthereumChain(const base::Value::List& args);
  void GetCustomNetworksList(const base::Value::List& args);
  void AddEthereumChain(const base::Value::List& args);
  void SetActiveNetwork(const base::Value::List& args);

  // Temporary callbacks that we need to keep for Chromium 100 so that we can
  // use them with RegisterMessageCallback() before that method gets migrated to
  // expect a const base::Value::List& along with Chromium 101 (see CL 3483819).
  // https://chromium-review.googlesource.com/c/chromium/src/+/3483819
  void GetAutoLockMinutesDeprecated(base::Value::ConstListView args);
  void RemoveEthereumChainDeprecated(base::Value::ConstListView args);
  void GetCustomNetworksListDeprecated(base::Value::ConstListView args);
  void AddEthereumChainDeprecated(base::Value::ConstListView args);
  void SetActiveNetworkDeprecated(base::Value::ConstListView args);

  BraveWalletHandler(const BraveWalletHandler&) = delete;
  BraveWalletHandler& operator=(const BraveWalletHandler&) = delete;

  void OnAddEthereumChain(base::Value javascript_callback,
                          const std::string& chain_id,
                          brave_wallet::mojom::ProviderError error,
                          const std::string& error_message);
  base::OnceClosure chain_callback_for_testing_;
  base::WeakPtrFactory<BraveWalletHandler> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_WALLET_HANDLER_H_
