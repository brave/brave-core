/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_media_file_download_manager.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/threading/sequenced_task_runner_handle.h"
#include "base/values.h"
#include "brave/components/playlist/playlist_constants.h"

namespace playlist {

PlaylistMediaFileDownloadManager::PlaylistMediaFileDownloadManager(
    content::BrowserContext* context,
    Delegate* delegate,
    const base::FilePath& base_dir)
    : base_dir_(base_dir), delegate_(delegate) {
  // TODO(pilgrim) dynamically set file extensions based on format.
  media_file_downloader_ = std::make_unique<PlaylistMediaFileDownloader>(
      this, context, kMediaFileName);
}

PlaylistMediaFileDownloadManager::~PlaylistMediaFileDownloadManager() = default;

void PlaylistMediaFileDownloadManager::DownloadMediaFile(
    const PlaylistItemInfo& playlist_item) {
  pending_media_file_creation_jobs_.push(playlist_item);

  // If either media file controller is generating a playlist media file,
  // delay the next playlist generation. It will be triggered when the current
  // one is finished.
  if (!IsCurrentDownloadingInProgress())
    TryStartingDownloadTask();
}

void PlaylistMediaFileDownloadManager::CancelDownloadRequest(
    const std::string& id) {
  VLOG(2) << __func__ << " " << id;

  // Cancel if currently downloading item is id.
  // Otherwise, GetNextPlaylistItemTarget() will drop canceled one.
  if (GetCurrentDownloadingPlaylistItemID() == id) {
    CancelCurrentDownloadingPlaylistItem();
    TryStartingDownloadTask();
    return;
  }
}

void PlaylistMediaFileDownloadManager::CancelAllDownloadRequests() {
  CancelCurrentDownloadingPlaylistItem();
  pending_media_file_creation_jobs_ = {};
}

void PlaylistMediaFileDownloadManager::TryStartingDownloadTask() {
  if (IsCurrentDownloadingInProgress())
    return;

  if (pending_media_file_creation_jobs_.empty())
    return;

  current_item_ = GetNextPlaylistItemTarget();
  if (!current_item_)
    return;

  VLOG(2) << __func__ << ": " << current_item_->title;

  media_file_downloader_->DownloadMediaFileForPlaylistItem(*current_item_,
                                                           base_dir_);
}

std::unique_ptr<PlaylistItemInfo>
PlaylistMediaFileDownloadManager::GetNextPlaylistItemTarget() {
  while (!pending_media_file_creation_jobs_.empty()) {
    auto playlist_item(std::move(pending_media_file_creation_jobs_.front()));
    pending_media_file_creation_jobs_.pop();

    if (delegate_->IsValidPlaylistItem(playlist_item.id))
      return std::make_unique<PlaylistItemInfo>(std::move(playlist_item));
  }

  return nullptr;
}

std::string
PlaylistMediaFileDownloadManager::GetCurrentDownloadingPlaylistItemID() const {
  if (IsCurrentDownloadingInProgress())
    return media_file_downloader_->current_playlist_id();

  return {};
}

void PlaylistMediaFileDownloadManager::CancelCurrentDownloadingPlaylistItem() {
  media_file_downloader_->RequestCancelCurrentPlaylistGeneration();
}

bool PlaylistMediaFileDownloadManager::IsCurrentDownloadingInProgress() const {
  return media_file_downloader_->in_progress();
}

void PlaylistMediaFileDownloadManager::OnMediaFileDownloadProgressed(
    const std::string& id,
    int64_t total_bytes,
    int64_t received_bytes,
    int percent_complete,
    base::TimeDelta time_remaining) {
  delegate_->OnMediaFileDownloadProgressed(id, total_bytes, received_bytes,
                                           percent_complete, time_remaining);
}

void PlaylistMediaFileDownloadManager::OnMediaFileReady(
    const std::string& id,
    const std::string& media_file_path) {
  VLOG(2) << __func__ << ": " << id << " is ready.";

  delegate_->OnMediaFileReady(id, media_file_path);

  current_item_.reset();

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&PlaylistMediaFileDownloadManager::TryStartingDownloadTask,
                     weak_factory_.GetWeakPtr()));
}

void PlaylistMediaFileDownloadManager::OnMediaFileGenerationFailed(
    const std::string& id) {
  VLOG(2) << __func__ << ": " << id;

  delegate_->OnMediaFileGenerationFailed(id);

  CancelCurrentDownloadingPlaylistItem();

  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE,
      base::BindOnce(&PlaylistMediaFileDownloadManager::TryStartingDownloadTask,
                     weak_factory_.GetWeakPtr()));
}

}  // namespace playlist
