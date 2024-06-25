/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_media_file_downloader.h"

#include <algorithm>
#include <string_view>
#include <utility>

#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/task/thread_pool.h"
#include "base/uuid.h"
#include "brave/components/playlist/browser/mime_util.h"
#include "build/build_config.h"
#include "components/download/public/common/download_item_impl.h"
#include "components/download/public/common/download_task_runner.h"
#include "components/download/public/common/in_progress_download_manager.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/download_request_utils.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "url/gurl.h"

namespace playlist {

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTagForURLLoad() {
  return net::DefineNetworkTrafficAnnotation("playlist_service", R"(
      semantics {
        sender: "Brave Playlist Service"
        description:
          "Fetching media file for newly created playlist"
        trigger:
          "User-initiated for creating new playlist "
        data:
          "media file for playlist"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
      })");
}

}  // namespace

PlaylistMediaFileDownloader::PlaylistMediaFileDownloader(
    Delegate* delegate,
    content::BrowserContext* context)
    : delegate_(delegate),
      url_loader_factory_(
          context->content::BrowserContext::GetDefaultStoragePartition()
              ->GetURLLoaderFactoryForBrowserProcess()) {}

PlaylistMediaFileDownloader::~PlaylistMediaFileDownloader() {
  ResetDownloadStatus();

  if (download_manager_) {
    for (auto& download : download_manager_->TakeInProgressDownloads()) {
      DCHECK(download_item_observation_.IsObservingSource(download.get()));
      download_items_to_be_detached_.push_back(std::move(download));
    }

    while (!download_items_to_be_detached_.empty()) {
      DetachCachedFile(download_items_to_be_detached_.front().get());
    }

    download_manager_->ShutDown();
  }
}

void PlaylistMediaFileDownloader::NotifyFail(const std::string& id) {
  CHECK(!id.empty());
  delegate_->OnMediaFileGenerationFailed(id);
  if (current_item_ && current_item_->id == id) {
    // As this callback could be called from the async callbacks from
    // DownloadManager, the new item can be already in progress.
    ResetDownloadStatus();
  }
}

void PlaylistMediaFileDownloader::NotifySucceed(
    const std::string& id,
    const std::string& media_file_path,
    int64_t received_bytes) {
  DCHECK(!id.empty());
  DCHECK(!media_file_path.empty());
  delegate_->OnMediaFileReady(id, media_file_path, received_bytes);
  if (current_item_ && current_item_->id == id) {
    // As this callback could be called from the async callbacks from
    // DownloadManager, the new item can be already in progress.
    ResetDownloadStatus();
  }
}

void PlaylistMediaFileDownloader::ScheduleToCancelDownloadItem(
    const std::string& guid) {
  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE,
      base::BindOnce(&PlaylistMediaFileDownloader::CancelDownloadItem,
                     weak_factory_.GetWeakPtr(), guid));
}

void PlaylistMediaFileDownloader::CancelDownloadItem(const std::string& guid) {
  if (auto* download_item = download_manager_->GetDownloadByGuid(guid);
      download_item &&
      download_item->GetState() == download::DownloadItem::IN_PROGRESS) {
    download_item->Cancel(/*user_cancel=*/true);
  }
}

void PlaylistMediaFileDownloader::ScheduleToDetachCachedFile(
    download::DownloadItem* item) {
  for (auto& download : download_manager_->TakeInProgressDownloads()) {
    DCHECK(download_item_observation_.IsObservingSource(download.get()));
    download_items_to_be_detached_.push_back(std::move(download));
  }

  base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
      FROM_HERE, base::BindOnce(&PlaylistMediaFileDownloader::DetachCachedFile,
                                weak_factory_.GetWeakPtr(), item));
}

