/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "brave/browser/profiles/brave_profile_avatar_downloader.h"

#define ProfileAvatarDownloader BraveProfileAvatarDownloader
#include "src/chrome/browser/profiles/profile_attributes_storage.cc"
#undef ProfileAvatarDownloader
