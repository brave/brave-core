/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_appearance_handler.h"

#include "base/functional/bind.h"
#include "base/metrics/histogram_macros.h"
#include "base/strings/string_number_conversions.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/themes/brave_dark_mode_utils.h"
#include "brave/components/brave_new_tab/new_tab_prefs.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/instant_service.h"
#include "chrome/browser/search/instant_service_factory.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/web_ui.h"

BraveAppearanceHandler::BraveAppearanceHandler() {
  local_state_change_registrar_.Init(g_browser_process->local_state());
  local_state_change_registrar_.Add(
      kBraveDarkMode,
      base::BindRepeating(&BraveAppearanceHandler::OnBraveDarkModeChanged,
                          base::Unretained(this)));
}

BraveAppearanceHandler::~BraveAppearanceHandler() = default;

// TODO(simonhong): Use separate handler for NTP settings.
void BraveAppearanceHandler::RegisterMessages() {
  profile_ = Profile::FromWebUI(web_ui());
  profile_state_change_registrar_.Init(profile_->GetPrefs());
  profile_state_change_registrar_.Add(
      brave_new_tab::prefs::kNewTabShowsOption,
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
      "setBraveThemeType",
      base::BindRepeating(&BraveAppearanceHandler::SetBraveThemeType,
                          base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
      "getBraveThemeType",
      base::BindRepeating(&BraveAppearanceHandler::GetBraveThemeType,
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

void BraveAppearanceHandler::SetBraveThemeType(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  CHECK(args[0].is_int());
  AllowJavascript();

  int int_type = args[0].GetInt();
  dark_mode::SetBraveDarkModeType(
      static_cast<dark_mode::BraveDarkModeType>(int_type));
}

void BraveAppearanceHandler::GetBraveThemeType(const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);
  AllowJavascript();
  // GetBraveThemeType() should be used because settings option displays all
  // available options including default.
  ResolveJavascriptCallback(
      args[0],
      base::Value(static_cast<int>(dark_mode::GetBraveDarkModeType())));
}

void BraveAppearanceHandler::OnBraveDarkModeChanged() {
  // GetBraveThemeType() should be used because settings option displays all
  // available options including default.
  if (IsJavascriptAllowed()) {
    FireWebUIListener(
        "brave-theme-type-changed",
        base::Value(static_cast<int>(dark_mode::GetBraveDarkModeType())));
  }
}

void BraveAppearanceHandler::OnPreferenceChanged(const std::string& pref_name) {
  if (IsJavascriptAllowed()) {
    if (pref_name == brave_new_tab::prefs::kNewTabShowsOption ||
        pref_name == prefs::kHomePage ||
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
