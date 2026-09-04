/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/content/browser/playlist_media_file_download_manager.h"

#include <utility>

#include "base/check.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/task/sequenced_task_runner.h"
#include "base/task/thread_pool.h"
#include "brave/components/playlist/content/browser/media_stream_classifier.h"
#include "brave/components/playlist/content/browser/playlist_constants.h"
#include "brave/components/playlist/core/common/features.h"

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
    : delegate_(delegate), context_(context) {
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
  if (!IsCurrentDownloadingInProgress()) {
    TryStartingDownloadTask();
  }
}

void PlaylistMediaFileDownloadManager::CancelDownloadRequest(
    const std::string& id) {
  VLOG(2) << __func__ << " " << id;

  // Cancel if currently downloading item is id.
  // Otherwise, PopNextJob() will drop canceled one.
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
  if (IsCurrentDownloadingInProgress()) {
    return;
  }

  if (pending_media_file_creation_jobs_.empty()) {
    return;
  }

  current_job_ = PopNextJob();
  if (!current_job_) {
    return;
  }

  DCHECK(current_job_->item);

  if (pause_download_for_testing_) {
    return;
  }

  VLOG(2) << __func__ << ": " << current_job_->item->name;

  if (base::FeatureList::IsEnabled(features::kPlaylistServiceV2) &&
      !IsYoutubeLegacyPlaylistSite(current_job_->item->page_source) &&
      IsStreamManifestUrl(current_job_->item->media_source)) {
    StartHlsDownloadTask();
    return;
  }

  media_file_downloader_->DownloadMediaFileForPlaylistItem(
      current_job_->item,
      delegate_->GetMediaPathForPlaylistItemItem(current_job_->item->id));
}

void PlaylistMediaFileDownloadManager::StartHlsDownloadTask() {
  const std::string id = current_job_->item->id;
  const GURL manifest_url = current_job_->item->media_source;
  const base::FilePath directory =
      delegate_->GetMediaPathForPlaylistItemItem(id).DirName().AppendASCII(
          "hls");

  waiting_for_hls_directory_ = true;
  base::ThreadPool::PostTaskAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(
          [](const base::FilePath& directory) {
            return base::CreateDirectory(directory);
          },
          directory),
      base::BindOnce(&PlaylistMediaFileDownloadManager::OnHlsDirectoryCreated,
                     weak_factory_.GetWeakPtr(), id, manifest_url, directory));
}

void PlaylistMediaFileDownloadManager::OnHlsDirectoryCreated(
    const std::string& id,
    const GURL& manifest_url,
    const base::FilePath& directory,
    bool created) {
  waiting_for_hls_directory_ = false;

  // Compare against the job directly rather than asking which download is in
  // progress: clearing the flag above already made that answer "none".
  if (!current_job_ || current_job_->item->id != id) {
    // Cancelled, or superseded, while we were touching the disk.
    return;
  }

  if (!created) {
    OnMediaFileGenerationFailed(id);
    return;
  }

  hls_downloader_ = std::make_unique<PlaylistStreamDownloader>(context_);
  hls_downloader_->Start(
      manifest_url, directory,
      base::BindRepeating(
          [](base::WeakPtr<PlaylistMediaFileDownloadManager> self,
             const std::string& id, int64_t received_bytes,
             int percent_complete) {
            if (self) {
              self->OnMediaFileDownloadProgressed(
                  id, /*total_bytes=*/0, received_bytes, percent_complete,
                  base::TimeDelta());
            }
          },
          weak_factory_.GetWeakPtr(), id),
      base::BindOnce(&PlaylistMediaFileDownloadManager::OnHlsDownloaded,
                     weak_factory_.GetWeakPtr(), id, directory));
}

void PlaylistMediaFileDownloadManager::OnHlsDownloaded(
    const std::string& id,
    const base::FilePath& directory,
    base::expected<PlaylistStreamDownloader::Result,
                   PlaylistStreamDownloader::Error> result) {
  hls_downloader_.reset();

  // Same reasoning as above: the downloader is already gone by this point.
  if (!current_job_ || current_job_->item->id != id) {
    return;
  }

  if (!result.has_value()) {
    VLOG(1) << __func__ << ": " << id << ": "
            << PlaylistStreamDownloader::ErrorToString(result.error());
    OnMediaFileGenerationFailed(id);
    return;
  }

  OnMediaFileReady(
      id, directory.AppendASCII(result->manifest_file_name).AsUTF8Unsafe(),
      result->received_bytes);
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
  // A stream download never reaches `media_file_downloader_`, so its id has to
  // come from the job itself.
  if (hls_downloader_ || waiting_for_hls_directory_) {
    return current_job_ ? current_job_->item->id : std::string();
  }

  if (media_file_downloader_->in_progress()) {
    return media_file_downloader_->current_item_id();
  }

  return {};
}

void PlaylistMediaFileDownloadManager::CancelCurrentDownloadingPlaylistItem() {
  if (current_job_ && current_job_->on_finish_callback) {
    std::move(current_job_->on_finish_callback)
        .Run(current_job_->item->Clone(),
             base::unexpected(DownloadFailureReason::kCanceled));
  }

  media_file_downloader_->RequestCancelCurrentPlaylistGeneration();
  // Destroying the stream downloader abandons everything it has in flight.
  hls_downloader_.reset();
  waiting_for_hls_directory_ = false;
  current_job_.reset();
}

bool PlaylistMediaFileDownloadManager::IsCurrentDownloadingInProgress() const {
  // A stream download doesn't go through `media_file_downloader_`, so ask both
  // - otherwise a second job would start on top of one in flight.
  return media_file_downloader_->in_progress() || hls_downloader_ ||
         waiting_for_hls_directory_;
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
    const std::string& media_file_path,
    int64_t received_bytes) {
  VLOG(2) << __func__ << ": " << id << " is ready.";
  if (!current_job_ || !current_job_->item) {
    return;
  }

  if (current_job_->item->id != id) {
    return;
  }

  if (current_job_->on_finish_callback) {
    std::move(current_job_->on_finish_callback)
        .Run(std::move(current_job_->item),
             DownloadResult(media_file_path, received_bytes));
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
        .Run(std::move(current_job_->item),
             base::unexpected(DownloadFailureReason::kFailed));
  }
  current_job_.reset();
  CancelCurrentDownloadingPlaylistItem();

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&PlaylistMediaFileDownloadManager::TryStartingDownloadTask,
                     weak_factory_.GetWeakPtr()));
}

base::SequencedTaskRunner* PlaylistMediaFileDownloadManager::GetTaskRunner() {
  return delegate_->GetTaskRunner();
}

}  // namespace playlist
