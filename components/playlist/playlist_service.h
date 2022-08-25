/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_H_
#define BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "base/values.h"
#include "brave/components/playlist/playlist_download_request_manager.h"
#include "brave/components/playlist/playlist_media_file_download_manager.h"
#include "brave/components/playlist/playlist_thumbnail_downloader.h"
#include "brave/components/playlist/playlist_types.h"
#include "components/keyed_service/core/keyed_service.h"

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

class PrefService;

namespace playlist {

class PlaylistServiceObserver;
class MediaDetectorComponentManager;

// This class is key interace for playlist. Client will ask any playlist related
// requests to this class.
// This handles youtube playlist download request by orchestrating three other
// classes. They are PlaylistMediaFileDownloadManager,
// PlaylistThunbnailDownloadManager and PlaylistDownloadRequestManager.
// PlaylistService owns all these managers.
// This notifies each playlist's status to users via PlaylistServiceObserver.

// Playlist download request is started by calling RequestDownload() from
// client. Passed argument is youtube url. Then, PlaylistDownloadRequestManager
// gives meta data. That meta data has urls for playlist item's audio/video
// media file urls and thumbnail url.
// Next, PlaylistService asks downloading audio/video media files and thumbnails
// to PlaylistMediaFileDownloadManager and PlaylistThumbnailDownloader.
// When each of data is ready to use it's notified to client.
// You can see all notification type - PlaylistItemChangeParams::Type.
class PlaylistService : public KeyedService,
                        public PlaylistMediaFileDownloadManager::Delegate,
                        public PlaylistThumbnailDownloader::Delegate {
 public:
  PlaylistService(content::BrowserContext* context,
                  MediaDetectorComponentManager* manager);
  ~PlaylistService() override;
  PlaylistService(const PlaylistService&) = delete;
  PlaylistService& operator=(const PlaylistService&) = delete;

  void CreatePlaylist(const PlaylistInfo& info);
  void RemovePlaylist(const std::string& id);

  std::vector<PlaylistItemInfo> GetAllPlaylistItems();
  PlaylistItemInfo GetPlaylistItem(const std::string& id);

  absl::optional<PlaylistInfo> GetPlaylist(const std::string& id);
  std::vector<PlaylistInfo> GetAllPlaylists();

  void RequestDownloadMediaFilesFromContents(const std::string& playlist_id,
                                             content::WebContents* contents);
  void RequestDownloadMediaFilesFromPage(const std::string& playlist_id,
                                         const std::string& url);
  void RecoverPlaylistItem(const std::string& id);

  void RemoveItemFromPlaylist(const std::string& playlist_id,
                              const std::string& item_id);
  void DeletePlaylistItem(const std::string& id);
  void DeleteAllPlaylistItems();

  void AddObserver(PlaylistServiceObserver* observer);
  void RemoveObserver(PlaylistServiceObserver* observer);

  bool GetThumbnailPath(const std::string& id, base::FilePath* thumbnail_path);
  bool GetMediaPath(const std::string& id, base::FilePath* media_path);

  base::FilePath GetPlaylistItemDirPath(const std::string& id) const;

 private:
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, ApiFunctions);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, CreatePlaylist);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, CreatePlaylistItem);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, MediaDownloadFailed);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, ThumbnailFailed);

  // KeyedService overrides:
  void Shutdown() override;

  // PlaylistMediaFileDownloadManager::Delegate overrides:
  // Called when all audio/video media file are downloaded.
  void OnMediaFileReady(const std::string& id,
                        const std::string& media_file_path) override;
  void OnMediaFileGenerationFailed(const std::string& id) override;
  bool IsValidPlaylistItem(const std::string& id) override;

  // PlaylistThumbnailDownloader::Delegate overrides:
  // Called when thumbnail image file is downloaded.
  void OnThumbnailDownloaded(const std::string& id,
                             const base::FilePath& path) override;

  // Called when meta data is fetched from url.
  void OnPlaylistCreationParamsReady(
      const std::string& playlist_id,
      const std::vector<PlaylistItemInfo>& params);

  void OnPlaylistItemDirCreated(const PlaylistItemInfo& info,
                                bool directory_ready);

  void CreatePlaylistItem(const PlaylistItemInfo& info);
  void DownloadThumbnail(const PlaylistItemInfo& info);
  void GenerateMediafileForPlaylistItem(const PlaylistItemInfo& info);

  base::SequencedTaskRunner* task_runner();

  // Delete orphaned playlist item directories that are not included in db.
  void CleanUp();
  void OnGetOrphanedPaths(const std::vector<base::FilePath> paths);

  void NotifyPlaylistChanged(const PlaylistChangeParams& params);

  void UpdatePlaylistItemValue(const std::string& id, base::Value value);
  void RemovePlaylistItemValue(const std::string& id);

  bool HasPrefStorePlaylistItem(const std::string& id) const;

  // Playlist creation can be ready to play two steps.
  // Step 1. When creation is requested, requested info is put to db and
  //         notification is delivered to user with basic infos like playlist
  //         name and titles if provided. playlist is visible to user but it
  //         doesn't have thumbnail.
  // Step 1. Request thumbnail.
  //         Request video files and audio files and combined as a single
  //         video and audio file.
  //         Whenever thumbnail is fetched or media files are ready,
  //         it is notified.

  void OnGetMetadata(base::Value value);

  const base::FilePath base_dir_;
  base::ObserverList<PlaylistServiceObserver> observers_;

  std::unique_ptr<PlaylistMediaFileDownloadManager>
      media_file_download_manager_;
  std::unique_ptr<PlaylistThumbnailDownloader> thumbnail_downloader_;

  std::unique_ptr<PlaylistDownloadRequestManager> download_request_manager_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  raw_ptr<PrefService> prefs_ = nullptr;

  base::WeakPtrFactory<PlaylistService> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_H_
