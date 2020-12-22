/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_

#define BRAVE_CONTENT_SETTINGS_MANAGER_IMPL_H                  \
 public:                                                       \
  void AllowStorageAccessWithoutEphemeralStorage(              \
      int32_t render_frame_id, StorageType storage_type,       \
      const url::Origin& origin, const GURL& site_for_cookies, \
      const url::Origin& top_frame_origin,                     \
      base::OnceCallback<void(bool)> callback) override;

#include "../../../../../components/content_settings/browser/content_settings_manager_impl.h"

#undef BRAVE_CONTENT_SETTINGS_MANAGER_IMPL_H

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_
