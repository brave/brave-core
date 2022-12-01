/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_thumbnail_downloader.h"

#include <utility>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

#if BUILDFLAG(IS_ANDROID)
#include "base/files/file_util.h"
#include "base/task/thread_pool.h"
#endif

namespace {

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTagForURLLoad() {
  return net::DefineNetworkTrafficAnnotation("playlist_thumbnail_downloader",
                                             R"(
      semantics {
        sender: "Brave playlist thumbnail downloader"
        description:
          "Fetching thumbnail image for newly created playlist item"
        trigger:
          "User-initiated for creating new playlist item"
        data:
          "Thumbnail for playlist item"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
      })");
}

}  // namespace

PlaylistThumbnailDownloader::PlaylistThumbnailDownloader(
    content::BrowserContext* context,
    Delegate* delegate)
    : url_loader_factory_(context->GetDefaultStoragePartition()
                              ->GetURLLoaderFactoryForBrowserProcess()),
      request_helper_(std::make_unique<api_request_helper::APIRequestHelper>(
          GetNetworkTrafficAnnotationTagForURLLoad(),
          url_loader_factory_)),
      delegate_(delegate) {}

PlaylistThumbnailDownloader::~PlaylistThumbnailDownloader() = default;

void PlaylistThumbnailDownloader::DownloadThumbnail(
    const std::string& id,
    const GURL& thumbnail_url,
    const base::FilePath& target_thumbnail_path) {
  VLOG(2) << __func__ << " " << id << " : " << thumbnail_url.spec();
  CancelDownloadRequest(id);

  auto ticket = request_helper_->Download(
      thumbnail_url, {}, {}, true, target_thumbnail_path,
      base::BindOnce(&PlaylistThumbnailDownloader::OnThumbnailDownloaded,
                     base::Unretained(this), id));
  ticket_map_[id] = ticket;
}

void PlaylistThumbnailDownloader::CancelDownloadRequest(const std::string& id) {
  VLOG(2) << __func__ << " " << id;
  if (!ticket_map_.count(id))
    return;

  request_helper_->Cancel(ticket_map_[id]);
  ticket_map_.erase(id);
}

void PlaylistThumbnailDownloader::CancelAllDownloadRequests() {
  VLOG(2) << __func__;
  request_helper_ = std::make_unique<api_request_helper::APIRequestHelper>(
      GetNetworkTrafficAnnotationTagForURLLoad(), url_loader_factory_);
  ticket_map_.clear();
}

void PlaylistThumbnailDownloader::OnThumbnailDownloaded(
    const std::string& id,
    base::FilePath path,
    base::flat_map<std::string, std::string> response_headers) {
  VLOG(2) << __func__ << " id: " << id;
#if BUILDFLAG(IS_ANDROID)
  if (!path.empty()) {
    // Anroid requires spedific format for thumbnail file.
    std::string extension = "png";
    if (response_headers.contains("content-type")) {
      std::string content_type = response_headers.at("content-type");
      if (base::StartsWith(content_type, "image/")) {
        extension = content_type.substr(6);
      }
    }
    RenameFilePerFormat(id, path, extension);
    return;
  }
#endif
  DCHECK(!ticket_map_.empty());
  ticket_map_.erase(id);
  delegate_->OnThumbnailDownloaded(id, path);
}

#if BUILDFLAG(IS_ANDROID)
void PlaylistThumbnailDownloader::RenameFilePerFormat(
    const std::string& id,
    const base::FilePath& path,
    const std::string& extension) {
  if (!task_runner_) {
    task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  base::FilePath new_path = path.AddExtension(extension);
  task_runner_->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::ReplaceFile, path, new_path, nullptr),
      base::BindOnce(&PlaylistThumbnailDownloader::OnRenameFilePerFormat,
                     base::Unretained(this), id, new_path));
}

void PlaylistThumbnailDownloader::OnRenameFilePerFormat(
    const std::string& id,
    const base::FilePath& new_path,
    bool result) {
  DCHECK(!ticket_map_.empty());
  ticket_map_.erase(id);
  delegate_->OnThumbnailDownloaded(
      id, result ? new_path : new_path.RemoveExtension());
}
#endif
