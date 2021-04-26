/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_

#include "components/content_settings/common/content_settings_manager.mojom.h"

#define OnContentBlocked                                       \
  NotUsed() {}                                                 \
  void AllowEphemeralStorageAccess(                            \
      int32_t render_frame_id, StorageType storage_type,       \
      const url::Origin& origin, const GURL& site_for_cookies, \
      const url::Origin& top_frame_origin,                     \
      base::OnceCallback<void(bool)> callback) override;       \
  void OnContentBlocked

#include "../../../../../components/content_settings/browser/content_settings_manager_impl.h"

#undef OnContentBlocked

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_
