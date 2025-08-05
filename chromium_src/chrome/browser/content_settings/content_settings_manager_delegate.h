/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_MANAGER_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_MANAGER_DELEGATE_H_

#include "components/content_settings/browser/content_settings_manager_impl.h"

#define GetCookieSettings(...)                                \
  GetCookieSettings(__VA_ARGS__) override;                    \
  void GetBraveShieldsSettings(                               \
      const content::GlobalRenderFrameHostToken& frame_token, \
      content_settings::mojom::ContentSettingsManager::       \
          GetBraveShieldsSettingsCallback callback)

#include <chrome/browser/content_settings/content_settings_manager_delegate.h>  // IWYU pragma: export

#undef GetCookieSettings

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_CONTENT_SETTINGS_CONTENT_SETTINGS_MANAGER_DELEGATE_H_
