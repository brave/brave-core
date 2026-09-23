/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_PERMISSION_SETTINGS_REGISTRY_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_PERMISSION_SETTINGS_REGISTRY_H_

#define ResetForTesting                       \
  ResetForTesting();                          \
  void Unregister(ContentSettingsType type) { \
    permission_settings_info_.erase(type);    \
  }                                           \
  void ResetForTesting_Unused

#include <components/content_settings/core/browser/permission_settings_registry.h>  // IWYU pragma: export
#undef ResetForTesting

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_PERMISSION_SETTINGS_REGISTRY_H_
