/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_WEBSITE_SETTINGS_REGISTRY_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_WEBSITE_SETTINGS_REGISTRY_H_

#define GetInstance                           \
  GetInstance();                              \
  void Unregister(ContentSettingsType type) { \
    website_settings_info_.erase(type);       \
  }                                           \
  static WebsiteSettingsRegistry* GetInstance_Unused

#include <components/content_settings/core/browser/website_settings_registry.h>  // IWYU pragma: export
#undef GetInstance

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_WEBSITE_SETTINGS_REGISTRY_H_
