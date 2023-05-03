/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_FILE_DOWNLOAD_MANAGER_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_FILE_DOWNLOAD_MANAGER_H_

#include <memory>
#include <string>

#include "base/containers/queue.h"
#include "brave/components/playlist/browser/playlist_media_file_downloader.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"

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
  struct DownloadJob {
    // This struct is move-only type.
    DownloadJob();
    DownloadJob(const DownloadJob&) = delete;
    DownloadJob& operator=(const DownloadJob&) = delete;
    DownloadJob(DownloadJob&&) noexcept;
    DownloadJob& operator=(DownloadJob&&) noexcept;
    ~DownloadJob();

    mojom::PlaylistItemPtr item;

    base::RepeatingCallback<void(const mojom::PlaylistItemPtr& /*item*/,
                                 int64_t /*total_bytes*/,
                                 int64_t /*received_bytes*/,
                                 int /*percent_complete*/,
                                 base::TimeDelta /*time_remaining*/)>
        on_progress_callback;

    // If the manage fails to download file, the |media_file_path| will be
    // empty.
    base::OnceCallback<void(mojom::PlaylistItemPtr /*item*/,
                            const std::string& /*media_file_path*/)>
        on_finish_callback;
  };

  class Delegate {
   public:
    virtual bool IsValidPlaylistItem(const std::string& id) = 0;
    virtual base::FilePath GetMediaPathForPlaylistItemItem(
        const std::string& id) = 0;

   protected:
    virtual ~Delegate() {}
  };

  PlaylistMediaFileDownloadManager(content::BrowserContext* context,
                                   Delegate* delegate);
  ~PlaylistMediaFileDownloadManager() override;

  PlaylistMediaFileDownloadManager(const PlaylistMediaFileDownloadManager&) =
      delete;
  PlaylistMediaFileDownloadManager& operator=(
      const PlaylistMediaFileDownloadManager&) = delete;

  void DownloadMediaFile(std::unique_ptr<DownloadJob> request);
  void CancelDownloadRequest(const std::string& id);
  void CancelAllDownloadRequests();

  bool has_download_requests() const { return !!current_job_; }

 private:
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, ResetAll);

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
  std::unique_ptr<DownloadJob> PopNextJob();
  std::string GetCurrentDownloadingPlaylistItemID() const;
  void CancelCurrentDownloadingPlaylistItem();
  bool IsCurrentDownloadingInProgress() const;

  raw_ptr<Delegate> delegate_;
  base::queue<std::unique_ptr<DownloadJob>> pending_media_file_creation_jobs_;

  std::unique_ptr<DownloadJob> current_job_;

  std::unique_ptr<PlaylistMediaFileDownloader> media_file_downloader_;

  bool pause_download_for_testing_ = false;

  base::WeakPtrFactory<PlaylistMediaFileDownloadManager> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_MEDIA_FILE_DOWNLOAD_MANAGER_H_
