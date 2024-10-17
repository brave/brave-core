/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_thumbnail_downloader.h"

#include <utility>

#include "base/files/file_util.h"
#include "base/task/thread_pool.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "ui/gfx/image/image.h"

namespace playlist {

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
      delegate_(delegate) {}

PlaylistThumbnailDownloader::~PlaylistThumbnailDownloader() = default;

void PlaylistThumbnailDownloader::DownloadThumbnail(
    const std::string& id,
    const GURL& thumbnail_url,
    const base::FilePath& target_thumbnail_path) {
  VLOG(2) << __func__ << " " << id << " : " << thumbnail_url.spec();
  CancelDownloadRequest(id);

  if (pause_download_for_testing_) {
    url_loader_map_[id] = {};
    return;
  }

  CreateURLLoader(id, thumbnail_url)
      ->DownloadToString(
          url_loader_factory_.get(),
          base::BindOnce(&PlaylistThumbnailDownloader::SaveResponseToFile,
                         weak_ptr_factory_.GetWeakPtr(), id,
                         target_thumbnail_path),
          network::SimpleURLLoader::kMaxBoundedStringDownloadSize);
}

void PlaylistThumbnailDownloader::DownloadThumbnail(
    const std::string& id,
    const GURL& thumbnail_url,
    base::OnceCallback<void(gfx::Image)> callback) {
  VLOG(2) << __func__ << " " << id << " : " << thumbnail_url.spec();
  CancelDownloadRequest(id);

  if (pause_download_for_testing_) {
    url_loader_map_[id] = {};
    return;
  }

  CreateURLLoader(id, thumbnail_url)
      ->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
          url_loader_factory_.get(),
          base::BindOnce(&PlaylistThumbnailDownloader::ConvertResponseToImage,
                         weak_ptr_factory_.GetWeakPtr(), id,
                         std::move(callback)));
}

void PlaylistThumbnailDownloader::CancelDownloadRequest(const std::string& id) {
  VLOG(2) << __func__ << " " << id;
  if (!url_loader_map_.count(id)) {
    return;
  }

  url_loader_map_.erase(id);
}

void PlaylistThumbnailDownloader::CancelAllDownloadRequests() {
  VLOG(2) << __func__;
  url_loader_map_.clear();
}

void PlaylistThumbnailDownloader::SaveResponseToFile(
    const std::string& id,
    base::FilePath path,
    std::unique_ptr<std::string> response_body) {
  DVLOG(2) << __func__ << " id: " << id;

  if (!url_loader_map_.count(id)) {
    // Download could have been canceled
    return;
  }

  if (!response_body) {
    url_loader_map_.erase(id);
    delegate_->OnThumbnailDownloaded(id, {});
    return;
  }

  auto on_save = base::BindOnce(
      [](base::WeakPtr<PlaylistThumbnailDownloader> self, std::string id,
         base::FilePath path) {
        if (!self) {
          return;
        }

        if (!self->url_loader_map_.count(id)) {
          // Download could have been canceled
          return;
        }

        self->url_loader_map_.erase(id);
        self->delegate_->OnThumbnailDownloaded(id, path);
      },
      weak_ptr_factory_.GetWeakPtr(), id);

  auto write_to_file =
      base::BindOnce(&PlaylistThumbnailDownloader::WriteToFile,
                     weak_ptr_factory_.GetWeakPtr(), path, std::move(on_save));
  delegate_->SanitizeImage(std::move(response_body), std::move(write_to_file));
}

void PlaylistThumbnailDownloader::ConvertResponseToImage(
    const std::string& id,
    base::OnceCallback<void(gfx::Image)> callback,
    std::unique_ptr<std::string> response_body) {
  DVLOG(2) << __func__ << " id: " << id;

  if (!url_loader_map_.count(id)) {
    // Download could have been canceled
    return;
  }

  const bool has_response = !!response_body;
  if (!has_response) {
    url_loader_map_.erase(id);
    std::move(callback).Run({});
    return;
  }

  auto on_sanitize = base::BindOnce(
      [](base::WeakPtr<PlaylistThumbnailDownloader> self, std::string id,
         base::OnceCallback<void(gfx::Image)> callback,
         scoped_refptr<base::RefCountedBytes> image) {
        if (!self) {
          return;
        }

        if (!self->url_loader_map_.count(id)) {
          // Download could have been canceled
          return;
        }

        self->url_loader_map_.erase(id);
        CHECK(callback);
        std::move(callback).Run(gfx::Image::CreateFrom1xPNGBytes(image));
      },
      weak_ptr_factory_.GetWeakPtr(), id, std::move(callback));

  delegate_->SanitizeImage(std::move(response_body), std::move(on_sanitize));
}

void PlaylistThumbnailDownloader::WriteToFile(
    const base::FilePath& path,
    base::OnceCallback<void(base::FilePath)> callback,
    scoped_refptr<base::RefCountedBytes> image) {
  if (!image || !image->size()) {
    return std::move(callback).Run({});
  }

  auto target_path = path;

#if BUILDFLAG(IS_ANDROID)
  // Android requires specific format for thumbnail file.
  target_path = target_path.AddExtension("png");
#endif

  auto write_to_file = base::BindOnce(
      [](base::FilePath path, scoped_refptr<base::RefCountedBytes> image) {
        if (!base::WriteFile(path, UNSAFE_TODO(base::span<const uint8_t>(
                                       image->front(), image->size())))) {
          DVLOG(2) << "Failed to write image to file " << path;
          return base::FilePath();
        }
        return path;
      },
      target_path, std::move(image));

  GetOrCreateTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE, std::move(write_to_file), std::move(callback));
}

scoped_refptr<base::SequencedTaskRunner>
PlaylistThumbnailDownloader::GetOrCreateTaskRunner() {
  if (!task_runner_) {
    task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN});
  }
  return task_runner_;
}

network::SimpleURLLoader* PlaylistThumbnailDownloader::CreateURLLoader(
    const std::string& id,
    const GURL& url) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES | net::LOAD_BYPASS_CACHE |
                        net::LOAD_DISABLE_CACHE;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTagForURLLoad());
  const unsigned int kRetriesCountOnNetworkChange = 1;
  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  url_loader->SetAllowHttpErrorResults(false);

  url_loader_map_.insert({id, std::move(url_loader)});
  return url_loader_map_.at(id).get();
}

}  // namespace playlist
