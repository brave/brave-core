/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/prefs/pref_change_registrar.h"

// Handles the queries from the Brave Origin settings page which
// allows folks to turn on or off features (and can prompt for restart).
class BraveOriginHandler : public settings::SettingsPageUIHandler {
 public:
  explicit BraveOriginHandler(Profile* profile);
  ~BraveOriginHandler() override;

 private:
  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

  void HandleGetInitialState(const base::Value::List& args);
  void HandleToggleValue(const base::Value::List& args);
  void HandleResetToDefaults(const base::Value::List& args);
  void OnValueChanged(const std::string& pref_name);
  void OnRestartNeededChanged();
  bool IsRestartNeeded();

  // SettingsPageUIHandler implementation.
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  PrefChangeRegistrar pref_change_registrar_;
  PrefChangeRegistrar local_state_change_registrar_;

  const raw_ptr<Profile, DanglingUntriaged> profile_;
  bool was_rewards_enabled_ = false;
  bool was_news_enabled_ = false;
  bool was_p3a_enabled_ = false;
  bool was_stats_reporting_enabled_ = false;
  bool was_crash_reporting_enabled_ = false;
#if BUILDFLAG(ENABLE_BRAVE_VPN)
  bool was_vpn_enabled_ = false;
#endif
#if BUILDFLAG(ENABLE_TOR)
  bool was_tor_enabled_ = false;
#endif
  bool was_ai_enabled_ = false;
  bool was_wallet_enabled_ = false;
  base::WeakPtrFactory<BraveOriginHandler> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_
