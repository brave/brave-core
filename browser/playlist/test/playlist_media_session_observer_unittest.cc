/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_media_session_observer.h"

#include <memory>
#include <optional>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/functional/callback_helpers.h"
#include "base/test/bind.h"
#include "base/time/time.h"
#include "brave/browser/playlist/test/playlist_unittest_base.h"
#include "services/media_session/public/cpp/media_image.h"
#include "services/media_session/public/cpp/media_metadata.h"
#include "services/media_session/public/cpp/media_position.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "ui/gfx/geometry/size.h"
#include "url/gurl.h"

namespace playlist {

namespace {

using MediaSessionImageType = media_session::mojom::MediaSessionImageType;

media_session::MediaImage MakeImage(const char* src,
                                    const std::vector<gfx::Size>& sizes) {
  media_session::MediaImage image;
  image.src = GURL(src);
  image.sizes = sizes;
  return image;
}

}  // namespace

class PlaylistMediaSessionObserverTest : public PlaylistUnitTestBase {
 public:
  void SetUp() override {
    PlaylistUnitTestBase::SetUp();
    observer_ = std::make_unique<PlaylistMediaSessionObserver>(
        web_contents(),
        base::BindLambdaForTesting([this] { ++change_count_; }));
  }

  void TearDown() override {
    observer_.reset();
    PlaylistUnitTestBase::TearDown();
  }

 protected:
  std::unique_ptr<PlaylistMediaSessionObserver> observer_;
  int change_count_ = 0;
};

TEST_F(PlaylistMediaSessionObserverTest, StartsEmpty) {
  EXPECT_TRUE(observer_->metadata().empty());
}

TEST_F(PlaylistMediaSessionObserverTest, TakesTitleAndArtist) {
  media_session::MediaMetadata metadata;
  metadata.title = u"A Song";
  metadata.artist = u"An Artist";
  metadata.source_title = u"example.com";
  observer_->MediaSessionMetadataChanged(metadata);

  EXPECT_EQ(u"A Song", observer_->metadata().title);
  EXPECT_EQ(u"An Artist", observer_->metadata().artist);
  EXPECT_FALSE(observer_->metadata().empty());
  EXPECT_EQ(1, change_count_);
}

TEST_F(PlaylistMediaSessionObserverTest, FallsBackToSourceTitleForArtist) {
  media_session::MediaMetadata metadata;
  metadata.title = u"A Song";
  metadata.source_title = u"example.com";
  observer_->MediaSessionMetadataChanged(metadata);

  EXPECT_EQ(u"example.com", observer_->metadata().artist);
}

TEST_F(PlaylistMediaSessionObserverTest, PicksLargestHttpsArtwork) {
  base::flat_map<MediaSessionImageType, std::vector<media_session::MediaImage>>
      images;
  images[MediaSessionImageType::kArtwork] = {
      MakeImage("https://example.com/small.png", {gfx::Size(64, 64)}),
      MakeImage("https://example.com/large.png", {gfx::Size(512, 512)}),
      MakeImage("https://example.com/medium.png", {gfx::Size(128, 128)}),
  };
  observer_->MediaSessionImagesChanged(images);

  EXPECT_EQ(GURL("https://example.com/large.png"),
            observer_->metadata().artwork);
  EXPECT_EQ(1, change_count_);
}

TEST_F(PlaylistMediaSessionObserverTest, IgnoresNonHttpsArtwork) {
  base::flat_map<MediaSessionImageType, std::vector<media_session::MediaImage>>
      images;
  images[MediaSessionImageType::kArtwork] = {
      MakeImage("http://example.com/insecure.png", {gfx::Size(512, 512)}),
      MakeImage("https://example.com/secure.png", {gfx::Size(64, 64)}),
  };
  observer_->MediaSessionImagesChanged(images);

  EXPECT_EQ(GURL("https://example.com/secure.png"),
            observer_->metadata().artwork);
}

TEST_F(PlaylistMediaSessionObserverTest, IgnoresArtworkOfOtherTypes) {
  base::flat_map<MediaSessionImageType, std::vector<media_session::MediaImage>>
      images;
  images[MediaSessionImageType::kSourceIcon] = {
      MakeImage("https://example.com/favicon.png", {gfx::Size(16, 16)})};
  observer_->MediaSessionImagesChanged(images);

  EXPECT_TRUE(observer_->metadata().artwork.is_empty());
  EXPECT_EQ(0, change_count_);
}

TEST_F(PlaylistMediaSessionObserverTest, TakesDuration) {
  observer_->MediaSessionPositionChanged(media_session::MediaPosition(
      /*playback_rate=*/1.0, /*duration=*/base::Seconds(90),
      /*position=*/base::Seconds(0), /*end_of_media=*/false));

  EXPECT_EQ(base::Seconds(90), observer_->metadata().duration);
  EXPECT_EQ(1, change_count_);
}

TEST_F(PlaylistMediaSessionObserverTest, IgnoresLiveStreamDuration) {
  // Live streams report a max duration - there's nothing to save.
  observer_->MediaSessionPositionChanged(media_session::MediaPosition(
      /*playback_rate=*/1.0, /*duration=*/base::TimeDelta::Max(),
      /*position=*/base::Seconds(0), /*end_of_media=*/false));

  EXPECT_TRUE(observer_->metadata().duration.is_zero());
  EXPECT_EQ(0, change_count_);
}

TEST_F(PlaylistMediaSessionObserverTest, DoesNotNotifyOnUnchangedDuration) {
  const media_session::MediaPosition position(
      /*playback_rate=*/1.0, /*duration=*/base::Seconds(90),
      /*position=*/base::Seconds(0), /*end_of_media=*/false);
  observer_->MediaSessionPositionChanged(position);
  observer_->MediaSessionPositionChanged(position);

  // Position updates fire constantly during playback; only a duration change
  // is worth waking the detector for.
  EXPECT_EQ(1, change_count_);
}

TEST_F(PlaylistMediaSessionObserverTest, ResetClearsEverything) {
  media_session::MediaMetadata metadata;
  metadata.title = u"A Song";
  observer_->MediaSessionMetadataChanged(metadata);
  ASSERT_FALSE(observer_->metadata().empty());

  observer_->Reset();
  EXPECT_TRUE(observer_->metadata().empty());
}

}  // namespace playlist
