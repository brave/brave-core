/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_WALLET_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_WALLET_HANDLER_H_

#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class Profile;

class BraveWalletHandler : public settings::SettingsPageUIHandler {
 public:
  BraveWalletHandler() = default;
  ~BraveWalletHandler() override = default;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void GetAutoLockMinutes(base::Value::ConstListView args);
  BraveWalletHandler(const BraveWalletHandler&) = delete;
  BraveWalletHandler& operator=(const BraveWalletHandler&) = delete;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_WALLET_HANDLER_H_
