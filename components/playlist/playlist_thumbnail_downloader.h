/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_THUMBNAIL_DOWNLOADER_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_THUMBNAIL_DOWNLOADER_H_

#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

class GURL;

class PlaylistThumbnailDownloader {
 public:
  class Delegate {
   public:
    // If |path| is empty, thumbnail fetching for |id| is failed.
    virtual void OnThumbnailDownloaded(const std::string& id,
                                       const base::FilePath& path) = 0;
  };

  PlaylistThumbnailDownloader(content::BrowserContext* context,
                              Delegate* delegate);
  virtual ~PlaylistThumbnailDownloader();

  PlaylistThumbnailDownloader(const PlaylistThumbnailDownloader&) = delete;
  PlaylistThumbnailDownloader& operator=(const PlaylistThumbnailDownloader&) =
      delete;

  void DownloadThumbnail(const std::string& id,
                         const GURL& thumbnail_url,
                         const base::FilePath& target_thumbnail_path);
  void CancelDownloadRequest(const std::string& id);
  void CancelAllDownloadRequests();

 private:
  using SimpleURLLoaderMap =
      base::flat_map<std::string, std::unique_ptr<network::SimpleURLLoader>>;

  void OnThumbnailDownloaded(const std::string& id, base::FilePath path);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  SimpleURLLoaderMap url_loaders_;
  Delegate* delegate_;
};

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_THUMBNAIL_DOWNLOADER_H_
