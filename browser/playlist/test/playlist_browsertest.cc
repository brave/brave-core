/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/features.h"
#include "brave/components/playlist/playlist_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/test/base/chrome_test_utils.h"
#include "content/public/test/browser_test.h"

#if BUILDFLAG(IS_ANDROID)
#include "chrome/test/base/android/android_browser_test.h"
#else
#include "chrome/test/base/in_process_browser_test.h"
#endif

namespace playlist {

class PlaylistBrowserTest : public PlatformBrowserTest {
 public:
  PlaylistBrowserTest() {
    scoped_feature_list_.InitAndEnableFeature(playlist::features::kPlaylist);
  }
  ~PlaylistBrowserTest() override = default;

  PlaylistService* GetPlaylistService() {
    return PlaylistServiceFactory::GetInstance()->GetForBrowserContext(
        chrome_test_utils::GetProfile(this));
  }

  PrefService* GetPrefs() {
    return chrome_test_utils::GetProfile(this)->GetPrefs();
  }

 private:
  base::test::ScopedFeatureList scoped_feature_list_;
};

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_AddItemsToList) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_RemoveItemFromList) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_ThumbnailFailed) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_MediaDownloadFailed) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_ApiFunctions) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, DISABLED_CreateAndRemovePlaylist) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest,
                       DISABLED_RemoveAndRestoreLocalData) {
  // TODO(sko) Test the actual UI, once the spec and the implementation for it
  // are done https://github.com/brave/brave-browser/issues/25829.
}

}  // namespace playlist
