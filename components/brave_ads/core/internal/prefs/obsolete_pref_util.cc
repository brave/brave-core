/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/prefs/obsolete_pref_util.h"

#include "brave/components/brave_ads/core/public/prefs/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_ads {

namespace {

constexpr char kObsoleteShouldShowSearchResultAdClickedInfoBar[] =
    "brave.brave_ads.should_show_search_result_ad_clicked_infobar";

constexpr const char* kObsoleteP2APrefPaths[] = {
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.architecture)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.artsentertainment)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.automotive)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.business)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.careers)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.cellphones)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.crypto)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.education)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.familyparenting)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.fashion)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.folklore)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.fooddrink)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.gaming)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.healthfitness)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.history)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.hobbiesinterests)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.home)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.law)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.military)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.other)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.personalfinance)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.pets)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.realestate)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.science)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.sports)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.technologycomputing)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.travel)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.untargeted)",
    R"(brave.weekly_storage.Brave.P2A.ad_notification.opportunities_per_segment.weather)",
    R"(brave.weekly_storage.Brave.P2A.inline_content_ad.opportunities)",
    R"(brave.weekly_storage.Brave.P2A.new_tab_page_ad.opportunities)"};

constexpr std::string_view kNewTabPageEventCountDictPref =
    "brave.brave_ads.p3a.ntp_event_count";
constexpr std::string_view kNewTabPageEventCountConstellationDictPref =
    "brave.brave_ads.p3a.ntp_event_count_constellation";
constexpr std::string_view kNewTabPageKnownCampaignsDictPref =
    "brave.brave_ads.p3a.ntp_known_campaigns";

void MaybeMigrateShouldShowSearchResultAdClickedInfoBarProfilePref(
    PrefService* const prefs) {
  if (!prefs->HasPrefPath(kObsoleteShouldShowSearchResultAdClickedInfoBar)) {
    return;
  }

  prefs->SetBoolean(
      prefs::kShouldShowSearchResultAdClickedInfoBar,
      prefs->GetBoolean(kObsoleteShouldShowSearchResultAdClickedInfoBar));
  prefs->ClearPref(kObsoleteShouldShowSearchResultAdClickedInfoBar);
}

}  // namespace

void RegisterProfilePrefsForMigration(PrefRegistrySimple* const registry) {
  // Added 05/2025.
  registry->RegisterBooleanPref(kObsoleteShouldShowSearchResultAdClickedInfoBar,
                                false);

  // Added 06/2025.
  for (const auto* path : kObsoleteP2APrefPaths) {
    registry->RegisterListPref(path);
  }
}

void MigrateObsoleteProfilePrefs(PrefService* const prefs) {
  // Added 05/2025.
  MaybeMigrateShouldShowSearchResultAdClickedInfoBarProfilePref(prefs);

  // Added 06/2025.
  for (const auto* path : kObsoleteP2APrefPaths) {
    prefs->ClearPref(path);
  }
}

void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
  // Added 06/2025
  registry->RegisterDictionaryPref(kNewTabPageEventCountDictPref);

  // Added 10/2025
  registry->RegisterDictionaryPref(kNewTabPageEventCountConstellationDictPref);
  registry->RegisterDictionaryPref(kNewTabPageKnownCampaignsDictPref);
}

void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // Added 06/2025
  local_state->ClearPref(kNewTabPageEventCountDictPref);

  // Added 10/2025
  local_state->ClearPref(kNewTabPageEventCountConstellationDictPref);
  local_state->ClearPref(kNewTabPageKnownCampaignsDictPref);
}

}  // namespace brave_ads
