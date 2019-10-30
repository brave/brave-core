/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/browser/tor/buildflags.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "chrome/common/extensions/webstore_install_result.h"
#include "components/prefs/pref_change_registrar.h"

class Profile;

class BraveDefaultExtensionsHandler : public settings::SettingsPageUIHandler {
 public:
  BraveDefaultExtensionsHandler();
  ~BraveDefaultExtensionsHandler() override;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void GetRestartNeeded(const base::ListValue* args);
  void SetWebTorrentEnabled(const base::ListValue* args);
  void SetBraveWaybackMachineEnabled(const base::ListValue* args);
  void SetHangoutsEnabled(const base::ListValue* args);
  void SetIPFSCompanionEnabled(const base::ListValue* args);
  void SetMediaRouterEnabled(const base::ListValue* args);
  void SetBraveWalletEnabled(const base::ListValue* args);
#if BUILDFLAG(ENABLE_TOR)
  void SetTorEnabled(const base::ListValue* args);
  void IsTorEnabled(const base::ListValue* args);
  void OnTorEnabledChanged();
  void IsTorManaged(const base::ListValue* args);
#endif

  void InitializePrefCallbacks();

  bool IsExtensionInstalled(const std::string& extension_id) const;
  void OnInstallResult(const std::string& pref_name,
      bool success, const std::string& error,
      extensions::webstore_install::Result result);

  void OnRestartNeededChanged();
  void OnMediaRouterEnabledChanged();
  bool IsRestartNeeded();

  Profile* profile_ = nullptr;
  PrefChangeRegistrar pref_change_registrar_;
#if BUILDFLAG(ENABLE_TOR)
  PrefChangeRegistrar local_state_change_registrar_;
#endif
  base::WeakPtrFactory<BraveDefaultExtensionsHandler> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BraveDefaultExtensionsHandler);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_
