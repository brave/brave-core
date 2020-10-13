/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_

#include <string>

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
  void OnBackgroundPreferenceChanged(const std::string& pref_name);
  void OnPreferenceChanged(const std::string& pref_name);
  void SetBraveThemeType(const base::ListValue* args);
  void GetBraveThemeType(const base::ListValue* args);
  void GetIsSuperReferralActive(const base::ListValue* args);
  void GetIsBinanceSupported(const base::ListValue* args);
  void GetIsBraveTogetherSupported(const base::ListValue* args);
  void GetIsGeminiSupported(const base::ListValue* args);
  void GetIsBitcoinDotComSupported(const base::ListValue* args);
  void ToggleTopSitesVisible(const base::ListValue* args);
  void GetShowTopSites(const base::ListValue* args);
  void TopSitesVisibleChanged(const std::string& pref_name);
  void GetIsCryptoDotComSupported(const base::ListValue* args);
  void GetNewTabShowsOptionsList(const base::ListValue* args);
  void ShouldShowNewTabDashboardSettings(const base::ListValue* args);

  Profile* profile_ = nullptr;
  PrefChangeRegistrar local_state_change_registrar_;
  PrefChangeRegistrar profile_state_change_registrar_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_
