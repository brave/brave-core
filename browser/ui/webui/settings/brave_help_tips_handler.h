/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_HELP_TIPS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_HELP_TIPS_HANDLER_H_

#include "brave/components/brave_wayback_machine/buildflags/buildflags.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"

class Profile;

class BraveHelpTipsHandler : public settings::SettingsPageUIHandler {
 public:
  BraveHelpTipsHandler() = default;
  ~BraveHelpTipsHandler() override = default;

 private:
  // settings::SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override {}
  void OnJavascriptDisallowed() override {}

#if BUILDFLAG(ENABLE_BRAVE_WAYBACK_MACHINE)
  void SetBraveWaybackMachineEnabled(const base::ListValue* args);
#endif

  Profile* profile_ = nullptr;

  DISALLOW_COPY_AND_ASSIGN(BraveHelpTipsHandler);
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_HELP_TIPS_HANDLER_H_
