/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_CONSTANTS_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_CONSTANTS_H_

namespace playlist {

constexpr char kPlaylistIDKey[] = "id";
constexpr char kPlaylistNameKey[] = "name";
constexpr char kPlaylistItemsKey[] = "items";

constexpr char kPlaylistItemIDKey[] = "id";
constexpr char kPlaylistItemPageSrcKey[] = "pageSrc";
constexpr char kPlaylistItemMediaSrcKey[] = "mediaSrc";
constexpr char kPlaylistItemThumbnailSrcKey[] = "thumbnailSrc";
constexpr char kPlaylistItemThumbnailPathKey[] = "thumbnailPath";
constexpr char kPlaylistItemMediaFilePathKey[] = "mediaFilePath";
constexpr char kPlaylistItemMediaFileCachedKey[] = "mediaCached";
constexpr char kPlaylistItemTitleKey[] = "title";
constexpr char kPlaylistItemAuthorKey[] = "author";
constexpr char kPlaylistItemDurationKey[] = "duration";

constexpr char kDefaultPlaylistID[] = "default";

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_CONSTANTS_H_
