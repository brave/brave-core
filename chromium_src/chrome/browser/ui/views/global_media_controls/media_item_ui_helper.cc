// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/global_media_controls/media_item_ui_helper.h"

#include "chrome/browser/media/router/media_router_feature.h"

#define BuildDeviceSelector BuildDeviceSelector_ChromiumImpl

#include "src/chrome/browser/ui/views/global_media_controls/media_item_ui_helper.cc"

#undef BuildDeviceSelector

// TODO(simonhong): Delete this when upstream fixes
// https://issues.chromium.org/u/3/issues/393606982.
std::unique_ptr<global_media_controls::MediaItemUIDeviceSelector>
BuildDeviceSelector(
    const std::string& id,
    base::WeakPtr<media_message_center::MediaNotificationItem> item,
    global_media_controls::mojom::DeviceService* device_service,
    MediaItemUIDeviceSelectorDelegate* selector_delegate,
    Profile* profile,
    global_media_controls::GlobalMediaControlsEntryPoint entry_point,
    bool show_devices,
    std::optional<media_message_center::MediaColorTheme> media_color_theme) {
  if (!media_router::MediaRouterEnabled(profile)) {
    return nullptr;
  }

  return BuildDeviceSelector_ChromiumImpl(
      id, item, device_service, selector_delegate, profile, entry_point,
      show_devices, media_color_theme);
}
