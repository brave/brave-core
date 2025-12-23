// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "components/prefs/pref_service.h"
#include "ui/base/metadata/metadata_impl_macros.h"

#define AlertIndicatorButton AlertIndicatorButtonBase

#include <chrome/browser/ui/views/tabs/alert_indicator_button.cc>

#undef AlertIndicatorButton

void AlertIndicatorButton::UpdateEnabledForMuteToggle() {
  const bool was_enabled = GetEnabled();
  if (delegate_->IsTabMuteIndicatorNotClickable()) {
    if (was_enabled)
      SetEnabled(false);
    return;
  }
  AlertIndicatorButtonBase::UpdateEnabledForMuteToggle();
}

BEGIN_METADATA(AlertIndicatorButton)
END_METADATA
