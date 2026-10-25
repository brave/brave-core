/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/peek/public/peek_controller.h"

#include "chrome/browser/ui/browser_window/public/browser_window_interface.h"

DEFINE_USER_DATA(PeekController);

PeekController::PeekController(BrowserWindowInterface& browser)
    : scoped_unowned_user_data_(browser.GetUnownedUserDataHost(), *this) {}

PeekController::~PeekController() = default;

// static
PeekController* PeekController::From(BrowserWindowInterface* browser) {
  return browser ? Get(browser->GetUnownedUserDataHost()) : nullptr;
}
