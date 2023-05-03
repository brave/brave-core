/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_FILE_DOWNLOADER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_FILE_DOWNLOADER_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/weak_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/scoped_observation.h"
#include "base/values.h"
#include "brave/components/playlist/browser/playlist_types.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "components/download/public/common/download_item.h"
#include "components/download/public/common/simple_download_manager.h"

namespace base {
class FilePath;
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace download {
class InProgressDownloadManager;
class DownloadItemImpl;
}  // namespace download

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

class GURL;

namespace playlist {

// Handle one Playlist at once.
class PlaylistMediaFileDownloader
    : public download::SimpleDownloadManager::Observer,
      public download::DownloadItem::Observer {
 public:
  class Delegate {
   public:
    virtual void OnMediaFileDownloadProgressed(
        const std::string& id,
        int64_t total_bytes,
        int64_t received_bytes,
        int percent_complete,
        base::TimeDelta time_remaining) = 0;

    // Called when target media file generation succeed.
    virtual void OnMediaFileReady(const std::string& id,
                                  const std::string& media_file_path) = 0;
    // Called when target media file generation failed.
    virtual void OnMediaFileGenerationFailed(const std::string& id) = 0;

   protected:
    virtual ~Delegate() {}
  };

  PlaylistMediaFileDownloader(Delegate* delegate,
                              content::BrowserContext* context);
  ~PlaylistMediaFileDownloader() override;

  PlaylistMediaFileDownloader(const PlaylistMediaFileDownloader&) = delete;
  PlaylistMediaFileDownloader& operator=(const PlaylistMediaFileDownloader&) =
      delete;

  void DownloadMediaFileForPlaylistItem(const mojom::PlaylistItemPtr& item,
                                        const base::FilePath& base_dir);

  void RequestCancelCurrentPlaylistGeneration();

  bool in_progress() const { return in_progress_; }
  const std::string& current_playlist_id() const { return current_item_->id; }

  // download::SimpleDownloadManager::Observer:
  void OnDownloadCreated(download::DownloadItem* item) override;

  // download::DownloadItemObserver:
  void OnDownloadUpdated(download::DownloadItem* item) override;
  void OnDownloadRemoved(download::DownloadItem* item) override;

 private:
  void ResetDownloadStatus();
  void DownloadMediaFile(const GURL& url);
  void OnMediaFileDownloaded(base::FilePath path);

  void NotifyFail(const std::string& id);
  void NotifySucceed(const std::string& id, const std::string& media_file_path);

  void ScheduleToDetachCachedFile(download::DownloadItem* item);
  void DetachCachedFile(download::DownloadItem* item);

  base::SequencedTaskRunner* task_runner();

  raw_ptr<Delegate> delegate_ = nullptr;

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<download::InProgressDownloadManager> download_manager_;
  std::vector<std::unique_ptr<download::DownloadItemImpl>>
      download_items_to_be_detached_;

  base::ScopedObservation<download::SimpleDownloadManager,
                          download::SimpleDownloadManager::Observer>
      download_manager_observation_{this};
  base::ScopedMultiSourceObservation<download::DownloadItem,
                                     download::DownloadItem::Observer>
      download_item_observation_{this};

  // All below variables are only for playlist creation.
  base::FilePath destination_path_;
  mojom::PlaylistItemPtr current_item_;

  // true when this class is working for playlist now.
  bool in_progress_ = false;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  base::WeakPtrFactory<PlaylistMediaFileDownloader> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_FILE_DOWNLOADER_H_
