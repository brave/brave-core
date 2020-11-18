/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_thumbnail_downloader.h"

#include <utility>

#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

constexpr unsigned int kRetriesCountOnNetworkChange = 1;

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
    : url_loader_factory_(
          content::BrowserContext::GetDefaultStoragePartition(context)
              ->GetURLLoaderFactoryForBrowserProcess()),
      delegate_(delegate) {}

PlaylistThumbnailDownloader::~PlaylistThumbnailDownloader() = default;

void PlaylistThumbnailDownloader::DownloadThumbnail(
    const std::string& id,
    const GURL& thumbnail_url,
    const base::FilePath& target_thumbnail_path) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = thumbnail_url;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  auto loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTagForURLLoad());
  loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  loader->DownloadToFile(
      url_loader_factory_.get(),
      base::BindOnce(&PlaylistThumbnailDownloader::OnThumbnailDownloaded,
                     base::Unretained(this), id),
      target_thumbnail_path);
  url_loaders_.insert_or_assign(id, std::move(loader));
}

void PlaylistThumbnailDownloader::CancelDownloadRequest(const std::string& id) {
  url_loaders_.erase(id);
}

void PlaylistThumbnailDownloader::CancelAllDownloadRequests() {
  url_loaders_.clear();
}

void PlaylistThumbnailDownloader::OnThumbnailDownloaded(const std::string& id,
                                                        base::FilePath path) {
  DCHECK(!url_loaders_.empty());
  url_loaders_.erase(id);
  delegate_->OnThumbnailDownloaded(id, path);
}
