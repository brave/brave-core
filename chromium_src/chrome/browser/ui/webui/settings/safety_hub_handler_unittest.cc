/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/safety_hub/safety_hub_test_util.h"
#include "components/content_settings/core/common/content_settings_types.h"

// clang-format off
#define FILE_SYSTEM_ACCESS_EXTENDED_PERMISSION      \
  FILE_SYSTEM_ACCESS_EXTENDED_PERMISSION,           \
      ContentSettingsType::BRAVE_FINGERPRINTING_V2, \
      ContentSettingsType::BRAVE_HTTPS_UPGRADE
// clang-format on

#define MIDI                                                         \
  BRAVE_WEBCOMPAT_NONE, ContentSettingsType::BRAVE_WEBCOMPAT_AUDIO,  \
      ContentSettingsType::BRAVE_WEBCOMPAT_CANVAS,                   \
      ContentSettingsType::BRAVE_WEBCOMPAT_DEVICE_MEMORY,            \
      ContentSettingsType::BRAVE_WEBCOMPAT_EVENT_SOURCE_POOL,        \
      ContentSettingsType::BRAVE_WEBCOMPAT_FONT,                     \
      ContentSettingsType::BRAVE_WEBCOMPAT_HARDWARE_CONCURRENCY,     \
      ContentSettingsType::BRAVE_WEBCOMPAT_KEYBOARD,                 \
      ContentSettingsType::BRAVE_WEBCOMPAT_LANGUAGE,                 \
      ContentSettingsType::BRAVE_WEBCOMPAT_MEDIA_DEVICES,            \
      ContentSettingsType::BRAVE_WEBCOMPAT_PLUGINS,                  \
      ContentSettingsType::BRAVE_WEBCOMPAT_SCREEN,                   \
      ContentSettingsType::BRAVE_WEBCOMPAT_SPEECH_SYNTHESIS,         \
      ContentSettingsType::BRAVE_WEBCOMPAT_USB_DEVICE_SERIAL_NUMBER, \
      ContentSettingsType::BRAVE_WEBCOMPAT_USER_AGENT,               \
      ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL,                    \
      ContentSettingsType::BRAVE_WEBCOMPAT_WEBGL2,                   \
      ContentSettingsType::BRAVE_WEBCOMPAT_WEB_SOCKETS_POOL,         \
      ContentSettingsType::BRAVE_WEBCOMPAT_ALL, ContentSettingsType::MIDI

#include "src/chrome/browser/ui/webui/settings/safety_hub_handler_unittest.cc"
#undef FILE_SYSTEM_ACCESS_EXTENDED_PERMISSION
#undef MIDI
