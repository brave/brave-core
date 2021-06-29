/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/content_settings/browser/content_settings_manager_impl.h"

#include "../../../../../components/content_settings/browser/content_settings_manager_impl.cc"

namespace content_settings {

void ContentSettingsManagerImpl::AllowEphemeralStorageAccess(
    int32_t render_frame_id,
    StorageType storage_type,
    const url::Origin& origin,
    const GURL& site_for_cookies,
    const url::Origin& top_frame_origin,
    base::OnceCallback<void(bool)> callback) {
  std::move(callback).Run(cookie_settings_->ShouldUseEphemeralStorage(
      origin.GetURL(), site_for_cookies, top_frame_origin));
}

}  // namespace content_settings
