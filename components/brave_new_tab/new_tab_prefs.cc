/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_new_tab/new_tab_prefs.h"

#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"

namespace brave_new_tab::prefs {

namespace {

constexpr auto kDefaultNewTabShowsOption = NewTabShowsOption::kDashboard;

}  // namespace

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  CHECK(registry);
  registry->RegisterIntegerPref(kNewTabShowsOption,
                                static_cast<int>(kDefaultNewTabShowsOption));
}

NewTabShowsOption GetNewTabShowsOption(PrefService* pref_service) {
  CHECK(pref_service);
  switch (pref_service->GetInteger(kNewTabShowsOption)) {
    case static_cast<int>(NewTabShowsOption::kDashboard):
      return NewTabShowsOption::kDashboard;
    case static_cast<int>(NewTabShowsOption::kHomepage):
      return NewTabShowsOption::kHomepage;
    case static_cast<int>(NewTabShowsOption::kBlankpage):
      return NewTabShowsOption::kBlankpage;
  }
  pref_service->SetInteger(kNewTabShowsOption,
                           static_cast<int>(kDefaultNewTabShowsOption));
  return kDefaultNewTabShowsOption;
}

}  // namespace brave_new_tab::prefs
