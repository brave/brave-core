/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_AVATAR_DOWNLOADER_H_
#define BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_AVATAR_DOWNLOADER_H_

#include "chrome/browser/profiles/profile_avatar_downloader.h"

class BraveProfileAvatarDownloader : public ProfileAvatarDownloader {
 public:
  BraveProfileAvatarDownloader(size_t icon_index,
                               FetchCompleteCallback callback);
  ~BraveProfileAvatarDownloader() override;

  void Start();

 private:
  FetchCompleteCallback callback_;
};

#endif  // BRAVE_BROWSER_PROFILES_BRAVE_PROFILE_AVATAR_DOWNLOADER_H_
