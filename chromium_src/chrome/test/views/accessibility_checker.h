/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_TEST_VIEWS_ACCESSIBILITY_CHECKER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_TEST_VIEWS_ACCESSIBILITY_CHECKER_H_

#include "chrome/browser/ui/views/chrome_views_delegate.h"

#if defined(OS_LINUX)
#include "brave/browser/ui/views/brave_views_delegate_linux.h"
#define ChromeViewsDelegate BraveViewsDelegateLinux
#endif
#include "src/chrome/test/views/accessibility_checker.h"
#if defined(OS_LINUX)
#undef ChromeViewsDelegate
#endif

#endif  // BRAVE_CHROMIUM_SRC_CHROME_TEST_VIEWS_ACCESSIBILITY_CHECKER_H_
