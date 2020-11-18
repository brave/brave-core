/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_media_file_download_manager.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/playlist/playlist_constants.h"

namespace playlist {

PlaylistMediaFileDownloadManager::PlaylistMediaFileDownloadManager(
    content::BrowserContext* context,
    Delegate* delegate,
    const base::FilePath& base_dir)
    : base_dir_(base_dir), delegate_(delegate) {
  // TODO(pilgrim) dynamically set file extensions based on format
  // (may require changes to youtubedown parser)
  video_media_file_downloader_.reset(new PlaylistMediaFileDownloader(
      this, context, FILE_PATH_LITERAL("video_source_files"),
      FILE_PATH_LITERAL("video_file.mp4"), kPlaylistVideoMediaFilePathKey,
      kPlaylistCreateParamsVideoMediaFilesPathKey));
  audio_media_file_downloader_.reset(new PlaylistMediaFileDownloader(
      this, context, FILE_PATH_LITERAL("audio_source_files"),
      FILE_PATH_LITERAL("audio_file.m4a"), kPlaylistAudioMediaFilePathKey,
      kPlaylistCreateParamsAudioMediaFilesPathKey));
}

PlaylistMediaFileDownloadManager::~PlaylistMediaFileDownloadManager() = default;

void PlaylistMediaFileDownloadManager::GenerateMediaFileForPlaylistItem(
    const base::Value& playlist_item) {
  pending_media_file_creation_jobs_.push(playlist_item.Clone());

  // If either media file controller is generating a playlist media file,
  // delay the next playlist generation. It will be triggered when the current
  // one is finished.
  if (!IsCurrentDownloadingInProgress())
    GenerateMediaFiles();
}

void PlaylistMediaFileDownloadManager::CancelDownloadRequest(
    const std::string& id) {
  // Cancel if currently downloading item is id.
  // Otherwise, GetNextPlaylistItemTarget() will drop canceled one.
  if (GetCurrentDownloadingPlaylistItemID() == id) {
    CancelCurrentDownloadingPlaylistItem();
    GenerateMediaFiles();
    return;
  }
}

void PlaylistMediaFileDownloadManager::ResetCurrentPlaylistItemInfo() {
  current_playlist_item_id_.clear();
  current_playlist_item_audio_file_path_.clear();
  current_playlist_item_video_file_path_.clear();
}

void PlaylistMediaFileDownloadManager::CancelAllDownloadRequests() {
  CancelCurrentDownloadingPlaylistItem();
  pending_media_file_creation_jobs_ = base::queue<base::Value>();
}

void PlaylistMediaFileDownloadManager::GenerateMediaFiles() {
  DCHECK(!IsCurrentDownloadingInProgress());
  ResetCurrentPlaylistItemInfo();

  if (pending_media_file_creation_jobs_.empty())
    return;

  base::Value video_value = GetNextPlaylistItemTarget();
  if (video_value.is_none())
    return;

  base::Value audio_value = video_value.Clone();
  VLOG(2) << __func__ << ": "
          << *video_value.FindStringKey(kPlaylistPlaylistNameKey);

  video_media_file_downloader_->GenerateSingleMediaFile(std::move(video_value),
                                                        base_dir_);
  audio_media_file_downloader_->GenerateSingleMediaFile(std::move(audio_value),
                                                        base_dir_);
}

base::Value PlaylistMediaFileDownloadManager::GetNextPlaylistItemTarget() {
  while (!pending_media_file_creation_jobs_.empty()) {
    base::Value playlist_item(
        std::move(pending_media_file_creation_jobs_.front()));
    pending_media_file_creation_jobs_.pop();
    const std::string playlist_id =
        *playlist_item.FindStringKey(kPlaylistIDKey);
    if (delegate_->IsValidPlaylistItem(playlist_id)) {
      current_playlist_item_id_ = playlist_id;
      return playlist_item;
    }
  }

  return {};
}

std::string
PlaylistMediaFileDownloadManager::GetCurrentDownloadingPlaylistItemID() const {
  return video_media_file_downloader_->current_playlist_id();
}

void PlaylistMediaFileDownloadManager::CancelCurrentDownloadingPlaylistItem() {
  video_media_file_downloader_->RequestCancelCurrentPlaylistGeneration();
  audio_media_file_downloader_->RequestCancelCurrentPlaylistGeneration();
}

bool PlaylistMediaFileDownloadManager::IsCurrentDownloadingInProgress() const {
  return video_media_file_downloader_->in_progress() ||
         audio_media_file_downloader_->in_progress();
}

void PlaylistMediaFileDownloadManager::OnMediaFileReady(
    const std::string& id,
    const std::string& media_file_path_key,
    const std::string& media_file_path) {
  if (media_file_path_key == kPlaylistAudioMediaFilePathKey) {
    current_playlist_item_audio_file_path_ = media_file_path;
  } else {
    current_playlist_item_video_file_path_ = media_file_path;
  }
  if (IsCurrentDownloadingInProgress())
    return;

  VLOG(2) << __func__ << ": " << id << " is ready.";

  delegate_->OnMediaFileReady(id, current_playlist_item_audio_file_path_,
                              current_playlist_item_video_file_path_);

  ResetCurrentPlaylistItemInfo();
  GenerateMediaFiles();
}

void PlaylistMediaFileDownloadManager::OnMediaFileGenerationFailed(
    const std::string& id) {
  VLOG(2) << __func__ << ": " << id;

  CancelCurrentDownloadingPlaylistItem();
  delegate_->OnMediaFileGenerationFailed(id);

  GenerateMediaFiles();
}

}  // namespace playlist
