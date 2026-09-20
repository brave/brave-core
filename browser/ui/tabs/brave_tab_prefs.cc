/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_prefs.h"

#include <string>

#include "base/command_line.h"
#include "base/feature_list.h"
#include "brave/browser/ui/tabs/public/switches.h"
#include "chrome/browser/ui/tabs/features.h"
#include "chrome/common/pref_names.h"
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

  if (base::FeatureList::IsEnabled(tabs::kBraveVerticalTabHideCompletely)) {
    registry->RegisterBooleanPref(kVerticalTabsHideCompletelyWhenCollapsed,
                                  false);
  }

  registry->RegisterBooleanPref(kVerticalTabsFloatingEnabled, true);
  registry->RegisterBooleanPref(kVerticalTabsShowToggleButton, true);
  registry->RegisterIntegerPref(kVerticalTabsExpandedWidth, 220);
  registry->RegisterBooleanPref(kVerticalTabsOnRight, false);
  registry->RegisterBooleanPref(kVerticalTabsShowScrollbar, false);
  registry->RegisterBooleanPref(kShowHorizontalTabScrollButtons, false);

  registry->RegisterBooleanPref(kSharedPinnedTab, false);

  if (base::FeatureList::IsEnabled(tabs::kBraveTreeTab)) {
    registry->RegisterBooleanPref(kTreeTabsEnabled, false);
  }

  registry->RegisterBooleanPref(kAlwaysHideTabCloseButton, false);
  registry->RegisterBooleanPref(kMiddleClickCloseTabEnabled, true);
  registry->RegisterIntegerPref(kTabMinWidthMode,
                                static_cast<int>(TabMinWidthMode::kMinimum));
  registry->RegisterBooleanPref(kScrollableHorizontalTabStrip, false);
  registry->RegisterBooleanPref(kAlwaysUseMiniAccentIcon, false);
}

void MigrateBraveProfilePrefs(PrefService* prefs) {
  if (auto* pref = prefs->FindPreference(kVerticalTabsShowScrollbar);
      pref && pref->IsDefaultValue() &&
      base::FeatureList::IsEnabled(tabs::kBraveVerticalTabScrollBar)) {
    prefs->SetBoolean(kVerticalTabsShowScrollbar, true);
  }
}

#if !BUILDFLAG(IS_ANDROID)
void MaybeApplyVerticalTabMigrationTestingOverride(PrefService* prefs) {
  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (!command_line->HasSwitch(tabs::switches::kVerticalTabMigrationSwitch)) {
    return;  // "default" choice (or flag untouched) - don't touch the pref.
  }
  const std::string value = command_line->GetSwitchValueASCII(
      tabs::switches::kVerticalTabMigrationSwitch);
  if (value == tabs::switches::kVerticalTabMigrationForceUpstreamValue) {
    prefs->SetBoolean(prefs::kVerticalTabsEnabled, true);
  } else if (value == tabs::switches::kVerticalTabMigrationResetValue) {
    // Unset -> falls back to upstream's own default (false); Brave's backend
    // takes over via VerticalTabController::SupportsBraveVerticalTabs().
    prefs->ClearPref(prefs::kVerticalTabsEnabled);
  }
}
#endif  // !BUILDFLAG(IS_ANDROID)

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kCompactHorizontalTabs, false);
}

bool AreTooltipsEnabled(PrefService* prefs) {
  return prefs->GetInteger(kTabHoverMode) == TabHoverMode::TOOLTIP;
}

bool AreCardPreviewsEnabled(PrefService* prefs) {
  return prefs->GetInteger(kTabHoverMode) == TabHoverMode::CARD_WITH_PREVIEW;
}

bool IsScrollableHorizontalTabStripEnabled(const PrefService* prefs) {
  return base::FeatureList::IsEnabled(tabs::kBraveScrollableTabStrip) &&
         prefs->GetBoolean(kScrollableHorizontalTabStrip);
}

}  // namespace brave_tabs
