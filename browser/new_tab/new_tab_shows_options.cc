/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/new_tab/new_tab_shows_options.h"

#include "brave/browser/profiles/profile_util.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/url_constants.h"

namespace brave {

GURL GetNewTabPageURL(Profile* profile) {
  DCHECK(profile);

  if (!brave::IsRegularProfile(profile))
    return GURL();

  auto* prefs = profile->GetPrefs();

  int option = prefs->GetInteger(kNewTabPageShowsOptions);
  if (option == brave::NewTabPageShowsOptions::HOMEPAGE) {
    if (!prefs->GetBoolean(prefs::kHomePageIsNewTabPage))
      return GURL(prefs->GetString(prefs::kHomePage));
    return GURL();
  } else if (option == brave::NewTabPageShowsOptions::BLANKPAGE) {
    return GURL(url::kAboutBlankURL);
  } else {
    DCHECK_EQ(brave::NewTabPageShowsOptions::DASHBOARD_WITH_IMAGES, option);
    return GURL();
  }

  return GURL();
}

}  // namespace brave
