/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_media_file_download_manager.h"

#include <utility>

#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/values.h"
#include "brave/components/playlist/browser/playlist_constants.h"

namespace playlist {

// DownloadJob -----------------------------------------------------------------

PlaylistMediaFileDownloadManager::DownloadJob::DownloadJob() = default;
PlaylistMediaFileDownloadManager::DownloadJob::DownloadJob(
    PlaylistMediaFileDownloadManager::DownloadJob&&) noexcept = default;
PlaylistMediaFileDownloadManager::DownloadJob&
PlaylistMediaFileDownloadManager::DownloadJob::operator=(
    PlaylistMediaFileDownloadManager::DownloadJob&&) noexcept = default;
PlaylistMediaFileDownloadManager::DownloadJob::~DownloadJob() = default;

// PlaylistMediaFileDownloadManager --------------------------------------------

PlaylistMediaFileDownloadManager::PlaylistMediaFileDownloadManager(
    content::BrowserContext* context,
    Delegate* delegate)
    : delegate_(delegate) {
  DCHECK(delegate_) << "We don't consider where |delegate| is null";
  media_file_downloader_ =
      std::make_unique<PlaylistMediaFileDownloader>(this, context);
}

PlaylistMediaFileDownloadManager::~PlaylistMediaFileDownloadManager() = default;

void PlaylistMediaFileDownloadManager::DownloadMediaFile(
    std::unique_ptr<DownloadJob> request) {
  DCHECK(request);
  DCHECK(request->item);

  pending_media_file_creation_jobs_.push(std::move(request));

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

  current_job_ = PopNextJob();
  if (!current_job_) {
    return;
  }

  DCHECK(current_job_->item);

  if (!pause_download_for_testing_) {
    VLOG(2) << __func__ << ": " << current_job_->item->name;
    media_file_downloader_->DownloadMediaFileForPlaylistItem(
        current_job_->item,
        delegate_->GetMediaPathForPlaylistItemItem(current_job_->item->id));
  }
}

std::unique_ptr<PlaylistMediaFileDownloadManager::DownloadJob>
PlaylistMediaFileDownloadManager::PopNextJob() {
  while (!pending_media_file_creation_jobs_.empty()) {
    auto request = std::move(pending_media_file_creation_jobs_.front());
    DCHECK(request);
    DCHECK(request->item);

    pending_media_file_creation_jobs_.pop();

    if (delegate_->IsValidPlaylistItem(request->item->id)) {
      return request;
    }
  }

  return {};
}

std::string
PlaylistMediaFileDownloadManager::GetCurrentDownloadingPlaylistItemID() const {
  if (IsCurrentDownloadingInProgress())
    return media_file_downloader_->current_playlist_id();

  return {};
}

void PlaylistMediaFileDownloadManager::CancelCurrentDownloadingPlaylistItem() {
  media_file_downloader_->RequestCancelCurrentPlaylistGeneration();
  current_job_.reset();
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
  if (!current_job_ || !current_job_->item) {
    return;
  }

  if (current_job_->item->id != id) {
    return;
  }

  if (current_job_->on_progress_callback) {
    current_job_->on_progress_callback.Run(current_job_->item, total_bytes,
                                           received_bytes, percent_complete,
                                           time_remaining);
  }
}

void PlaylistMediaFileDownloadManager::OnMediaFileReady(
    const std::string& id,
    const std::string& media_file_path) {
  VLOG(2) << __func__ << ": " << id << " is ready.";
  if (!current_job_ || !current_job_->item) {
    return;
  }

  if (current_job_->item->id != id) {
    return;
  }

  if (current_job_->on_finish_callback) {
    std::move(current_job_->on_finish_callback)
        .Run(std::move(current_job_->item), media_file_path);
  }
  current_job_.reset();

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&PlaylistMediaFileDownloadManager::TryStartingDownloadTask,
                     weak_factory_.GetWeakPtr()));
}

void PlaylistMediaFileDownloadManager::OnMediaFileGenerationFailed(
    const std::string& id) {
  VLOG(2) << __func__ << ": " << id;
  if (!current_job_ || !current_job_->item) {
    return;
  }

  if (current_job_->item && current_job_->item->id != id) {
    return;
  }

  if (current_job_->on_finish_callback) {
    std::move(current_job_->on_finish_callback)
        .Run(std::move(current_job_->item), {});
  }
  current_job_.reset();
  CancelCurrentDownloadingPlaylistItem();

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&PlaylistMediaFileDownloadManager::TryStartingDownloadTask,
                     weak_factory_.GetWeakPtr()));
}

}  // namespace playlist
