/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_media_file_downloader.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/files/file.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/task_runner_util.h"
#include "base/task/thread_pool.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_types.h"
#include "build/build_config.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/resource_request.h"
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
    content::BrowserContext* context,
    base::FilePath::StringType media_file_name)
    : delegate_(delegate),
      url_loader_factory_(
          context->content::BrowserContext::GetDefaultStoragePartition()
              ->GetURLLoaderFactoryForBrowserProcess()),
      request_helper_(std::make_unique<api_request_helper::APIRequestHelper>(
          GetNetworkTrafficAnnotationTagForURLLoad(),
          url_loader_factory_)),
      media_file_name_(media_file_name) {}

PlaylistMediaFileDownloader::~PlaylistMediaFileDownloader() = default;

void PlaylistMediaFileDownloader::NotifyFail(const std::string& id) {
  CHECK(!id.empty());
  delegate_->OnMediaFileGenerationFailed(id);
  ResetDownloadStatus();
}

void PlaylistMediaFileDownloader::NotifySucceed(
    const std::string& id,
    const std::string& media_file_path) {
  DCHECK(!id.empty());
  DCHECK(!media_file_path.empty());
  delegate_->OnMediaFileReady(id, media_file_path);
  ResetDownloadStatus();
}

void PlaylistMediaFileDownloader::DownloadMediaFileForPlaylistItem(
    const PlaylistItemInfo& item,
    const base::FilePath& base_dir) {
  DCHECK(!in_progress_);

  ResetDownloadStatus();

  in_progress_ = true;
  current_item_ = std::make_unique<PlaylistItemInfo>(item);

  if (item.media_file_cached) {
    VLOG(2) << __func__ << ": media file is already downloaded";
    NotifySucceed(current_item_->id, current_item_->media_file_path);
    return;
  }

  if (GURL media_url(current_item_->media_src); media_url.is_valid()) {
    playlist_dir_path_ = base_dir.AppendASCII(current_item_->id);
    DownloadMediaFile(media_url, 0);
  } else {
    VLOG(2) << __func__ << ": media file is empty";
    NotifyFail(current_item_->id);
  }
}

void PlaylistMediaFileDownloader::DownloadMediaFile(const GURL& url,
                                                    int index) {
  VLOG(2) << __func__ << ": " << url.spec() << " at: " << index;

  const base::FilePath file_path = playlist_dir_path_.Append(media_file_name_);
  request_helper_->Download(
      url, {}, {}, true, file_path,
      base::BindOnce(&PlaylistMediaFileDownloader::OnMediaFileDownloaded,
                     base::Unretained(this), index));
}

void PlaylistMediaFileDownloader::OnMediaFileDownloaded(
    int index,
    base::FilePath path,
    base::flat_map<std::string, std::string> response_headers) {
  VLOG(2) << __func__ << ": downloaded media file at " << path;

  DCHECK(current_item_);

  if (path.empty()) {
    // This fail is handled during the generation.
    // See |has_skipped_source_files| in DoGenerateSingleMediaFile().
    // |has_skipped_source_files| will be set to true.
    VLOG(1) << __func__ << ": failed to download media file at " << index;
    NotifyFail(current_item_->id);
    return;
  }

  NotifySucceed(current_item_->id, path.AsUTF8Unsafe());
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
  request_helper_ = std::make_unique<api_request_helper::APIRequestHelper>(
      GetNetworkTrafficAnnotationTagForURLLoad(), url_loader_factory_);
  playlist_dir_path_.clear();
}

}  // namespace playlist
