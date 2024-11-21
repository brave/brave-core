/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_HANDLER_H_

#include "base/values.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class BraveAccountHandler : public settings::SettingsPageUIHandler {
 public:
  BraveAccountHandler();
  ~BraveAccountHandler() override;
  BraveAccountHandler(const BraveAccountHandler&) = delete;
  BraveAccountHandler& operator=(const BraveAccountHandler&) = delete;

 private:
  // WebUIMessageHandler overrides:
  void RegisterMessages() override;
  // SettingsPageUIHandler overrides:
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void GetPasswordStrength(const base::Value::List& args);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_HANDLER_H_
