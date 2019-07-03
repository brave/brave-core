// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_message_handler.h"

#include <string>

#include "base/bind.h"
#include "base/values.h"
#include "brave/browser/ui/webui/brave_new_tab_ui.h"
#include "brave/browser/search_engines/search_engine_provider_util.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

BraveNewTabMessageHandler::BraveNewTabMessageHandler(BraveNewTabUI* web_ui)
          : new_tab_web_ui_(web_ui) {
}

BraveNewTabMessageHandler::~BraveNewTabMessageHandler() {}

void BraveNewTabMessageHandler::OnJavascriptAllowed() {
  // Observe relevant preferences
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  pref_change_registrar_.Init(prefs);
  // Stats
  pref_change_registrar_.Add(kAdsBlocked,
    base::Bind(&BraveNewTabMessageHandler::OnStatsChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kTrackersBlocked,
    base::Bind(&BraveNewTabMessageHandler::OnStatsChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kHttpsUpgrades,
    base::Bind(&BraveNewTabMessageHandler::OnStatsChanged,
    base::Unretained(this)));
  // Private New Tab Page preferences
  pref_change_registrar_.Add(kUseAlternativeSearchEngineProvider,
    base::Bind(&BraveNewTabMessageHandler::OnPrivatePropertiesChanged,
    base::Unretained(this)));
  pref_change_registrar_.Add(kAlternativeSearchEngineProviderInTor,
    base::Bind(&BraveNewTabMessageHandler::OnPrivatePropertiesChanged,
    base::Unretained(this)));
  // New Tab Page preferences
  pref_change_registrar_.Add(kNewTabPageShowBackgroundImage,
    base::Bind(&BraveNewTabMessageHandler::OnPreferencesChanged,
    base::Unretained(this)));
}

void BraveNewTabMessageHandler::OnJavascriptDisallowed() {
  pref_change_registrar_.RemoveAll();
}

void BraveNewTabMessageHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
    "newTabPageInitialized",
    base::BindRepeating(
      &BraveNewTabMessageHandler::HandleInitialized,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "toggleAlternativePrivateSearchEngine",
    base::BindRepeating(
      &BraveNewTabMessageHandler::HandleToggleAlternativeSearchEngineProvider,
      base::Unretained(this)));
  web_ui()->RegisterMessageCallback(
    "saveNewTabPagePref",
    base::BindRepeating(
      &BraveNewTabMessageHandler::HandleSaveNewTabPagePref,
      base::Unretained(this)));
}

void BraveNewTabMessageHandler::HandleInitialized(const base::ListValue* args) {
  AllowJavascript();
}

void BraveNewTabMessageHandler::HandleToggleAlternativeSearchEngineProvider(
    const base::ListValue* args) {
  brave::ToggleUseAlternativeSearchEngineProvider(
      Profile::FromWebUI(web_ui()));
}

void BraveNewTabMessageHandler::HandleSaveNewTabPagePref(
    const base::ListValue* args) {
  if (args->GetSize() != 2) {
    LOG(ERROR) << "Invalid input";
    return;
  }
  PrefService* prefs = Profile::FromWebUI(web_ui())->GetPrefs();
  // Collect args
  std::string settingsKeyInput = args->GetList()[0].GetString();
  auto settingsValue = args->GetList()[1].Clone();
  // Validate args
  // Note: if we introduce any non-bool settings values
  // then perform this type check within the appropriate key conditionals.
  if (!settingsValue.is_bool()) {
    LOG(ERROR) << "Invalid value type";
    return;
  }
  const auto settingsValueBool = settingsValue.GetBool();
  std::string settingsKey;
  if (settingsKeyInput == "showBackgroundImage") {
    settingsKey = kNewTabPageShowBackgroundImage;
  } else {
    LOG(ERROR) << "Invalid setting key";
    return;
  }
  prefs->SetBoolean(settingsKey, settingsValueBool);
}

void BraveNewTabMessageHandler::OnPrivatePropertiesChanged() {
  new_tab_web_ui_->OnPrivatePropertiesChanged();
}

void BraveNewTabMessageHandler::OnStatsChanged() {
  new_tab_web_ui_->OnStatsChanged();
  FireWebUIListener("stats-updated");
}

void BraveNewTabMessageHandler::OnPreferencesChanged() {
  new_tab_web_ui_->OnPreferencesChanged();
  FireWebUIListener("preferences-changed");
}
