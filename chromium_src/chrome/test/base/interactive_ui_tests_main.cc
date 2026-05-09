/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/test/base/brave_test_launcher_delegate.h"

#define ChromeTestLauncherDelegate BraveTestLauncherDelegate

#include <chrome/test/base/interactive_ui_tests_main.cc>

#undef ChromeTestLauncherDelegate
