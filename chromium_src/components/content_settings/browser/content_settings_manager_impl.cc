/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "net/cookies/static_cookie_policy.h"
#include "third_party/blink/public/common/features.h"

#define BRAVE_ALLOW_STORAGE_ACCESS \
  ShouldUseEphemeralStorage(url, site_for_cookies, storage_type, &allowed);

#include "../../../../../components/content_settings/browser/content_settings_manager_impl.cc"
#undef BRAVE_ALLOW_STORAGE_ACCESS

namespace content_settings {
void ContentSettingsManagerImpl::ShouldUseEphemeralStorage(
    const GURL& url,
    const GURL& site_for_cookies,
    const StorageType storage_type,
    bool* allowed) const {
  if (!allowed || *allowed)
    return;

  if (storage_type == StorageType::LOCAL_STORAGE ||
      storage_type == StorageType::SESSION_STORAGE)
    *allowed =
        base::FeatureList::IsEnabled(blink::features::kBraveEphemeralStorage) &&
        CookieSettingsBase::IsThirdParty(url, site_for_cookies);
}

void ContentSettingsManagerImpl::IsEphemeralStorageAccessAllowed(
    const url::Origin& origin,
    const GURL& site_for_cookies,
    const url::Origin& top_frame_origin,
    const StorageType storage_type,
    base::OnceCallback<void(bool)> callback) {
  GURL url = origin.GetURL();
  bool allowed = false;
  ShouldUseEphemeralStorage(url, site_for_cookies, storage_type, &allowed);
  if (allowed) {
    // The ephemeral storage access is not allowed if third-party cookies is
    // enabled.
    allowed = !cookie_settings_->IsCookieAccessAllowed(url, site_for_cookies,
                                                       top_frame_origin);
  }

  std::move(callback).Run(allowed);
}

}  // namespace content_settings
