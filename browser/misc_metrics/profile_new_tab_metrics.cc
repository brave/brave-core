/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/misc_metrics/profile_new_tab_metrics.h"

#include <optional>

#include "base/containers/contains.h"
#include "base/metrics/histogram_macros.h"
#include "brave/browser/new_tab/new_tab_shows_options.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "url/gurl.h"

namespace misc_metrics {

ProfileNewTabMetrics::ProfileNewTabMetrics(PrefService* profile_prefs)
    : profile_prefs_(profile_prefs) {
  pref_change_registrar_.Init(profile_prefs_);

  auto callback = base::BindRepeating(
      &ProfileNewTabMetrics::ReportNewTabPageDefault, base::Unretained(this));
  pref_change_registrar_.Add(kNewTabPageShowsOptions, callback);
  pref_change_registrar_.Add(prefs::kHomePage, callback);
  pref_change_registrar_.Add(prefs::kHomePageIsNewTabPage, callback);

  // Report the initial state
  ReportNewTabPageDefault();
}

ProfileNewTabMetrics::~ProfileNewTabMetrics() = default;

void ProfileNewTabMetrics::ReportNewTabPageDefault() {
  std::optional<NewTabPageDefaultType> type;
  brave::NewTabPageShowsOptions option =
      static_cast<brave::NewTabPageShowsOptions>(
          profile_prefs_->GetInteger(kNewTabPageShowsOptions));

  if (option == brave::NewTabPageShowsOptions::kDashboard) {
    type = NewTabPageDefaultType::kDashboard;
  } else if (option == brave::NewTabPageShowsOptions::kHomepage) {
    if (profile_prefs_->GetBoolean(prefs::kHomePageIsNewTabPage)) {
      type = NewTabPageDefaultType::kDashboard;
    } else {
      GURL homepage_url(profile_prefs_->GetString(prefs::kHomePage));

      std::string_view host = homepage_url.host();
      if (host == "search.brave.com") {
        type = NewTabPageDefaultType::kHomepageBraveSearch;
      } else if (base::Contains(host, "google")) {
        type = NewTabPageDefaultType::kHomepageGoogle;
      } else if (base::Contains(host, "duckduckgo")) {
        type = NewTabPageDefaultType::kHomepageDuckDuckGo;
      } else {
        type = NewTabPageDefaultType::kHomepageOther;
      }
    }
  } else if (option == brave::NewTabPageShowsOptions::kBlankpage) {
    type = NewTabPageDefaultType::kBlank;
  }

  if (type) {
    UMA_HISTOGRAM_ENUMERATION(kNewTabPageDefaultHistogramName, *type);
  }
}

}  // namespace misc_metrics
