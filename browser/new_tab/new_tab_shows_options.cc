/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/new_tab_shows_options.h"

#include <utility>

#include "base/values.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/common/pref_names.h"
#include "brave/common/webui_url_constants.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/webui/ntp/new_tab_ui.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/webui_url_constants.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "url/url_constants.h"

namespace brave {

GURL GetNewTabPageURL(Profile* profile) {
  DCHECK(profile);

  if (!brave::IsRegularProfile(profile))
    return GURL();

  auto* prefs = profile->GetPrefs();

  NewTabPageShowsOptions option = static_cast<NewTabPageShowsOptions>(
      prefs->GetInteger(kNewTabPageShowsOptions));
  if (option == NewTabPageShowsOptions::kHomepage) {
    if (prefs->GetBoolean(prefs::kHomePageIsNewTabPage))
      return GURL();
    return GURL(prefs->GetString(prefs::kHomePage));
  } else if (option == NewTabPageShowsOptions::kBlankpage) {
    // NewTab route will handle for blank page.
    return GURL();
  } else {
    DCHECK_EQ(NewTabPageShowsOptions::kDashboard, option);
    return GURL();
  }

  return GURL();
}

base::Value GetNewTabShowsOptionsList(Profile* profile) {
  base::Value list(base::Value::Type::LIST);

  base::Value dashboard_option(base::Value::Type::DICTIONARY);
  dashboard_option.SetIntKey(
      "value",
      static_cast<int>(NewTabPageShowsOptions::kDashboard));
  dashboard_option.SetStringKey(
      "name",
      l10n_util::GetStringUTF8(
          IDS_SETTINGS_NEW_TAB_NEW_TAB_PAGE_SHOWS_DASHBOARD));
  list.Append(std::move(dashboard_option));

  base::Value homepage_option(base::Value::Type::DICTIONARY);
  homepage_option.SetIntKey(
      "value",
      static_cast<int>(NewTabPageShowsOptions::kHomepage));
  homepage_option.SetStringKey(
      "name",
      l10n_util::GetStringUTF8(
          IDS_SETTINGS_NEW_TAB_NEW_TAB_PAGE_SHOWS_HOMEPAGE));
  list.Append(std::move(homepage_option));

  base::Value blankpage_option(base::Value::Type::DICTIONARY);
  blankpage_option.SetIntKey(
      "value",
      static_cast<int>(NewTabPageShowsOptions::kBlankpage));
  blankpage_option.SetStringKey(
      "name",
      l10n_util::GetStringUTF8(
          IDS_SETTINGS_NEW_TAB_NEW_TAB_PAGE_SHOWS_BLANKPAGE));
  list.Append(std::move(blankpage_option));
  return list;
}

bool ShouldUseNewTabURLForNewTab(Profile* profile) {
  const GURL url = GetNewTabPageURL(profile);
  return url.is_empty() ||
         url.host() == chrome::kChromeUINewTabHost ||
         NewTabUI::IsNewTab(url);
}

bool ShouldNewTabShowDashboard(Profile* profile) {
  auto* prefs = profile->GetPrefs();
  if (static_cast<NewTabPageShowsOptions>(
          prefs->GetInteger(kNewTabPageShowsOptions)) ==
      NewTabPageShowsOptions::kBlankpage)
    return false;

  return ShouldUseNewTabURLForNewTab(profile);
}

bool ShouldNewTabShowBlankpage(Profile* profile) {
  if (!brave::IsRegularProfile(profile))
    return false;

  return profile->GetPrefs()->GetInteger(kNewTabPageShowsOptions) ==
      static_cast<int>(brave::NewTabPageShowsOptions::kBlankpage);
}

}  // namespace brave
