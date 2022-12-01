/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_MEDIA_FILE_DOWNLOADER_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_MEDIA_FILE_DOWNLOADER_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/playlist/playlist_types.h"

namespace api_request_helper {
class APIRequestHelper;
}  // namespace api_request_helper

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

class GURL;

namespace playlist {

// Handle one Playlist at once.
class PlaylistMediaFileDownloader {
 public:
  class Delegate {
   public:
    // Called when target media file generation succeed.
    virtual void OnMediaFileReady(const std::string& id,
                                  const std::string& media_file_path) = 0;
    // Called when target media file generation failed.
    virtual void OnMediaFileGenerationFailed(const std::string& id) = 0;

   protected:
    virtual ~Delegate() {}
  };

  PlaylistMediaFileDownloader(Delegate* delegate,
                              content::BrowserContext* context,
                              base::FilePath::StringType media_file_name);
  virtual ~PlaylistMediaFileDownloader();

  PlaylistMediaFileDownloader(const PlaylistMediaFileDownloader&) = delete;
  PlaylistMediaFileDownloader& operator=(const PlaylistMediaFileDownloader&) =
      delete;

  void DownloadMediaFileForPlaylistItem(const PlaylistItemInfo& item,
                                        const base::FilePath& base_dir);

  void RequestCancelCurrentPlaylistGeneration();

  bool in_progress() const { return in_progress_; }
  const std::string& current_playlist_id() const { return current_item_->id; }

 private:
  void ResetDownloadStatus();
  void DownloadMediaFile(const GURL& url, int index);
  void OnMediaFileDownloaded(
      int index,
      base::FilePath path,
      base::flat_map<std::string, std::string> response_headers);

  void NotifyFail(const std::string& id);
  void NotifySucceed(const std::string& id, const std::string& media_file_path);

  base::SequencedTaskRunner* task_runner();

  raw_ptr<Delegate> delegate_ = nullptr;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<api_request_helper::APIRequestHelper> request_helper_;

  const base::FilePath::StringType media_file_name_;

  // All below variables are only for playlist creation.
  base::FilePath playlist_dir_path_;
  std::unique_ptr<PlaylistItemInfo> current_item_;

  // true when this class is working for playlist now.
  bool in_progress_ = false;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  base::WeakPtrFactory<PlaylistMediaFileDownloader> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_MEDIA_FILE_DOWNLOADER_H_
