/* Copyright (c) 2020 The Brave Authors. All rights reserved.
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
    virtual void OnMediaFileReady(const std::string& id,
                                  const std::string& audio_file_path,
                                  const std::string& video_file_path) = 0;
    virtual void OnMediaFileGenerationFailed(const std::string& id) = 0;
    virtual bool IsValidPlaylistItem(const std::string& id) = 0;

   protected:
    virtual ~Delegate() {}
  };

  PlaylistMediaFileDownloadManager(content::BrowserContext* context,
                                   Delegate* delegate,
                                   const base::FilePath& base_dir);
  ~PlaylistMediaFileDownloadManager() override;

  PlaylistMediaFileDownloadManager(const PlaylistMediaFileDownloadManager&) =
      delete;
  PlaylistMediaFileDownloadManager& operator=(
      const PlaylistMediaFileDownloadManager&) = delete;

  void GenerateMediaFileForPlaylistItem(const base::Value& playlist_item);
  void CancelDownloadRequest(const std::string& id);
  void CancelAllDownloadRequests();

 private:
  // PlaylistMediaFileDownloader::Delegate overrides:
  void OnMediaFileReady(const std::string& id,
                        const std::string& media_file_path_key,
                        const std::string& media_file_path) override;
  void OnMediaFileGenerationFailed(const std::string& id) override;

  void GenerateMediaFiles();
  base::Value GetNextPlaylistItemTarget();
  std::string GetCurrentDownloadingPlaylistItemID() const;
  void CancelCurrentDownloadingPlaylistItem();
  bool IsCurrentDownloadingInProgress() const;
  void ResetCurrentPlaylistItemInfo();

  const base::FilePath base_dir_;
  Delegate* delegate_;
  base::queue<base::Value> pending_media_file_creation_jobs_;
  std::string current_playlist_item_id_;
  std::string current_playlist_item_audio_file_path_;
  std::string current_playlist_item_video_file_path_;

  // TODO(simonhong): Unify these two downloader into one. Using two downloaders
  // just increase complexity.
  std::unique_ptr<PlaylistMediaFileDownloader> video_media_file_downloader_;
  std::unique_ptr<PlaylistMediaFileDownloader> audio_media_file_downloader_;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_MEDIA_FILE_DOWNLOAD_MANAGER_H_
