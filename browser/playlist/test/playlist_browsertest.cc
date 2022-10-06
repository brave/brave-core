/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/browser/playlist/playlist_service_factory.h"
#include "brave/components/playlist/browser/playlist_service.h"
#include "brave/components/playlist/common/features.h"
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

IN_PROC_BROWSER_TEST_F(PlaylistBrowserTest, RemoveAndRestoreLocalData) {
  VLOG(2) << "create playlist 1";
  ResetStatus();
  service->CreatePlaylistItem(GetValidCreateParams());
  WaitForEvents(3);

  // pre condition: there's an already downloaded playlist item.
  auto items = service->GetAllPlaylistItems();
  ASSERT_EQ(1UL, items.size());

  auto item = items.front();
  ASSERT_TRUE(item.media_file_cached);
  ASSERT_NE(item.media_src, item.media_file_path);
  ASSERT_NE(item.thumbnail_src, item.thumbnail_path);
  auto dir_path = service->GetPlaylistItemDirPath(item.id);
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    ASSERT_TRUE(base::DirectoryExists(dir_path));
  }

  // Store the item's local file path first
  items = service->GetAllPlaylistItems();
  item = items.front();
  base::FilePath media_path;
  base::FilePath thumbnail_path;
  ASSERT_TRUE(service->GetMediaPath(item.id, &media_path));
  ASSERT_TRUE(service->GetThumbnailPath(item.id, &thumbnail_path));

  // Remove local data for the item. When we remove local data, we remove only
  // media file.
  service->DeletePlaylistLocalData(items.front().id);
  items = service->GetAllPlaylistItems();
  EXPECT_EQ(1UL, items.size());
  item = items.front();

  auto file_exists = [](const base::FilePath& path) {
    return base::PathExists(path) && !base::DirectoryExists(path);
  };

  // Values are updated first and then the data from disk will be removed.
  EXPECT_FALSE(item.media_file_cached);
  EXPECT_EQ(item.media_src, item.media_file_path);
  EXPECT_NE(item.thumbnail_src, item.thumbnail_path);
  WaitUntil(base::BindLambdaForTesting([&]() {
    base::ScopedAllowBlockingForTesting allow_blocking;
    return !file_exists(media_path);
  }));

  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::DirectoryExists(dir_path));
    EXPECT_TRUE(file_exists(thumbnail_path));
    EXPECT_FALSE(file_exists(media_path));
  }

  // Restore local data for the item.
  service->RecoverPlaylistItem(item.id);
  items = service->GetAllPlaylistItems();
  EXPECT_EQ(1UL, items.size());

  item = items.front();
  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::DirectoryExists(dir_path));
  }

  WaitUntil(base::BindLambdaForTesting([&]() {
    auto items = service->GetAllPlaylistItems();
    return items.size() && items.front().media_file_cached;
  }));
  item = service->GetAllPlaylistItems().front();
  EXPECT_NE(item.media_src, item.media_file_path);

  {
    base::ScopedAllowBlockingForTesting allow_blocking;
    EXPECT_TRUE(base::DirectoryExists(dir_path));
    EXPECT_TRUE(file_exists(thumbnail_path));
    EXPECT_TRUE(file_exists(media_path));
  }
}

}  // namespace playlist
