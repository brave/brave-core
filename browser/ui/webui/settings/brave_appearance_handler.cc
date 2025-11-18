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
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/instant_service.h"
#include "chrome/browser/search/instant_service_factory.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
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
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], base::Value(brave::GetNewTabShowsOptionsList(profile_)));
}

void BraveAppearanceHandler::ShouldShowNewTabDashboardSettings(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  ResolveJavascriptCallback(
      args[0], base::Value(brave::ShouldNewTabShowDashboard(profile_)));
}
