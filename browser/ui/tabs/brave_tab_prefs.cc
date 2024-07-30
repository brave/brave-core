/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_prefs.h"

#include "brave/browser/ui/tabs/features.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_tabs {

void RegisterBraveProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kTabHoverMode, TabHoverMode::CARD);
  registry->RegisterBooleanPref(kVerticalTabsEnabled, false);
  registry->RegisterBooleanPref(kVerticalTabsCollapsed, false);
  registry->RegisterBooleanPref(kVerticalTabsExpandedStatePerWindow, false);
#if BUILDFLAG(IS_WIN)
  // On Windows, we show window title by default
  // https://github.com/brave/brave-browser/issues/30027
  registry->RegisterBooleanPref(kVerticalTabsShowTitleOnWindow, true);
#else
  registry->RegisterBooleanPref(kVerticalTabsShowTitleOnWindow, false);
#endif
  registry->RegisterBooleanPref(kVerticalTabsFloatingEnabled, true);
  registry->RegisterIntegerPref(kVerticalTabsExpandedWidth, 220);
  registry->RegisterBooleanPref(kVerticalTabsOnRight, false);
  registry->RegisterBooleanPref(kVerticalTabsShowScrollbar, false);

  registry->RegisterBooleanPref(kSharedPinnedTab, false);
}

void MigrateBraveProfilePrefs(PrefService* prefs) {
  if (auto* pref = prefs->FindPreference(kVerticalTabsShowScrollbar);
      pref && pref->IsDefaultValue() &&
      base::FeatureList::IsEnabled(
          tabs::features::kBraveVerticalTabScrollBar)) {
    prefs->SetBoolean(kVerticalTabsShowScrollbar, true);
  }
}

bool AreTooltipsEnabled(PrefService* prefs) {
  return prefs->GetInteger(kTabHoverMode) == TabHoverMode::TOOLTIP;
}

bool AreCardPreviewsEnabled(PrefService* prefs) {
  return prefs->GetInteger(kTabHoverMode) == TabHoverMode::CARD_WITH_PREVIEW;
}

}  // namespace brave_tabs
