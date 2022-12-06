// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"

#include "brave/components/constants/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "components/prefs/pref_service.h"

#define AlertIndicatorButton AlertIndicatorButtonBase

#include "src/chrome/browser/ui/views/tabs/alert_indicator_button.cc"

#undef AlertIndicatorButton

void AlertIndicatorButton::UpdateEnabledForMuteToggle() {
  const bool was_enabled = GetEnabled();
  auto* profile = GetTab()->controller()->GetBrowser()->profile();
  const bool pref_enabled =
      profile->GetPrefs()->GetBoolean(kBraveTabsMuteIndicatorClickable);

  // Upstream has clickable mute indicators disabled by default. Thus, if and
  // only if our pref is enabled (and the mute button isn't) we should set
  // enabled to true.
  // Note: We have a test which checks the feature is disabled by default. If
  // that changes this may need to as well.
  if (pref_enabled) {
    if (was_enabled != pref_enabled)
      SetEnabled(true);
    return;
  }
  AlertIndicatorButtonBase::UpdateEnabledForMuteToggle();
}
