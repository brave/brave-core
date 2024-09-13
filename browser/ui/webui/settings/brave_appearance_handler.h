/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/prefs/pref_change_registrar.h"

class Profile;

class BraveAppearanceHandler : public settings::SettingsPageUIHandler {
 public:
  BraveAppearanceHandler();
  ~BraveAppearanceHandler() override;

  BraveAppearanceHandler(const BraveAppearanceHandler&) = delete;
  BraveAppearanceHandler& operator=(const BraveAppearanceHandler&) = delete;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void OnBraveDarkModeChanged();
  void OnPreferenceChanged(const std::string& pref_name);
  void SetBraveThemeType(const base::Value::List& args);
  void GetBraveThemeType(const base::Value::List& args);
  void GetNewTabShowsOptionsList(const base::Value::List& args);
  void ShouldShowNewTabDashboardSettings(const base::Value::List& args);

  raw_ptr<Profile> profile_ = nullptr;
  PrefChangeRegistrar local_state_change_registrar_;
  PrefChangeRegistrar profile_state_change_registrar_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_
