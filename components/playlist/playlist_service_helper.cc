/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_service_helper.h"

#include <vector>

#include "base/values.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_types.h"

namespace playlist {

namespace {

base::Value GetValueFromMediaFile(const MediaFileInfo& info) {
  base::Value media_file(base::Value::Type::DICTIONARY);
  media_file.SetStringKey(kPlaylistMediaFileUrlKey, info.media_file_url);
  media_file.SetStringKey(kPlaylistMediaFileTitleKey, info.media_file_title);
  return media_file;
}

base::Value GetValueFromCreateParams(const CreatePlaylistParams& params) {
  base::Value create_params_value(base::Value::Type::DICTIONARY);
  create_params_value.SetStringKey(kPlaylistPlaylistThumbnailUrlKey,
                                   params.playlist_thumbnail_url);
  create_params_value.SetStringKey(kPlaylistPlaylistNameKey,
                                   params.playlist_name);
  create_params_value.SetKey(kPlaylistVideoMediaFilesKey,
                             GetValueFromMediaFiles(params.video_media_files));
  create_params_value.SetKey(kPlaylistAudioMediaFilesKey,
                             GetValueFromMediaFiles(params.audio_media_files));
  return create_params_value;
}

base::Value GetTitleValueFromCreateParams(const CreatePlaylistParams& params) {
  base::Value titles_value(base::Value::Type::LIST);
  for (const MediaFileInfo& info : params.video_media_files)
    titles_value.Append(base::Value(info.media_file_title));
  return titles_value;
}

}  // namespace

base::Value GetValueFromMediaFiles(
    const std::vector<MediaFileInfo>& media_files) {
  base::Value media_files_value(base::Value::Type::LIST);
  for (const MediaFileInfo& info : media_files)
    media_files_value.Append(GetValueFromMediaFile(info));
  return media_files_value;
}

base::Value GetValueFromPlaylistInfo(const PlaylistInfo& info) {
  base::Value playlist_value(base::Value::Type::DICTIONARY);
  playlist_value.SetStringKey(kPlaylistIDKey, info.id);
  playlist_value.SetStringKey(kPlaylistPlaylistNameKey, info.playlist_name);
  playlist_value.SetStringKey(kPlaylistThumbnailPathKey, info.thumbnail_path);
  playlist_value.SetStringKey(kPlaylistVideoMediaFilePathKey,
                              info.video_media_file_path);
  playlist_value.SetStringKey(kPlaylistAudioMediaFilePathKey,
                              info.audio_media_file_path);
  playlist_value.SetBoolKey(kPlaylistReadyKey, info.ready);
  playlist_value.SetKey(kPlaylistTitlesKey,
                        GetTitleValueFromCreateParams(info.create_params));
  playlist_value.SetKey(kPlaylistCreateParamsKey,
                        GetValueFromCreateParams(info.create_params));
  return playlist_value;
}

}  // namespace playlist
