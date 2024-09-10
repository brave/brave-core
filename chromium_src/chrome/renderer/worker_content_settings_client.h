/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_RENDERER_WORKER_CONTENT_SETTINGS_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_RENDERER_WORKER_CONTENT_SETTINGS_CLIENT_H_

#define BRAVE_WORKER_CONTENT_SETTINGS_CLIENT_H                       \
  brave_shields::mojom::ShieldsSettingsPtr GetBraveShieldsSettings(  \
      ContentSettingsType webcompat_settings_type) override;         \
  blink::WebSecurityOrigin GetEphemeralStorageOriginSync() override; \
  bool HasContentSettingsRules() const override;

#include "src/chrome/renderer/worker_content_settings_client.h"  // IWYU pragma: export

#undef BRAVE_WORKER_CONTENT_SETTINGS_CLIENT_H

#endif  // BRAVE_CHROMIUM_SRC_CHROME_RENDERER_WORKER_CONTENT_SETTINGS_CLIENT_H_
