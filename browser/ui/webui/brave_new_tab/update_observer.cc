// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab/update_observer.h"

#include <utility>

#include "brave/browser/ntp_background/ntp_background_prefs.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/browser/ui/webui/new_tab_page/ntp_pref_names.h"
#include "chrome/common/pref_names.h"

namespace brave_new_tab {

UpdateObserver::UpdateObserver(PrefService& pref_service) {
  pref_change_registrar_.Init(&pref_service);
  AddPrefListener(ntp_background_images::prefs::kNewTabPageShowBackgroundImage,
                  Source::kBackgroundPrefs);
  AddPrefListener(ntp_background_images::prefs::
                      kNewTabPageShowSponsoredImagesBackgroundImage,
                  Source::kBackgroundPrefs);
  AddPrefListener(NTPBackgroundPrefs::kPrefName, Source::kBackgroundPrefs);
  AddPrefListener(NTPBackgroundPrefs::kCustomImageListPrefName,
                  Source::kBackgroundPrefs);
  AddPrefListener(brave_search_conversion::prefs::kShowNTPSearchBox,
                  Source::kSearchPrefs);
  AddPrefListener(prefs::kSearchSuggestEnabled, Source::kSearchPrefs);
  AddPrefListener(brave_search_conversion::prefs::kDismissed,
                  Source::kSearchPrefs);
  AddPrefListener(ntp_prefs::kNtpShortcutsVisible, Source::kTopSitesPrefs);
  AddPrefListener(ntp_prefs::kNtpUseMostVisitedTiles, Source::kTopSitesPrefs);
  AddPrefListener(kNewTabPageShowClock, Source::kClockPrefs);
  AddPrefListener(kNewTabPageClockFormat, Source::kClockPrefs);
}

UpdateObserver::~UpdateObserver() = default;

void UpdateObserver::SetCallback(
    base::RepeatingCallback<void(Source)> callback) {
  callback_ = std::move(callback);
}

void UpdateObserver::OnUpdate(Source update_source) {
  if (callback_) {
    callback_.Run(update_source);
  }
}

void UpdateObserver::OnPrefChanged(Source update_kind,
                                   const std::string& path) {
  OnUpdate(update_kind);
}

void UpdateObserver::AddPrefListener(const std::string& path,
                                     Source update_source) {
  pref_change_registrar_.Add(
      path, base::BindRepeating(&UpdateObserver::OnPrefChanged,
                                weak_factory_.GetWeakPtr(), update_source));
}

}  // namespace brave_new_tab
