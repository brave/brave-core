/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_

#include "base/containers/flat_map.h"
#include "brave/components/brave_shields/core/common/shields_settings.mojom.h"
#include "components/content_settings/common/content_settings_manager.mojom.h"

// Extend ContentSettingsManagerImpl::Delegate.
#define GetCookieSettings(...)                                \
  GetCookieSettings(__VA_ARGS__) = 0;                         \
  virtual void GetBraveShieldsSettings(                       \
      const content::GlobalRenderFrameHostToken& frame_token, \
      GetBraveShieldsSettingsCallback callback)

// Extend ContentSettingsManagerImpl.
#define OnContentBlocked                                                    \
  NotUsed() {}                                                              \
  void AllowEphemeralStorageAccess(                                         \
      const blink::LocalFrameToken& frame_token, const url::Origin& origin, \
      const net::SiteForCookies& site_for_cookies,                          \
      const url::Origin& top_frame_origin,                                  \
      AllowEphemeralStorageAccessCallback callback) override;               \
  void GetBraveShieldsSettings(const blink::LocalFrameToken& frame_token,   \
                               GetBraveShieldsSettingsCallback callback)    \
      override;                                                             \
  void OnContentBlocked

#include <components/content_settings/browser/content_settings_manager_impl.h>  // IWYU pragma: export

#undef OnContentBlocked
#undef GetCookieSettings

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_CONTENT_SETTINGS_BROWSER_CONTENT_SETTINGS_MANAGER_IMPL_H_
