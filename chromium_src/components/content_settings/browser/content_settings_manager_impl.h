/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_

#include "base/containers/flat_map.h"
#include "components/content_settings/common/content_settings_manager.mojom.h"

#define OnContentBlocked                                      \
  NotUsed() {}                                                \
  void AllowEphemeralStorageAccess(                           \
      int32_t render_frame_id, const url::Origin& origin,     \
      const net::SiteForCookies& site_for_cookies,            \
      const url::Origin& top_frame_origin,                    \
      AllowEphemeralStorageAccessCallback callback) override; \
  void OnContentBlocked

#include "src/components/content_settings/browser/content_settings_manager_impl.h"  // IWYU pragma: export

#undef OnContentBlocked

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_
