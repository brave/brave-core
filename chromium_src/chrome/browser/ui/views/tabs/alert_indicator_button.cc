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
  const bool not_clickable =
      profile->GetPrefs()->GetBoolean(kBraveTabsMuteIndicatorNotClickable);

  // We have clickable mute indicators enabled by default. Thus, if our pref is
  // disabled we can force the indicator off.
  // Note: We have a test which checks the feature is enabled by default. If
  // that changes this may need to as well.
  if (not_clickable) {
    if (was_enabled)
      SetEnabled(false);
    return;
  }
  AlertIndicatorButtonBase::UpdateEnabledForMuteToggle();
}
