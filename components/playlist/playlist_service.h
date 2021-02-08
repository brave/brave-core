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
class PlaylistYoutubeDownComponentManager;
struct CreatePlaylistParams;

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
// You can see all notification type - PlaylistChangeParams::ChangeType.
class PlaylistService : public KeyedService,
                        public PlaylistMediaFileDownloadManager::Delegate,
                        public PlaylistThumbnailDownloader::Delegate,
                        public PlaylistDownloadRequestManager::Delegate {
 public:
  PlaylistService(content::BrowserContext* context,
                  PlaylistYoutubeDownComponentManager* manager);
  ~PlaylistService() override;
  PlaylistService(const PlaylistService&) = delete;
  PlaylistService& operator=(const PlaylistService&) = delete;

  base::Value GetAllPlaylistItems();
  base::Value GetPlaylistItem(const std::string& id);
  void RecoverPlaylistItem(const std::string& id);
  void DeletePlaylistItem(const std::string& id);
  void DeleteAllPlaylistItems();
  void RequestDownload(const std::string& url);

  void AddObserver(PlaylistServiceObserver* observer);
  void RemoveObserver(PlaylistServiceObserver* observer);

  bool GetThumbnailPath(const std::string& id, base::FilePath* thumbnail_path);

  base::FilePath GetPlaylistItemDirPath(const std::string& id) const;

 private:
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, CreatePlaylist);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest,
                           CreatePlaylistWithSeparateAudio);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, ThumbnailFailed);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, MediaDownloadFailed);
  FRIEND_TEST_ALL_PREFIXES(PlaylistBrowserTest, ApiFunctions);

  // KeyedService overrides:
  void Shutdown() override;

  // PlaylistMediaFileDownloadManager::Delegate overrides:
  // Called when all audio/video media file are downloaded.
  void OnMediaFileReady(const std::string& id,
                        const std::string& audio_file_path,
                        const std::string& video_file_path) override;
  void OnMediaFileGenerationFailed(const std::string& id) override;
  bool IsValidPlaylistItem(const std::string& id) override;

  // PlaylistThumbnailDownloader::Delegate overrides:
  // Called when thumbnail image file is downloaded.
  void OnThumbnailDownloaded(const std::string& id,
                             const base::FilePath& path) override;

  // PlaylistDownloadRequestManager::Delegate overrides:
  // Called when meta data is ready. |params| have playlist item's audio/video
  // media files url, thumbnail and title.
  void OnPlaylistCreationParamsReady(
      const CreatePlaylistParams& params) override;

  void OnPlaylistItemDirCreated(const std::string& id, bool directory_ready);

  void CreatePlaylistItem(const CreatePlaylistParams& params);
  void DownloadThumbnail(const std::string& id);
  void GenerateMediafileForPlaylistItem(const std::string& id);

  base::SequencedTaskRunner* task_runner();

  // Delete orphaned playlist item directories that are not included in db.
  void CleanUp();
  void OnGetOrphanedPaths(const std::vector<base::FilePath> paths);

  void NotifyPlaylistChanged(const PlaylistChangeParams& params);

  void UpdatePlaylistValue(const std::string& id, base::Value value);
  void RemovePlaylist(const std::string& id);

  // index.html is only used for demo (brave://playlist)
  void GenerateIndexHTMLFile(const base::FilePath& playlist_path);
  void OnHTMLFileGenerated(bool generated);
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
  PrefService* prefs_;

  base::WeakPtrFactory<PlaylistService> weak_factory_;
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_PLAYLIST_SERVICE_H_
