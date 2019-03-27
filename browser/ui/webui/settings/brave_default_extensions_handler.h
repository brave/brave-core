/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_

#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class Profile;

class BraveDefaultExtensionsHandler : public settings::SettingsPageUIHandler {
 public:
  BraveDefaultExtensionsHandler() = default;
  ~BraveDefaultExtensionsHandler() override = default;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void SetWebTorrentEnabled(const base::ListValue* args);
  void SetHangoutsEnabled(const base::ListValue* args);
  void SetIPFSCompanionEnabled(const base::ListValue* args);

  Profile* profile_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BraveDefaultExtensionsHandler);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_DEFAULT_EXTENSIONS_HANDLER_H_
