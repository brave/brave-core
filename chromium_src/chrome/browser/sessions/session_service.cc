/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/constants/pref_names.h"

// Prevent detached tab has unnecessary tab restore steps.
// When last window's last tab is closed, |has_open_trackable_browsers_|
// becomes false. If we turn off "Close window when closing last tab" option,
// another new tab is created after last tab is closed. So, it's not the last
// actually. It could make ShouldRestore() give true when creating
// new browser by detaching a tab.
#define BRAVE_SESSION_SERVICE_TAB_CLOSED \
  if (profile()->GetPrefs()->GetBoolean(kEnableClosingLastTab))

#include "src/chrome/browser/sessions/session_service.cc"

#undef BRAVE_SESSION_SERVICE_TAB_CLOSED
