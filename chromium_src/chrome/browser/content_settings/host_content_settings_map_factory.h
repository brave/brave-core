/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_HOST_CONTENT_SETTINGS_MAP_FACTORY_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_HOST_CONTENT_SETTINGS_MAP_FACTORY_H_

#include "components/keyed_service/content/refcounted_browser_context_keyed_service_factory.h"
#include "components/keyed_service/core/refcounted_keyed_service_factory.h"

#define BuildServiceInstanceFor(...)                       \
  BuildServiceInstanceFor_ChromiumImpl(__VA_ARGS__) const; \
  scoped_refptr<RefcountedKeyedService> BuildServiceInstanceFor(__VA_ARGS__)

#include "src/chrome/browser/content_settings/host_content_settings_map_factory.h"  // IWYU pragma: export

#undef BuildServiceInstanceFor

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_HOST_CONTENT_SETTINGS_MAP_FACTORY_H_
