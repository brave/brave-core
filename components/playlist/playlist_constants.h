/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_CONSTANTS_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_CONSTANTS_H_

namespace playlist {

constexpr char kPlaylistMediaFileUrlKey[] = "mediaFileUrl";
constexpr char kPlaylistMediaFileTitleKey[] = "mediaFileTitle";

constexpr char kPlaylistPlaylistNameKey[] = "playlistName";
constexpr char kPlaylistPlaylistThumbnailUrlKey[] = "playlistThumbnailUrl";
constexpr char kPlaylistVideoMediaFilesKey[] = "videoMediaFiles";
constexpr char kPlaylistAudioMediaFilesKey[] = "audioMediaFiles";

constexpr char kPlaylistIDKey[] = "id";
constexpr char kPlaylistThumbnailPathKey[] = "thumbnailPath";
constexpr char kPlaylistVideoMediaFilePathKey[] = "videoMediaFilePath";
constexpr char kPlaylistAudioMediaFilePathKey[] = "audioMediaFilePath";
constexpr char kPlaylistReadyKey[] = "ready";
constexpr char kPlaylistTitlesKey[] = "titles";
constexpr char kPlaylistCreateParamsKey[] = "createParams";

constexpr char kPlaylistCreateParamsThumbnailUrlPathKey[] =
    "createParams.playlistThumbnailUrl";
constexpr char kPlaylistCreateParamsVideoMediaFilesPathKey[] =
    "createParams.videoMediaFiles";
constexpr char kPlaylistCreateParamsAudioMediaFilesPathKey[] =
    "createParams.audioMediaFiles";

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_CONSTANTS_H_
