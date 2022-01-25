/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_DEFAULT_BRAVE_SHIELDS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_DEFAULT_BRAVE_SHIELDS_HANDLER_H_

#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class Profile;

class DefaultBraveShieldsHandler : public settings::SettingsPageUIHandler {
 public:
  DefaultBraveShieldsHandler() = default;
  DefaultBraveShieldsHandler(const DefaultBraveShieldsHandler&) = delete;
  DefaultBraveShieldsHandler& operator=(const DefaultBraveShieldsHandler&) =
      delete;
  ~DefaultBraveShieldsHandler() override = default;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void SetAdControlType(base::Value::ConstListView args);
  void IsAdControlEnabled(base::Value::ConstListView args);
  void SetCosmeticFilteringControlType(base::Value::ConstListView args);
  void IsFirstPartyCosmeticFilteringEnabled(base::Value::ConstListView args);
  void SetCookieControlType(base::Value::ConstListView args);
  void GetCookieControlType(base::Value::ConstListView args);
  void SetFingerprintingControlType(base::Value::ConstListView args);
  void GetFingerprintingControlType(base::Value::ConstListView args);
  void SetHTTPSEverywhereEnabled(base::Value::ConstListView args);
  void GetHTTPSEverywhereEnabled(base::Value::ConstListView args);
  void SetNoScriptControlType(base::Value::ConstListView args);
  void GetNoScriptControlType(base::Value::ConstListView args);

  Profile* profile_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_DEFAULT_BRAVE_SHIELDS_HANDLER_H_
