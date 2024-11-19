/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_WEAK_CHECK_UTILITY_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_WEAK_CHECK_UTILITY_H_

#include <string>

#define BulkWeakCheck(...)    \
  BulkWeakCheck(__VA_ARGS__); \
  int GetPasswordStrength(const std::string& password)
// Returns strength for `password` on a scale from 0 to 100.

#include "src/components/password_manager/core/browser/ui/weak_check_utility.h"  // IWYU pragma: export

#undef BulkWeakCheck

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_UI_WEAK_CHECK_UTILITY_H_
