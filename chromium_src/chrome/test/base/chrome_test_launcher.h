/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_TEST_LAUNCHER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_TEST_LAUNCHER_H_
#include "brave/app/brave_main_delegate.h"

#define ChromeMainDelegate BraveMainDelegate
#include "src/chrome/test/base/chrome_test_launcher.h"  // IWYU pragma: export
#undef ChromeMainDelegate

#endif  // BRAVE_CHROMIUM_SRC_CHROME_TEST_BASE_CHROME_TEST_LAUNCHER_H_
