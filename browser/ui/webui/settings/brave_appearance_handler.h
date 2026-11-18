/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_

#include <string>

#include "base/callback_list.h"
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

  void OnPreferenceChanged(const std::string& pref_name);
  void GetNewTabShowsOptionsList(const base::ListValue& args);
  void ShouldShowNewTabDashboardSettings(const base::ListValue& args);
  // Vertical tabs can't be switched while the browser window is fullscreen on
  // macOS (see tabs::utils::IsVerticalTabToggleEnabled()). The settings toggle
  // then calls ToggleVerticalTabs() instead of setting the pref, which defers
  // the switch and shows a toast.
  void GetIsVerticalTabsToggleEnabled(const base::ListValue& args);
  bool IsVerticalTabsToggleEnabled();
  void ToggleVerticalTabs(const base::ListValue& args);

  // Compact mode is incompatible with immersive fullscreen (see
  // WindowFeatureController::UsesImmersiveFullscreenMode()), so the compact
  // mode toggle is disabled while the browser window is fullscreen.
  void GetIsCompactModeToggleEnabled(const base::ListValue& args);
  void OnFullscreenStateChanged();
  bool IsCompactModeToggleEnabled();

  raw_ptr<Profile> profile_ = nullptr;
  PrefChangeRegistrar profile_state_change_registrar_;

  base::CallbackListSubscription fullscreen_subscription_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_
