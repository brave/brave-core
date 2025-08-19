/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_

#include <string>
#include <unordered_map>

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
  BraveOriginHandler();
  BraveOriginHandler(const BraveOriginHandler&) = delete;
  BraveOriginHandler& operator=(const BraveOriginHandler&) = delete;
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
  void StoreInitialValues();

  // SettingsPageUIHandler implementation.
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  PrefChangeRegistrar pref_change_registrar_;
  PrefChangeRegistrar local_state_change_registrar_;

  raw_ptr<Profile> profile_ = nullptr;

  // Map to store initial preference values for restart detection
  std::unordered_map<std::string_view, bool> initial_values_;

  base::WeakPtrFactory<BraveOriginHandler> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_
