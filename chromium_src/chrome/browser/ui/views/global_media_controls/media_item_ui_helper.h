// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_GLOBAL_MEDIA_CONTROLS_MEDIA_ITEM_UI_HELPER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_GLOBAL_MEDIA_CONTROLS_MEDIA_ITEM_UI_HELPER_H_

#define BuildDeviceSelector(...) \
  BuildDeviceSelector(__VA_ARGS__, Profile* profile_to_check = nullptr)

#include <chrome/browser/ui/views/global_media_controls/media_item_ui_helper.h>  // IWYU pragma: export

#undef BuildDeviceSelector

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_GLOBAL_MEDIA_CONTROLS_MEDIA_ITEM_UI_HELPER_H_
