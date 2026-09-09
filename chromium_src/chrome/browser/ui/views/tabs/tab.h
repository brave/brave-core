/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_

#include "base/memory/raw_ref.h"
#include "chrome/browser/ui/views/tabs/hovercard/hover_card_anchor_target.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "components/tabs/public/tab_interface.h"

class BraveTabTest;
class BraveTab;
class Tab;

// A struct to wrap the state of whether the close button is shown on a tab,
// taking into account whether the controller wants to always hide close
// buttons.
struct ControllableCloseButtonState final {
  ControllableCloseButtonState(TabSlotController& controller, Tab& tab);
  ~ControllableCloseButtonState();

  bool operator=(bool show);

  // Disable "explicit" enforcement to allow implicit conversion to bool.
  // NOLINTNEXTLINE(google-explicit-constructor)
  operator bool() const;

  // Controller of the tab that owns this state.
  raw_ref<TabSlotController> controller;

  // Owner of this state.
  raw_ref<Tab> tab;

  bool showing_close_button = false;
};

#include <chrome/browser/ui/views/tabs/tab.h>  // IWYU pragma: export

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_TABS_TAB_H_
