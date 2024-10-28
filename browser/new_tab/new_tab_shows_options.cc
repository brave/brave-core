/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/new_tab_shows_options.h"

#include <utility>

#include "brave/components/brave_new_tab/new_tab_prefs.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/constants/webui_url_constants.h"
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

  if (!profile->IsRegularProfile()) {
    return GURL();
  }

  auto* prefs = profile->GetPrefs();

  switch (brave_new_tab::prefs::GetNewTabShowsOption(prefs)) {
    case brave_new_tab::prefs::NewTabShowsOption::kHomepage:
      return prefs->GetBoolean(prefs::kHomePageIsNewTabPage)
                 ? GURL()
                 : GURL(prefs->GetString(prefs::kHomePage));
    case brave_new_tab::prefs::NewTabShowsOption::kBlankpage:
    case brave_new_tab::prefs::NewTabShowsOption::kDashboard:
      return GURL();
  }
}

base::Value::List GetNewTabShowsOptionsList(Profile* profile) {
  using brave_new_tab::prefs::NewTabShowsOption;

  base::Value::List list;

  base::Value::Dict dashboard_option;
  dashboard_option.Set("value",
                       static_cast<int>(NewTabShowsOption::kDashboard));
  dashboard_option.Set("name",
                       l10n_util::GetStringUTF8(
                           IDS_SETTINGS_NEW_TAB_NEW_TAB_PAGE_SHOWS_DASHBOARD));
  list.Append(std::move(dashboard_option));

  base::Value::Dict homepage_option;
  homepage_option.Set("value", static_cast<int>(NewTabShowsOption::kHomepage));
  homepage_option.Set("name",
                      l10n_util::GetStringUTF8(
                          IDS_SETTINGS_NEW_TAB_NEW_TAB_PAGE_SHOWS_HOMEPAGE));
  list.Append(std::move(homepage_option));

  base::Value::Dict blankpage_option;
  blankpage_option.Set("value",
                       static_cast<int>(NewTabShowsOption::kBlankpage));
  blankpage_option.Set("name",
                       l10n_util::GetStringUTF8(
                           IDS_SETTINGS_NEW_TAB_NEW_TAB_PAGE_SHOWS_BLANKPAGE));
  list.Append(std::move(blankpage_option));
  return list;
}

bool ShouldUseNewTabURLForNewTab(Profile* profile) {
  const GURL url = GetNewTabPageURL(profile);
  return url.is_empty() || url.host() == chrome::kChromeUINewTabHost ||
         NewTabUI::IsNewTab(url);
}

bool ShouldNewTabShowDashboard(Profile* profile) {
  auto option = brave_new_tab::prefs::GetNewTabShowsOption(profile->GetPrefs());
  if (option == brave_new_tab::prefs::NewTabShowsOption::kBlankpage) {
    return false;
  }

  return ShouldUseNewTabURLForNewTab(profile);
}

bool ShouldNewTabShowBlankpage(Profile* profile) {
  if (!profile->IsRegularProfile()) {
    return false;
  }

  auto option = brave_new_tab::prefs::GetNewTabShowsOption(profile->GetPrefs());
  return option == brave_new_tab::prefs::NewTabShowsOption::kBlankpage;
}

}  // namespace brave
