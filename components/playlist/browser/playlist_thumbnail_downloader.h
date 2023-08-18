/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_THUMBNAIL_DOWNLOADER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_THUMBNAIL_DOWNLOADER_H_

#include <list>
#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_refptr.h"
#include "services/network/public/cpp/simple_url_loader.h"
namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace gfx {
class Image;
}  // namespace gfx

#if BUILDFLAG(IS_ANDROID)
namespace base {
class SequencedTaskRunner;
}  // namespace base
#endif

class GURL;

namespace playlist {

class PlaylistThumbnailDownloader {
 public:
  class Delegate {
   public:
    virtual void SanitizeImage(
        std::unique_ptr<std::string> image,
        base::OnceCallback<void(scoped_refptr<base::RefCountedBytes>)>
            callback) = 0;

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
  void DownloadThumbnail(const std::string& id,
                         const GURL& thumbnail_url,
                         base::OnceCallback<void(gfx::Image)> callback);

  void CancelDownloadRequest(const std::string& id);
  void CancelAllDownloadRequests();

  bool has_download_requests() const { return url_loader_map_.size(); }

 private:
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, ResetAll);

  using URLLoaders = std::list<std::unique_ptr<network::SimpleURLLoader>>;
  using URLLoaderIter = URLLoaders::iterator;
  using IDToURLLoaderIterMap = base::flat_map<std::string, URLLoaderIter>;

  URLLoaderIter CreateURLLoaderHandler(const GURL& url);
  void Cancel(const URLLoaderIter& ticket);

  void SaveResponseToFile(URLLoaderIter iter,
                          const std::string& id,
                          base::FilePath path,
                          std::unique_ptr<std::string> response_body);
  void ConvertResponseToImage(URLLoaderIter iter,
                              const std::string& id,
                              base::OnceCallback<void(gfx::Image)> callback,
                              std::unique_ptr<std::string> response_body);

  void WriteToFile(const base::FilePath& path,
                   base::OnceCallback<void(base::FilePath)> callback,
                   scoped_refptr<base::RefCountedBytes> image);

  scoped_refptr<base::SequencedTaskRunner> GetOrCreateTaskRunner();

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  URLLoaders url_loaders_;
  IDToURLLoaderIterMap url_loader_map_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  bool pause_download_for_testing_ = false;

  raw_ptr<Delegate> delegate_;

  base::WeakPtrFactory<PlaylistThumbnailDownloader> weak_ptr_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_THUMBNAIL_DOWNLOADER_H_
