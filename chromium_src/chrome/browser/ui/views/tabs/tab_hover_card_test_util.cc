/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

// This following include is needed because of the upstream change
// https://chromium.googlesource.com/chromium/src/+/4021762e. It added
// TabHoverCardController* hover_card_controller_for_testing() method to
// TabStrip class, which is called in this file.But, we use chromium_src
// override on TabStrip to replace TabHoverCardController with
// BraveTabHoverCardController, so we need to add the header to avoid the
// member access into incomplete type error.
#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"

#include "src/chrome/browser/ui/views/tabs/tab_hover_card_test_util.cc"