void PlaylistMediaFileDownloader::DetachCachedFile(
    download::DownloadItem* item) {
  // We allow only one item to be downloaded at a time.
  auto iter = base::ranges::find_if(
      download_items_to_be_detached_,
      [item](const auto& download) { return download.get() == item; });
  DCHECK(iter != download_items_to_be_detached_.end());

  // Before removing item from the vector, extend DownloadItems' lifetimes
  // so that it can be deleted after removing the file. i.e. The item should
  // be deleted after the file is released.
  auto will_be_detached = std::move(*iter);
  download_items_to_be_detached_.erase(iter);

  download_item_observation_.RemoveObservation(item);

  if (item->GetLastReason() ==
          download::DownloadInterruptReason::DOWNLOAD_INTERRUPT_REASON_NONE &&
      item->IsDone()) {
    will_be_detached->MarkAsComplete();
  } else {
    will_be_detached->Remove();
  }
}

void PlaylistMediaFileDownloader::DownloadMediaFileForPlaylistItem(
    const mojom::PlaylistItemPtr& item,
    const base::FilePath& destination) {
  DCHECK(!in_progress_);

  ResetDownloadStatus();

  if (item->cached) {
    DVLOG(2) << __func__ << ": media file is already downloaded";
    NotifySucceed(current_item_->id, current_item_->media_path.spec(), {});
    return;
  }

  in_progress_ = true;
  current_item_ = item->Clone();
  current_download_item_guid_ =
      base::Uuid::GenerateRandomV4().AsLowercaseString();
  if (!download_manager_) {
    // Creates our own manager. The arguments below are what's used by
    // AwBrowserContext::RetrieveInProgressDownloadManager().
    auto manager = std::make_unique<download::InProgressDownloadManager>(
        nullptr, base::FilePath(), nullptr,
        /* is_origin_secure_cb, */ base::BindRepeating([](const GURL& origin) {
          return true;
        }),
        base::BindRepeating(&content::DownloadRequestUtils::IsURLSafe),
        /*wake_lock_provider_binder*/ base::NullCallback());
    manager->set_url_loader_factory(url_loader_factory_);
    DCHECK(url_loader_factory_);
    download_manager_ = std::move(manager);
    download_manager_observation_.Observe(download_manager_.get());
  }

  DCHECK(download::GetIOTaskRunner()) << "This should be set by embedder";

  if (GURL media_url(current_item_->media_source); media_url.is_valid()) {
    destination_path_ = destination;
    DownloadMediaFile(media_url);
  } else {
    DVLOG(2) << __func__ << ": media file is empty";
    NotifyFail(current_item_->id);
  }
}

void PlaylistMediaFileDownloader::OnDownloadCreated(
    download::DownloadItem* item) {
  DVLOG(2) << __func__ << " " << item->GetGuid();

  if (current_download_item_guid_ != item->GetGuid()) {
    // This can happen when a user canceled it. But we should
    // observe the item anyway to handle the lifecycle of
    // download item.
    ScheduleToCancelDownloadItem(item->GetGuid());
    return;
  }

  DCHECK(!download_item_observation_.IsObservingSource(item));
  download_item_observation_.AddObservation(item);
}

void PlaylistMediaFileDownloader::OnDownloadUpdated(
    download::DownloadItem* item) {
  if (!current_item_) {
    // Download could be already finished. This seems to be late async callback.
    return;
  }

  if (item->GetLastReason() !=
      download::DownloadInterruptReason::DOWNLOAD_INTERRUPT_REASON_NONE) {
    LOG(ERROR) << __func__ << ": Download interrupted - reason: "
               << download::DownloadInterruptReasonToString(
                      item->GetLastReason());
    ScheduleToDetachCachedFile(item);
    OnMediaFileDownloaded(item->GetGuid(), {}, {}, {});
    return;
  }

  if (current_download_item_guid_ == item->GetGuid()) {
    base::TimeDelta time_remaining;
    item->TimeRemaining(&time_remaining);
    delegate_->OnMediaFileDownloadProgressed(
        current_item_->id, item->GetTotalBytes(), item->GetReceivedBytes(),
        item->PercentComplete(), time_remaining);
  }

  if (item->IsDone()) {
    ScheduleToDetachCachedFile(item);

    auto header = item->GetResponseHeaders();
    DCHECK(header);
    std::string mime_type;
    header->GetMimeType(&mime_type);
    DVLOG(2) << "mime_type from response header: " << mime_type;
    OnMediaFileDownloaded(item->GetGuid(), mime_type, destination_path_,
                          item->GetReceivedBytes());
    return;
  }
}

