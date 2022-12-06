// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/tabs/alert_indicator_button.h"

#define AlertIndicatorButton AlertIndicatorButtonBase

#include "src/chrome/browser/ui/views/tabs/alert_indicator_button.cc"

#undef AlertIndicatorButton

void AlertIndicatorButton::UpdateEnabledForMuteToggle() {
  LOG(ERROR) << "Toggled mute";
  AlertIndicatorButtonBase::UpdateEnabledForMuteToggle();
}
