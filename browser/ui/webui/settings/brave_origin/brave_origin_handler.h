/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
//#include "brave/components/brave_vpn/browser/brave_vpn_service_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/prefs/pref_change_registrar.h"

// TODO(bsclifton): ideal situation would be to have a Brave Origin keyed
// service and this could be an observer of that. When the service initializes,
// etc it would trigger an OnSubscriptionStatusChanged() that this can
// implement.
class BraveOriginHandler : public settings::SettingsPageUIHandler {
 public:
  explicit BraveOriginHandler(Profile* profile);
  ~BraveOriginHandler() override;

 private:
  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

  void HandleGetInitialState(const base::Value::List& args);

  // SettingsPageUIHandler implementation.
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  PrefChangeRegistrar pref_change_registrar_;
  const raw_ptr<Profile, DanglingUntriaged> profile_;
  base::WeakPtrFactory<BraveOriginHandler> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ORIGIN_BRAVE_ORIGIN_HANDLER_H_
