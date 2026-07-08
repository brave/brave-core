/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_appearance_handler.h"

#include "base/check.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/instant_service.h"
#include "chrome/browser/search/instant_service_factory.h"
#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_window/public/browser_window_features.h"
#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/tabs/public/tab_interface.h"
#include "content/public/browser/web_ui.h"

BraveAppearanceHandler::BraveAppearanceHandler() = default;
BraveAppearanceHandler::~BraveAppearanceHandler() = default;

// TODO(simonhong): Use separate handler for NTP settings.
void BraveAppearanceHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
  profile_state_change_registrar_.Init(profile_->GetPrefs());
  profile_state_change_registrar_.Add(
      kNewTabPageShowsOptions,
      base::BindRepeating(&BraveAppearanceHandler::OnPreferenceChanged,
                          base::Unretained(this)));
  profile_state_change_registrar_.Add(
      prefs::kHomePageIsNewTabPage,
      base::BindRepeating(&BraveAppearanceHandler::OnPreferenceChanged,
                          base::Unretained(this)));
  profile_state_change_registrar_.Add(
      prefs::kHomePage,
      base::BindRepeating(&BraveAppearanceHandler::OnPreferenceChanged,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getNewTabShowsOptionsList",
      base::BindRepeating(&BraveAppearanceHandler::GetNewTabShowsOptionsList,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "shouldShowNewTabDashboardSettings",
      base::BindRepeating(
          &BraveAppearanceHandler::ShouldShowNewTabDashboardSettings,
          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getIsVerticalTabsToggleEnabled",
      base::BindRepeating(
          &BraveAppearanceHandler::GetIsVerticalTabsToggleEnabled,
          base::Unretained(this)));

  if (auto* tab = tabs::TabInterface::MaybeGetFromContents(
          web_ui()->GetWebContents())) {
    command_updater_ = tab->GetBrowserWindowInterface()
                           ->GetFeatures()
                           .browser_command_controller();
    // `command_updater_` is owned by the browser window's features and is
    // destroyed during window teardown (e.g. on relaunch), before this WebUI
    // handler's OnJavascriptDisallowed() runs. Clear the cached pointer while
    // the controller is still alive so it is never dereferenced after free.
    tab_will_detach_subscription_ = tab->RegisterWillDetach(base::BindRepeating(
        &BraveAppearanceHandler::OnTabWillDetach, base::Unretained(this)));
  }
}

void BraveAppearanceHandler::OnJavascriptAllowed() {
  if (command_updater_) {
    command_updater_->AddCommandObserver(IDC_TOGGLE_VERTICAL_TABS, this);
  }
}

void BraveAppearanceHandler::OnJavascriptDisallowed() {
  if (command_updater_) {
    command_updater_->RemoveCommandObserver(IDC_TOGGLE_VERTICAL_TABS, this);
  }
}

void BraveAppearanceHandler::EnabledStateChangedForCommand(int id,
                                                           bool enabled) {
  DCHECK_EQ(id, IDC_TOGGLE_VERTICAL_TABS);
  if (IsJavascriptAllowed()) {
    FireWebUIListener("vertical-tabs-toggle-enabled-changed",
                      base::Value(enabled));
  }
}

void BraveAppearanceHandler::OnPreferenceChanged(const std::string& pref_name) {
  if (IsJavascriptAllowed()) {
    if (pref_name == kNewTabPageShowsOptions || pref_name == prefs::kHomePage ||
        pref_name == prefs::kHomePageIsNewTabPage) {
      FireWebUIListener(
          "show-new-tab-dashboard-settings-changed",
          base::Value(brave::ShouldNewTabShowDashboard(profile_)));
      return;
    }
  }
}

void BraveAppearanceHandler::GetNewTabShowsOptionsList(
    const base::ListValue& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], base::Value(brave::GetNewTabShowsOptionsList(profile_)));
}

void BraveAppearanceHandler::ShouldShowNewTabDashboardSettings(
    const base::ListValue& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], base::Value(brave::ShouldNewTabShowDashboard(profile_)));
}

void BraveAppearanceHandler::GetIsVerticalTabsToggleEnabled(
    const base::ListValue& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  const bool enabled = !command_updater_ || command_updater_->IsCommandEnabled(
                                                IDC_TOGGLE_VERTICAL_TABS);
  ResolveJavascriptCallback(args[0], base::Value(enabled));
}

void BraveAppearanceHandler::OnTabWillDetach(
    tabs::TabInterface* tab,
    tabs::TabInterface::DetachReason reason) {
  if (!command_updater_) {
    return;
  }
  // The command observer is registered only while JavaScript is allowed (see
  // OnJavascriptAllowed/OnJavascriptDisallowed). Remove it now, while the
  // controller is still alive, to keep AddCommandObserver/RemoveCommandObserver
  // balanced and avoid a use-after-free when OnJavascriptDisallowed() runs
  // later during WebContents teardown.
  if (IsJavascriptAllowed()) {
    command_updater_->RemoveCommandObserver(IDC_TOGGLE_VERTICAL_TABS, this);
  }
  command_updater_ = nullptr;
}
