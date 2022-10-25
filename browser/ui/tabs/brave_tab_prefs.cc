/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_prefs.h"

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_tabs {

const char kTabHoverMode[] = "brave.tabs.hover_mode";
const char kVerticalTabsCollapsed[] = "brave.tabs.vertical_tabs_collapsed";
const char kVerticalTabsShowTitleOnWindow[] =
    "brave.tabs.vertical_tabs_show_title_on_window";

void RegisterBraveProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kTabHoverMode, TabHoverMode::CARD);
  registry->RegisterBooleanPref(kVerticalTabsCollapsed, false);
  registry->RegisterBooleanPref(kVerticalTabsShowTitleOnWindow, true);
}

bool AreTooltipsEnabled(PrefService* prefs) {
  return prefs->GetInteger(kTabHoverMode) == TabHoverMode::TOOLTIP;
}

bool AreCardPreviewsEnabled(PrefService* prefs) {
  return prefs->GetInteger(kTabHoverMode) == TabHoverMode::CARD_WITH_PREVIEW;
}

}  // namespace brave_tabs
