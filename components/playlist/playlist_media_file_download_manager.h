/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_MEDIA_FILE_DOWNLOAD_MANAGER_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_MEDIA_FILE_DOWNLOAD_MANAGER_H_

#include <memory>
#include <string>

#include "base/containers/queue.h"
#include "brave/components/playlist/playlist_media_file_downloader.h"

namespace base {
class Value;
}  // namespace base

namespace content {
class BrowserContext;
}  // namespace content

namespace playlist {

// Download youtube playlist item's audio/video media files.
// This handles one request at once. So, it has pending queue.
// And PlaylistMediaFileDownloader does file download task.
// TODO(simonhong): Download multiple media files simultaneously.
class PlaylistMediaFileDownloadManager
    : public PlaylistMediaFileDownloader::Delegate {
 public:
  class Delegate {
   public:
    virtual void OnMediaFileDownloadProgressed(
        const std::string& id,
        int64_t total_bytes,
        int64_t received_bytes,
        int percent_complete,
        base::TimeDelta time_remaining) = 0;
    virtual void OnMediaFileReady(const std::string& id,
                                  const std::string& media_file_path) = 0;
    virtual void OnMediaFileGenerationFailed(const std::string& id) = 0;
    virtual bool IsValidPlaylistItem(const std::string& id) = 0;

   protected:
    virtual ~Delegate() {}
  };

  static constexpr base::FilePath::CharType kMediaFileName[] =
      FILE_PATH_LITERAL("media_file.mp4");

  PlaylistMediaFileDownloadManager(content::BrowserContext* context,
                                   Delegate* delegate,
                                   const base::FilePath& base_dir);
  ~PlaylistMediaFileDownloadManager() override;

  PlaylistMediaFileDownloadManager(const PlaylistMediaFileDownloadManager&) =
      delete;
  PlaylistMediaFileDownloadManager& operator=(
      const PlaylistMediaFileDownloadManager&) = delete;

  void DownloadMediaFile(const PlaylistItemInfo& playlist_item);
  void CancelDownloadRequest(const std::string& id);
  void CancelAllDownloadRequests();

 private:
  // PlaylistMediaFileDownloader::Delegate overrides:
  void OnMediaFileDownloadProgressed(const std::string& id,
                                     int64_t total_bytes,
                                     int64_t received_bytes,
                                     int percent_complete,
                                     base::TimeDelta time_remaining) override;
  void OnMediaFileReady(const std::string& id,
                        const std::string& media_file_path) override;
  void OnMediaFileGenerationFailed(const std::string& id) override;

  void TryStartingDownloadTask();
  std::unique_ptr<PlaylistItemInfo> GetNextPlaylistItemTarget();
  std::string GetCurrentDownloadingPlaylistItemID() const;
  void CancelCurrentDownloadingPlaylistItem();
  bool IsCurrentDownloadingInProgress() const;

  const base::FilePath base_dir_;
  raw_ptr<Delegate> delegate_;
  base::queue<PlaylistItemInfo> pending_media_file_creation_jobs_;

  std::unique_ptr<PlaylistItemInfo> current_item_;

  std::unique_ptr<PlaylistMediaFileDownloader> media_file_downloader_;

  base::WeakPtrFactory<PlaylistMediaFileDownloadManager> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_MEDIA_FILE_DOWNLOAD_MANAGER_H_
