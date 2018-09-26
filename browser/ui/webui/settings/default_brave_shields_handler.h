/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_DEFAULT_BRAVE_SHIELDS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_DEFAULT_BRAVE_SHIELDS_HANDLER_H_

#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class Profile;

class DefaultBraveShieldsHandler : public settings::SettingsPageUIHandler {
 public:
  DefaultBraveShieldsHandler() = default;
  ~DefaultBraveShieldsHandler() override = default;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void SetAdControlType(const base::ListValue* args);
  void GetAdControlType(const base::ListValue* args);
  void SetCookieControlType(const base::ListValue* args);
  void GetCookieControlType(const base::ListValue* args);
  void SetFingerprintingControlType(const base::ListValue* args);
  void GetFingerprintingControlType(const base::ListValue* args);
  void SetHTTPSEverywhereControlType(const base::ListValue* args);
  void GetHTTPSEverywhereControlType(const base::ListValue* args);
  void SetNoScriptControlType(const base::ListValue* args);
  void GetNoScriptControlType(const base::ListValue* args);

  Profile* profile_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(DefaultBraveShieldsHandler);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_DEFAULT_BRAVE_SHIELDS_HANDLER_H_
