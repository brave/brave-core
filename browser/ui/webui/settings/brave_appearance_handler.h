/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_

#include <string>

#include "base/callback_list.h"
#include "base/memory/raw_ptr.h"
#include "chrome/browser/command_observer.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/tabs/public/tab_interface.h"

class CommandUpdater;
class Profile;

class BraveAppearanceHandler : public settings::SettingsPageUIHandler,
                               public CommandObserver {
 public:
  BraveAppearanceHandler();
  ~BraveAppearanceHandler() override;

  BraveAppearanceHandler(const BraveAppearanceHandler&) = delete;
  BraveAppearanceHandler& operator=(const BraveAppearanceHandler&) = delete;

 private:
  // SettingsPageUIHandler overrides:
  void RegisterMessages() override;
  void OnJavascriptAllowed() override;
  void OnJavascriptDisallowed() override;

  // CommandObserver override:
  void EnabledStateChangedForCommand(int id, bool enabled) override;

  void OnPreferenceChanged(const std::string& pref_name);
  void GetNewTabShowsOptionsList(const base::ListValue& args);
  void ShouldShowNewTabDashboardSettings(const base::ListValue& args);
  void GetIsVerticalTabsToggleEnabled(const base::ListValue& args);

  // Clears `command_updater_` (removing the command observer first, while the
  // controller is still alive) before the owning browser window is torn down.
  void OnTabWillDetach(tabs::TabInterface* tab,
                       tabs::TabInterface::DetachReason reason);

  raw_ptr<Profile> profile_ = nullptr;
  raw_ptr<CommandUpdater> command_updater_ = nullptr;
  PrefChangeRegistrar profile_state_change_registrar_;
  base::CallbackListSubscription tab_will_detach_subscription_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_APPEARANCE_HANDLER_H_
