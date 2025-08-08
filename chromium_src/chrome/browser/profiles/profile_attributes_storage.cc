/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_attributes_storage.h"

#include "brave/browser/profiles/brave_profile_avatar_downloader.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/grit/branded_strings.h"

// We want to use our own default profile name "Personal" instead of upstream's
// branded default profile name "Your Brave".
#define IDS_PROFILE_MENU_PLACEHOLDER_PROFILE_NAME_SAVE \
  IDS_PROFILE_MENU_PLACEHOLDER_PROFILE_NAME
#undef IDS_PROFILE_MENU_PLACEHOLDER_PROFILE_NAME
#define IDS_PROFILE_MENU_PLACEHOLDER_PROFILE_NAME \
  IDS_PROFILE_MENU_BRAVE_PLACEHOLDER_PROFILE_NAME

#define ProfileAvatarDownloader BraveProfileAvatarDownloader
#include <chrome/browser/profiles/profile_attributes_storage.cc>
#undef ProfileAvatarDownloader
#undef IDS_PROFILE_MENU_PLACEHOLDER_PROFILE_NAME
#define IDS_PROFILE_MENU_PLACEHOLDER_PROFILE_NAME \
  IDS_PROFILE_MENU_PLACEHOLDER_PROFILE_NAME_SAVE
#undef IDS_PROFILE_MENU_PLACEHOLDER_PROFILE_NAME_SAVE
