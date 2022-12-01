/* Copyright (c) 2021 The Brave Authors. All rights reserved.
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
#include "brave/components/api_request_helper/api_request_helper.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

#if BUILDFLAG(IS_ANDROID)
namespace base {
class SequencedTaskRunner;
}  // namespace base
#endif

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
  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using TicketMap = base::flat_map<std::string, APIRequestHelper::Ticket>;

  void OnThumbnailDownloaded(
      const std::string& id,
      base::FilePath path,
      base::flat_map<std::string, std::string> response_headers);

#if BUILDFLAG(IS_ANDROID)
  void RenameFilePerFormat(const std::string& id,
                           const base::FilePath& path,
                           const std::string& extension);
  void OnRenameFilePerFormat(const std::string& id,
                             const base::FilePath& new_path,
                             bool result);

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
#endif

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;

  std::unique_ptr<api_request_helper::APIRequestHelper> request_helper_;
  TicketMap ticket_map_;

  raw_ptr<Delegate> delegate_;
};

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_THUMBNAIL_DOWNLOADER_H_
