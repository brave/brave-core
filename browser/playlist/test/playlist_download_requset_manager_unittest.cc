/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_download_request_manager.h"

#include "base/json/values_util.h"
#include "base/test/gtest_util.h"
#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace playlist {

////////////////////////////////////////////////////////////////////////////////
// PlaylistDownloadRequestManagerUnitTest fixture
class PlaylistDownloadRequestManagerUnitTest : public testing::Test {
 public:
  PlaylistDownloadRequestManagerUnitTest() = default;
  ~PlaylistDownloadRequestManagerUnitTest() override = default;

  PlaylistDownloadRequestManager& manager() {
    return *download_request_manager_;
  }

  std::vector<mojom::PlaylistItemPtr> GetPlaylistItems(base::Value value) {
    return manager().GetPlaylistItems(std::move(value),
                                      GURL("https://example.com"));
  }

  // testing::Test:
  void SetUp() override {
    media_detector_component_manager_ =
        std::make_unique<MediaDetectorComponentManager>(nullptr);
    download_request_manager_ =
        std::make_unique<PlaylistDownloadRequestManager>(
            nullptr, nullptr, media_detector_component_manager_.get());
  }

 private:
  std::unique_ptr<MediaDetectorComponentManager>
      media_detector_component_manager_;
  std::unique_ptr<PlaylistDownloadRequestManager> download_request_manager_;
};

TEST_F(PlaylistDownloadRequestManagerUnitTest,
       CanCacheMedia_HttpsScheme_ReturnsTrue) {
  mojom::PlaylistItemPtr item = mojom::PlaylistItem::New();
  item->media_source = GURL("https://example.com/media.mp4");
  EXPECT_TRUE(manager().CanCacheMedia(item));
}

TEST_F(PlaylistDownloadRequestManagerUnitTest,
       CanCacheMedia_NonHttpsScheme_ReturnsFalse) {
  // We don't allow cache non-https media.
  mojom::PlaylistItemPtr item = mojom::PlaylistItem::New();
  item->media_source = GURL("http://example.com/media.mp4");
  EXPECT_FALSE(manager().CanCacheMedia(item));
}

TEST_F(PlaylistDownloadRequestManagerUnitTest,
       CanCacheMedia_BlobFromMediaSource_ReturnsTrue) {
  mojom::PlaylistItemPtr item = mojom::PlaylistItem::New();
  // A site known to be give us a plain media URL when we hide MediaSource API
  item->media_source = GURL("blob:https://youtube.com/12345");
  item->is_blob_from_media_source = true;
  EXPECT_TRUE(manager().CanCacheMedia(item));

  // A site known to be give us a plain media URL when we use fake(iOS) UA
  item->media_source = GURL("blob:https://ted.com/12345");
  EXPECT_TRUE(manager().CanCacheMedia(item));
}

TEST_F(PlaylistDownloadRequestManagerUnitTest,
       CanCacheMedia_BlobFromMediaSourceButUnknown_ReturnsFalse) {
  mojom::PlaylistItemPtr item = mojom::PlaylistItem::New();
  // At this moment, even a media url is from MediaSource, we can't cache it.
  // Still work in progress.
  item->media_source = GURL("blob:https://example.com/12345");
  item->is_blob_from_media_source = true;
  EXPECT_FALSE(manager().CanCacheMedia(item));
}

TEST_F(PlaylistDownloadRequestManagerUnitTest,
       CanCacheMedia_BlobNotFromMediaSource_ReturnsFalse) {
  mojom::PlaylistItemPtr item = mojom::PlaylistItem::New();
  item->media_source = GURL("blob:https://youtube.com/12345");
  item->is_blob_from_media_source = false;
  EXPECT_FALSE(manager().CanCacheMedia(item));
}

TEST_F(PlaylistDownloadRequestManagerUnitTest,
       ShouldExtractMediaFromBackgroundWebContents_UnsupportedURL_ExpectDeath) {
  mojom::PlaylistItemPtr item = mojom::PlaylistItem::New();
  item->media_source = GURL("http://example.com/media.mp4");
  ASSERT_FALSE(manager().CanCacheMedia(item));

  BASE_EXPECT_DEATH(manager().ShouldExtractMediaFromBackgroundWebContents(item),
                    CHECK_WILL_STREAM() ? "NOTREACHED hit. " : "");
}

TEST_F(PlaylistDownloadRequestManagerUnitTest,
       GetPlaylistItems_NoMediaDetected) {
  // When media detection script doesn't find any media, it returns an empty
  // dict.
  EXPECT_TRUE(GetPlaylistItems(base::Value(base::Value::Type::DICT)).empty());
}

TEST_F(PlaylistDownloadRequestManagerUnitTest, GetPlaylistItems_InvalidValue) {
  // GetPlaylistItems only takes either list or dict
  BASE_EXPECT_DEATH(GetPlaylistItems(base::Value(base::Value::Type::BOOLEAN)),
                    CHECK_WILL_STREAM() ? "Check failed: value\\.is_list" : "");
}

