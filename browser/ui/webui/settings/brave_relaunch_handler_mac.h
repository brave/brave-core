/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_RELAUNCH_HANDLER_MAC_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_RELAUNCH_HANDLER_MAC_H_

#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class Profile;

class BraveRelaunchHandler : public settings::SettingsPageUIHandler {
 public:
  BraveRelaunchHandler() = default;
  ~BraveRelaunchHandler() override = default;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

  void Relaunch(base::Value::ConstListView args);

  DISALLOW_COPY_AND_ASSIGN(BraveRelaunchHandler);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_RELAUNCH_HANDLER_MAC_H_
