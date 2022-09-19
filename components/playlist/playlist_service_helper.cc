/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_service_helper.h"

#include <vector>

#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_types.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_user_data.h"

namespace playlist {

namespace {

class PlaylistBackgroundWebContentsTag
    : public content::WebContentsUserData<PlaylistBackgroundWebContentsTag> {
 public:
  PlaylistBackgroundWebContentsTag(const PlaylistBackgroundWebContentsTag&) =
      delete;
  PlaylistBackgroundWebContentsTag& operator=(
      const PlaylistBackgroundWebContentsTag&) = delete;

  ~PlaylistBackgroundWebContentsTag() override = default;

 private:
  friend class content::WebContentsUserData<PlaylistBackgroundWebContentsTag>;

  explicit PlaylistBackgroundWebContentsTag(content::WebContents* web_contents)
      : content::WebContentsUserData<PlaylistBackgroundWebContentsTag>(
            *web_contents) {}

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

WEB_CONTENTS_USER_DATA_KEY_IMPL(PlaylistBackgroundWebContentsTag);

}  // namespace

base::Value::Dict GetValueFromPlaylistItemInfo(const PlaylistItemInfo& info) {
  base::Value::Dict playlist_value;
  playlist_value.Set(kPlaylistItemIDKey, info.id);
  playlist_value.Set(kPlaylistItemTitleKey, info.title);
  playlist_value.Set(kPlaylistItemPageSrcKey, info.page_src);
  playlist_value.Set(kPlaylistItemMediaSrcKey, info.media_src);
  playlist_value.Set(kPlaylistItemThumbnailSrcKey, info.thumbnail_src);
  playlist_value.Set(kPlaylistItemThumbnailPathKey, info.thumbnail_path);
  playlist_value.Set(kPlaylistItemMediaFilePathKey, info.media_file_path);
  playlist_value.Set(kPlaylistItemMediaFileCachedKey, info.media_file_cached);
  return playlist_value;
}

void MarkAsBackgroundWebContents(content::WebContents* web_contents) {
  PlaylistBackgroundWebContentsTag::CreateForWebContents(web_contents);
}

bool IsBackgroundWebContents(content::WebContents* web_contents) {
  return !!PlaylistBackgroundWebContentsTag::FromWebContents(web_contents);
}

}  // namespace playlist
