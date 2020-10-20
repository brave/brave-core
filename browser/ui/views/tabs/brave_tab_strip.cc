/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/tabs/brave_tab_strip.h"

#include "chrome/browser/ui/views/tabs/tab.h"

BraveTabStrip::~BraveTabStrip() = default;

bool BraveTabStrip::ShouldHideCloseButtonForTab(Tab* tab) const {
  bool should_hide = TabStrip::ShouldHideCloseButtonForTab(tab);

  // If upstream logic want to hide, follow it.
  if (should_hide)
    return should_hide;

  if (tab->IsActive())
    return false;

  // Only shows close button on tab when mouse is hovered on tab.
  return !tab->mouse_hovered();
}
