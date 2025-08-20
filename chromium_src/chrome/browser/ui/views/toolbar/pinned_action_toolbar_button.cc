/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/toolbar/pinned_action_toolbar_button.h"

#include "chrome/browser/ui/views/toolbar/pinned_toolbar_actions_container.h"
#include "chrome/browser/ui/views/toolbar/toolbar_button.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"

#define UpdateIcon UpdateIcon_ChromiumImpl
#include <chrome/browser/ui/views/toolbar/pinned_action_toolbar_button.cc>
#undef UpdateIcon

bool PinnedActionToolbarButton::ShouldShowMenu() {
  return false;
}

void PinnedActionToolbarButton::UpdateIcon() {
  if (HasIconEnabledColorsOverride() && action_engaged_) {
    ToolbarButton::UpdateIcon();
    return;
  }
  UpdateIcon_ChromiumImpl();
}
