/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_H_

#define RegisterProfilePrefs                       \
  RegisterProfilePrefs_ChromiumImpl(               \
      user_prefs::PrefRegistrySyncable* registry); \
  static void RegisterProfilePrefs

#include <components/password_manager/core/browser/password_manager.h>  // IWYU pragma: export

#undef RegisterProfilePrefs

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PASSWORD_MANAGER_CORE_BROWSER_PASSWORD_MANAGER_H_
