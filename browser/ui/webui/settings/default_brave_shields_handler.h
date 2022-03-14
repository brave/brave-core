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

  void SetAdControlType(const base::Value::List& args);
  void IsAdControlEnabled(const base::Value::List& args);
  void SetCosmeticFilteringControlType(const base::Value::List& args);
  void IsFirstPartyCosmeticFilteringEnabled(const base::Value::List& args);
  void SetCookieControlType(const base::Value::List& args);
  void GetCookieControlType(const base::Value::List& args);
  void SetFingerprintingControlType(const base::Value::List& args);
  void GetFingerprintingControlType(const base::Value::List& args);
  void SetHTTPSEverywhereEnabled(const base::Value::List& args);
  void GetHTTPSEverywhereEnabled(const base::Value::List& args);
  void SetNoScriptControlType(const base::Value::List& args);
  void GetNoScriptControlType(const base::Value::List& args);

  Profile* profile_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_DEFAULT_BRAVE_SHIELDS_HANDLER_H_
