/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/search/ntp_utils.h"

#include "base/logging.h"
#include "brave/components/constants/pref_names.h"
#include "chrome/browser/ui/webui/new_tab_page/ntp_pref_names.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"

namespace {

void ClearNewTabPageProfilePrefs(PrefService* prefs) {
  prefs->ClearPref(kNewTabPageShowTopSites);
}

const char* const kWidgetPrefNames[] = {kNewTabPageShowRewards,
                                        kNewTabPageShowBraveTalk};

}  // namespace

namespace new_tab_page {

void MigrateNewTabPagePrefs(PrefService* prefs) {
  // Migrate over to the Chromium setting for shortcuts visible
  // Only sets the value if user has changed it
  const PrefService::Preference* top_sites_pref =
      prefs->FindPreference(kNewTabPageShowTopSites);
  if (top_sites_pref->HasUserSetting()) {
    prefs->SetBoolean(ntp_prefs::kNtpShortcutsVisible,
                      prefs->GetBoolean(kNewTabPageShowTopSites));
  }

  // The toggle to turn off all widgets used to simply turn off
  // all existing widgets. We later introduced a pref so that future
  // new widgets do not show for that user. Perform a one-off migration
  // for known widgets at the time to set this new pref.
  const PrefService::Preference* hide_widgets_pref =
      prefs->FindPreference(kNewTabPageHideAllWidgets);
  if (!hide_widgets_pref->HasUserSetting()) {
    VLOG(1) << "Migrating hide widget pref...";
    // If all the widgets are off, assume user wants no future widgets.
    bool all_were_off = true;
    for (auto* const pref_name : kWidgetPrefNames) {
      auto* pref = prefs->FindPreference(pref_name);
      // Do not assume default is for pref to be on, so make
      // sure user has overriden pref value and it is false,
      // since that's what the previous version of the
      // "turn off all widgets" toggle did.
      bool user_turned_off = pref->HasUserSetting() &&
          (pref->GetValue()->GetIfBool().value_or(true) == false);
      VLOG(1) << "Setting: " << pref_name <<
          ", was off? " << user_turned_off;
      if (user_turned_off == false) {
        all_were_off = false;
        break;
      }
    }
    if (all_were_off) {
      // Turn the widgets back on so that when user "un-hides all", they
      // will show. This replicates the previous functionality, for the first
      // time the user re-enables. After that, the new functionality will
      // take effect, whereby each widget's show/hide setting is remembered
      // individually.
      prefs->SetBoolean(kNewTabPageShowRewards, true);
      prefs->SetBoolean(kNewTabPageShowBraveTalk, true);
    }
    // Record whether this has been migrated by setting an explicit
    // value for the HideAllWidgets pref.
    prefs->SetBoolean(kNewTabPageHideAllWidgets, all_were_off);
    VLOG(1) << "Done migrating hide widget pref: " << all_were_off;
  }

  // Clear deprecated prefs.
  ClearNewTabPageProfilePrefs(prefs);
}

void RegisterNewTabPagePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kNewTabPageShowTopSites, true);
}

}  // namespace new_tab_page
