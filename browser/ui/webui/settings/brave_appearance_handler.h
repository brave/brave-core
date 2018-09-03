/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_

#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class Profile;

class BraveAppearanceHandler : public settings::SettingsPageUIHandler {
 public:
  BraveAppearanceHandler() = default;
  ~BraveAppearanceHandler() override = default;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void SetBraveThemeType(const base::ListValue* args);
  void GetBraveThemeType(const base::ListValue* args);

  Profile* profile_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BraveAppearanceHandler);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_
