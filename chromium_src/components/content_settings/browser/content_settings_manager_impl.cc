/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>

#include "components/content_settings/browser/content_settings_manager_impl.h"
#include "src/components/content_settings/browser/content_settings_manager_impl.cc"

namespace content_settings {

void ContentSettingsManagerImpl::AllowEphemeralStorageAccess(
    const blink::LocalFrameToken& frame_token,
    const url::Origin& origin,
    const net::SiteForCookies& site_for_cookies,
    const url::Origin& top_frame_origin,
    AllowEphemeralStorageAccessCallback callback) {
  url::Origin storage_origin;
  const bool should_use = cookie_settings_->ShouldUseEphemeralStorage(
      origin, site_for_cookies, top_frame_origin, storage_origin);
  std::move(callback).Run(should_use
                              ? std::make_optional<url::Origin>(storage_origin)
                              : std::nullopt);
}

}  // namespace content_settings
