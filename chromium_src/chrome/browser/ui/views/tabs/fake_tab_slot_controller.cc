/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/ui/views/tabs/fake_tab_slot_controller.cc"

bool FakeTabSlotController::IsTabTiled(const Tab* tab) const {
  return false;
}
bool FakeTabSlotController::IsFirstTabInTile(const Tab* tab) const {
  return false;
}
