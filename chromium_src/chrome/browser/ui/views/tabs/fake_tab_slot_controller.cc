/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <chrome/browser/ui/views/tabs/fake_tab_slot_controller.cc>

bool FakeTabSlotController::ShouldAlwaysHideCloseButton() const {
  return should_always_hide_close_button_;
}

bool FakeTabSlotController::CanCloseTabViaMiddleButtonClick() const {
  return can_close_tab_via_middle_button_click_;
}

bool FakeTabSlotController::IsVerticalTabsFloating() const {
  return false;
}

bool FakeTabSlotController::ShouldPaintTabAccent(const Tab* tab) const {
  return false;
}

std::optional<SkColor> FakeTabSlotController::GetTabAccentColor(
    const Tab* tab) const {
  return std::nullopt;
}

ui::ImageModel FakeTabSlotController::GetTabAccentIcon(const Tab* tab) const {
  return ui::ImageModel();
}