TEST_F(PlaylistDownloadRequestManagerUnitTest,
       GetPlaylistItems_RequiredProperties) {
  base::Value value(base::Value::Dict()
                        .Set("name", "Video 1")
                        .Set("pageTitle", "Example page")
                        .Set("pageSrc", "https://example.com/redirected")
                        .Set("mimeType", "video")
                        .Set("src", "https://example.com/video.mp4")
                        .Set("srcIsMediaSourceObjectURL", false));

  std::vector<mojom::PlaylistItemPtr> result =
      GetPlaylistItems(base::Value(base::Value::List().Append(value.Clone())));
  EXPECT_EQ(result.size(), 1u);
  EXPECT_FALSE(result[0]->id.empty());
  EXPECT_EQ(result[0]->name, "Video 1");
  EXPECT_EQ(result[0]->page_source, GURL("https://example.com"));
  EXPECT_EQ(result[0]->page_redirected, GURL("https://example.com/redirected"));
  EXPECT_EQ(result[0]->media_source, GURL("https://example.com/video.mp4"));
  EXPECT_EQ(result[0]->media_path, GURL("https://example.com/video.mp4"));
  EXPECT_FALSE(result[0]->is_blob_from_media_source);

  // When base::Value has required properties but invalid value, should be
  // filtered.
  for (auto* required_property : {"name", "pageTitle", "pageSrc", "mimeType",
                                  "src", "srcIsMediaSourceObjectURL"}) {
    auto invalid_value = value.Clone();
    invalid_value.GetDict().Set(required_property, base::Value());
    EXPECT_TRUE(GetPlaylistItems(base::Value(base::Value::List().Append(
                                     std::move(invalid_value))))
                    .empty());
  }
}

TEST_F(PlaylistDownloadRequestManagerUnitTest,
       GetPlaylistItems_OptionalProperties) {
  base::Value value(base::Value::Dict()
                        .Set("name", "Video 1")
                        .Set("pageTitle", "Example page")
                        .Set("pageSrc", "https://example.com/redirected")
                        .Set("mimeType", "video")
                        .Set("src", "https://example.com/video.mp4")
                        .Set("srcIsMediaSourceObjectURL", false)
                        .Set("author", "Me")
                        .Set("thumbnail", "https://example.com/thumbnail.jpg")
                        .Set("duration", 1234.0));

  std::vector<mojom::PlaylistItemPtr> result =
      GetPlaylistItems(base::Value(base::Value::List().Append(value.Clone())));
  EXPECT_EQ(result.size(), 1u);
  EXPECT_FALSE(result[0]->id.empty());
  EXPECT_EQ(result[0]->author, "Me");
  EXPECT_EQ(result[0]->thumbnail_source,
            GURL("https://example.com/thumbnail.jpg"));
  EXPECT_EQ(result[0]->thumbnail_path,
            GURL("https://example.com/thumbnail.jpg"));
  EXPECT_EQ(result[0]->duration,
            base::TimeDeltaToValue(base::Seconds(1234)).GetString());
}

TEST_F(PlaylistDownloadRequestManagerUnitTest,
       GetPlaylistItems_MediaSourceScheme) {
  base::Value value(base::Value::Dict()
                        .Set("name", "Video 1")
                        .Set("pageTitle", "Example page")
                        .Set("pageSrc", "https://example.com/redirected")
                        .Set("mimeType", "video")
                        .Set("src", "https://example.com/video.mp4")
                        .Set("srcIsMediaSourceObjectURL", false));

  EXPECT_FALSE(
      GetPlaylistItems(base::Value(base::Value::List().Append(value.Clone())))
          .empty());

  // http:// scheme is not allowed.
  value.GetDict().Set("src", "http://example.com/12345");
  EXPECT_TRUE(
      GetPlaylistItems(base::Value(base::Value::List().Append(value.Clone())))
          .empty());

  // blob: that's not backed by MediaSource is not allowed
  value.GetDict().Set("src", "blob:https://example.com/12345");
  EXPECT_TRUE(
      GetPlaylistItems(base::Value(base::Value::List().Append(value.Clone())))
          .empty());

  // blob: that's backed by MediaSource but from unknown source is not allowed
  value.GetDict().Set("src", "blob:https://example.com/12345");
  value.GetDict().Set("srcIsMediaSourceObjectURL", true);
  EXPECT_TRUE(
      GetPlaylistItems(base::Value(base::Value::List().Append(value.Clone())))
          .empty());

  // blob: that's backed by MediaSource and from known source is allowed
  value.GetDict().Set("src", "blob:https://youtube.com/12345");
  EXPECT_FALSE(
      GetPlaylistItems(base::Value(base::Value::List().Append(value.Clone())))
          .empty());
}

}  // namespace playlist
