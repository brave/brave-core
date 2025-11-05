/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/global_media_controls/media_dialog_view.h"

#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/views/global_media_controls/media_item_ui_helper.h"

#define GetOriginalProfile(...)     \
  GetOriginalProfile(__VA_ARGS__)),\
  profile_to_check_(profile

#define BuildDeviceSelector(...) \
  BuildDeviceSelector(__VA_ARGS__, profile_to_check_)

#include <chrome/browser/ui/views/global_media_controls/media_dialog_view.cc>

#undef GetOriginalProfile
#undef BuildDeviceSelector
