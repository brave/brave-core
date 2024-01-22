/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_VPN_BRAVE_VPN_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_VPN_BRAVE_VPN_HANDLER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_vpn/browser/brave_vpn_service_observer.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/prefs/pref_change_registrar.h"

class BraveVpnHandler : public settings::SettingsPageUIHandler,
                        public brave_vpn::BraveVPNServiceObserver {
 public:
  explicit BraveVpnHandler(Profile* profile);
  ~BraveVpnHandler() override;

 private:
  // WebUIMessageHandler implementation.
  void RegisterMessages() override;

  void HandleIsWireguardServiceInstalled(const base::Value::List& args);
  void OnWireguardServiceInstalled(const std::string& callback_id,
                                   bool success);
  void HandleIsBraveVpnConnected(const base::Value::List& args);

  // brave_vpn::BraveVPNServiceObserver
  void OnConnectionStateChanged(
      brave_vpn::mojom::ConnectionState state) override;
  void OnProtocolChanged();

  // SettingsPageUIHandler implementation.
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  PrefChangeRegistrar pref_change_registrar_;
  const raw_ptr<Profile, DanglingUntriaged> profile_;
  base::WeakPtrFactory<BraveVpnHandler> weak_factory_{this};
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_VPN_BRAVE_VPN_HANDLER_H_