void PlaylistMediaFileDownloader::OnRenameFile(const base::FilePath& new_path,
                                               int64_t received_bytes,
                                               bool result) {
  if (result) {
    NotifySucceed(current_item_->id, new_path.AsUTF8Unsafe(), received_bytes);
  } else {
    DLOG(WARNING)
        << "Failed to rename file with extension, but shouldn't be fatal error";
    NotifySucceed(current_item_->id, destination_path_.AsUTF8Unsafe(),
                  received_bytes);
  }
}

void PlaylistMediaFileDownloader::OnDownloadRemoved(
    download::DownloadItem* item) {
  NOTREACHED_IN_MIGRATION()
      << "`item` was removed out of this class. This could cause flaky tests";
}

void PlaylistMediaFileDownloader::DownloadMediaFile(const GURL& url) {
  DVLOG(2) << __func__ << ": " << url.spec();

  auto params = std::make_unique<download::DownloadUrlParameters>(
      url, GetNetworkTrafficAnnotationTagForURLLoad());
  params->set_file_path(destination_path_);
  params->set_guid(current_download_item_guid_);

  params->set_transient(true);
  params->set_require_safety_checks(false);
  DCHECK(download_manager_->CanDownload(params.get()));
  download_manager_->DownloadUrl(std::move(params));
}

void PlaylistMediaFileDownloader::OnMediaFileDownloaded(
    const std::string& download_item_guid,
    const std::string& mime_type,
    base::FilePath path,
    int64_t received_bytes) {
  DVLOG(2) << __func__ << ": downloaded media file at " << path;
  if (download_item_guid != current_download_item_guid_) {
    return;
  }

  if (path.empty()) {
    // This fail is handled during the generation.
    // See |has_skipped_source_files| in DoGenerateSingleMediaFile().
    // |has_skipped_source_files| will be set to true.
    DVLOG(1) << __func__ << ": failed to download media file: "
             << current_item_->media_source;
    NotifyFail(current_item_->id);
    return;
  }

  DCHECK_EQ(path, destination_path_);
  if (path.Extension().empty() && !mime_type.empty()) {
    // Try to infer proper extension from mime_type
    // TODO(sko) It's unlikely but there could be parameter or suffix delimited
    // with "+" or ";" in |mime_type|.
    if (auto extension = mime_util::GetFileExtensionForMimetype(mime_type);
        extension.has_value()) {
      auto new_path = path.AddExtension(*extension);
      delegate_->GetTaskRunner()->PostTaskAndReplyWithResult(
          FROM_HERE, base::BindOnce(&base::Move, path, new_path),
          base::BindOnce(&PlaylistMediaFileDownloader::OnRenameFile,
                         weak_factory_.GetWeakPtr(), new_path, received_bytes));
      return;
    }

    DLOG(WARNING) << "We can't find out what extension the file should have.";
  }

  NotifySucceed(current_item_->id, destination_path_.AsUTF8Unsafe(),
                received_bytes);
}

void PlaylistMediaFileDownloader::RequestCancelCurrentPlaylistGeneration() {
  ResetDownloadStatus();
}

base::SequencedTaskRunner* PlaylistMediaFileDownloader::task_runner() {
  if (!task_runner_) {
    task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return task_runner_.get();
}

void PlaylistMediaFileDownloader::ResetDownloadStatus() {
  in_progress_ = false;
  current_item_.reset();
  destination_path_.clear();
  if (!current_download_item_guid_.empty()) {
    ScheduleToCancelDownloadItem(current_download_item_guid_);
    current_download_item_guid_.clear();
  }
}

}  // namespace playlist
