/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/test/base/brave_test_launcher_delegate.h"

// Apply Brave-specific behavior change in the test launcher delegate.
// * Suppress first run dialog (Mac/Linux).
// * Suppress browser window closing dialog.
#define ChromeTestLauncherDelegate BraveTestLauncherDelegate

#include <chrome/test/base/interactive_ui_tests_main.cc>

#undef ChromeTestLauncherDelegate
