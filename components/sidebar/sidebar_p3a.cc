/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/sidebar/sidebar_p3a.h"

#include "base/metrics/histogram_macros.h"
#include "brave/components/sidebar/pref_names.h"
#include "brave/components/sidebar/sidebar_service.h"
#include "components/prefs/pref_service.h"

namespace sidebar {
namespace p3a {

SidebarP3A::SidebarP3A(PrefService* profile_prefs)
    : profile_prefs_(profile_prefs) {
  profile_pref_change_registrar_.Init(profile_prefs);
  profile_pref_change_registrar_.Add(
      kSidebarShowOption, base::BindRepeating(&SidebarP3A::RecordEnabledSetting,
                                              base::Unretained(this)));

  RecordEnabledSetting();
}

SidebarP3A::~SidebarP3A() = default;

void SidebarP3A::RecordEnabledSetting() {
  bool is_enabled =
      profile_prefs_->GetInteger(kSidebarShowOption) !=
      static_cast<int>(SidebarService::ShowSidebarOption::kShowNever);
  int answer = is_enabled ? 1 : (INT_MAX - 1);
  UMA_HISTOGRAM_EXACT_LINEAR(kSidebarEnabledHistogramName, answer, 2);
}

}  // namespace p3a
}  // namespace sidebar
