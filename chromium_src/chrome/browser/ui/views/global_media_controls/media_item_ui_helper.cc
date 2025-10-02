/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/global_media_controls/media_item_ui_helper.h"

#define BuildDeviceSelector(...) BuildDeviceSelector_ChromiumImpl(__VA_ARGS__)

#include <chrome/browser/ui/views/global_media_controls/media_item_ui_helper.cc>

#undef BuildDeviceSelector

std::unique_ptr<global_media_controls::MediaItemUIDeviceSelector>
BuildDeviceSelector(
    const std::string& id,
    base::WeakPtr<media_message_center::MediaNotificationItem> item,
    global_media_controls::mojom::DeviceService* device_service,
    MediaItemUIDeviceSelectorDelegate* selector_delegate,
    Profile* profile,
    global_media_controls::GlobalMediaControlsEntryPoint entry_point,
    bool show_devices,
    std::optional<media_message_center::MediaColorTheme> media_color_theme,
    Profile* profile_to_check) {
  if (profile_to_check && profile_to_check->IsTor()) {
    return nullptr;
  }
  return BuildDeviceSelector_ChromiumImpl(
      id, std::move(item), device_service, selector_delegate, profile,
      entry_point, show_devices, std::move(media_color_theme));
}
