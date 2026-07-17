// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_new_tab_page_refresh/update_observer.h"

#include <utility>
#include <vector>

#include "brave/browser/ntp_background/ntp_background_prefs.h"
#include "brave/browser/ui/webui/brave_new_tab_page_refresh/top_sites_facade.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "brave/components/brave_rewards/core/buildflags/buildflags.h"
#include "brave/components/brave_search_conversion/pref_names.h"
#include "brave/components/brave_talk/buildflags/buildflags.h"
#include "brave/components/brave_vpn/common/buildflags/buildflags.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "chrome/browser/new_tab_page/ntp_pref_names.h"
#include "chrome/common/pref_names.h"

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
#include "brave/components/brave_rewards/core/pref_names.h"
#endif

#if BUILDFLAG(ENABLE_BRAVE_TALK)
#include "brave/components/brave_talk/pref_names.h"
#endif

namespace brave_new_tab_page_refresh {

UpdateObserver::UpdateObserver(PrefService& pref_service,
                               TopSitesFacade* top_sites_facade) {
  pref_change_registrar_.Init(&pref_service);

  // Sponsored site tiles need both prefs enabled to show, so turning either
  // one off should also hide the tiles.
  AddPrefListener(ntp_background_images::prefs::kNewTabPageShowBackgroundImage,
                  {Source::kBackgrounds, Source::kTopSites});
  AddPrefListener(ntp_background_images::prefs::
                      kNewTabPageShowSponsoredImagesBackgroundImage,
                  {Source::kBackgrounds, Source::kTopSites});
  AddPrefListener(kNewTabPageShowSponsoredSites, Source::kTopSites);
  AddPrefListener(NTPBackgroundPrefs::kPrefName, Source::kBackgrounds);
  AddPrefListener(NTPBackgroundPrefs::kCustomImageListPrefName,
                  Source::kBackgrounds);

  AddPrefListener(brave_search_conversion::prefs::kShowNTPSearchBox,
                  Source::kSearch);
  AddPrefListener(prefs::kSearchSuggestEnabled, Source::kSearch);
  AddPrefListener(brave_search_conversion::prefs::kDismissed, Source::kSearch);

  AddPrefListener(ntp_prefs::kNtpShortcutsVisible, Source::kTopSites);
  AddPrefListener(ntp_prefs::kNtpCustomLinksVisible, Source::kTopSites);

  AddPrefListener(kNewTabPageShowClock, Source::kClock);
  AddPrefListener(kNewTabPageClockFormat, Source::kClock);

  AddPrefListener(kNewTabPageShowStats, Source::kShieldsStats);
  AddPrefListener(kAdsBlocked, Source::kShieldsStats);
  AddPrefListener(kTrackersBlocked, Source::kShieldsStats);
  AddPrefListener(brave_perf_predictor::prefs::kBandwidthSavedBytes,
                  Source::kShieldsStats);

#if BUILDFLAG(ENABLE_BRAVE_TALK)
  AddPrefListener(brave_talk::prefs::kNewTabPageShowBraveTalk, Source::kTalk);
#endif

#if BUILDFLAG(ENABLE_BRAVE_REWARDS)
  AddPrefListener(kNewTabPageShowRewards, Source::kRewards);
  AddPrefListener(brave_rewards::prefs::kExternalWalletType, Source::kTopSites);
#endif

#if BUILDFLAG(ENABLE_BRAVE_VPN)
  AddPrefListener(kNewTabPageShowBraveVPN, Source::kVPN);
#endif

  if (top_sites_facade) {
    top_sites_facade->SetSitesUpdatedCallback(
        base::BindRepeating(&UpdateObserver::OnUpdate,
                            weak_factory_.GetWeakPtr(), Source::kTopSites));
  }
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

void UpdateObserver::AddPrefListener(
    const std::string& path,
    std::initializer_list<Source> update_sources) {
  pref_change_registrar_.Add(
      path,
      base::BindRepeating(
          [](base::WeakPtr<UpdateObserver> self,
             std::vector<Source> update_sources) {
            if (self) {
              for (Source update_source : update_sources) {
                self->OnUpdate(update_source);
              }
            }
          },
          weak_factory_.GetWeakPtr(), std::vector<Source>(update_sources)));
}

}  // namespace brave_new_tab_page_refresh
