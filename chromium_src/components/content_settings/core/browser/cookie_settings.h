/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_COOKIE_SETTINGS_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_COOKIE_SETTINGS_H_

#include <vector>

#include "base/containers/flat_map.h"
#include "components/content_settings/core/browser/content_settings_provider.h"
#include "components/keyed_service/core/refcounted_keyed_service.h"
#include "url/origin.h"

#define ShutdownOnUIThread                                                   \
  ShutdownOnUIThread_ChromiumImpl();                                         \
  bool ShouldUseEphemeralStorage(                                            \
      const url::Origin& origin, const GURL& site_for_cookies,               \
      const absl::optional<url::Origin>& top_frame_origin,                   \
      url::Origin& storage_origin);                                          \
  std::vector<url::Origin> TakeEphemeralStorageOpaqueOrigins(                \
      const std::string& ephemeral_storage_domain);                          \
                                                                             \
 private:                                                                    \
  /* Ephemeral storage domain to non_opaque->opaque origins map. */          \
  using EphemeralStorageOrigins =                                            \
      base::flat_map<std::string, base::flat_map<url::Origin, url::Origin>>; \
  EphemeralStorageOrigins ephemeral_storage_origins_;                        \
                                                                             \
 public:                                                                     \
  void ShutdownOnUIThread

#include "../../../../../../components/content_settings/core/browser/cookie_settings.h"

#undef ShutdownOnUIThread

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_CORE_BROWSER_COOKIE_SETTINGS_H_
