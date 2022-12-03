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

namespace blink::web_pref {
struct WebPreferences;
}  // namespace blink::web_pref

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

class CosmeticFilteringPlaylistFlagEnabledTest;
class PlaylistRenderFrameObserverBrowserTest;
class PrefService;

namespace playlist {

class PlaylistServiceObserver;
class MediaDetectorComponentManager;

// This class is key interface for playlist. Client will ask any playlist
// related requests to this class. This handles youtube playlist download
// request by orchestrating three other classes. They are
// PlaylistMediaFileDownloadManager, PlaylistThumbnailDownloadManager and
// PlaylistDownloadRequestManager. PlaylistService owns all these managers. This
// notifies each playlist's status to users via PlaylistServiceObserver.
//
//                  Service│ Request             Update prefs/Create dir
//                         │    │                       ▲ │
//   DownloadRequestManager│    └─► Find Media files ───┘ │
//                         │                              │
//      ThumbnailDownloader│                              ├──►Download thumbnail
//                         │                              │
// MediaFileDownloadManager│                              └──►Download media
//
//
// Playlist download request is started by calling
// RequestDownloadMediaFilesFrom{Contents, Page, Items}() from client. Then,
// PlaylistDownloadRequestManager gives meta data. That meta data has urls for
// playlist item's audio/video media file urls and thumbnail url. Next,
// PlaylistService asks downloading audio/video media files and thumbnails to
// PlaylistMediaFileDownloadManager and PlaylistThumbnailDownloader. When each
// of data is ready to use it's notified to client. You can see all notification
// type - PlaylistItemChangeParams::Type.
class PlaylistService : public KeyedService,
                        public PlaylistMediaFileDownloadManager::Delegate,
                        public PlaylistThumbnailDownloader::Delegate {
 public:
  using PlaylistId = base::StrongAlias<class PlaylistIdTag, std::string>;
  using PlaylistItemId =
      base::StrongAlias<class PlaylistItemIdTag, std::string>;

  PlaylistService(content::BrowserContext* context,
                  MediaDetectorComponentManager* manager);
  ~PlaylistService() override;
  PlaylistService(const PlaylistService&) = delete;
  PlaylistService& operator=(const PlaylistService&) = delete;

  // This function will fill |info|'s id with a new generated id.
  void CreatePlaylist(PlaylistInfo& info);
  void RemovePlaylist(const std::string& id);

  std::vector<PlaylistItemInfo> GetAllPlaylistItems();
  PlaylistItemInfo GetPlaylistItem(const std::string& id);

  absl::optional<PlaylistInfo> GetPlaylist(const std::string& id);
  std::vector<PlaylistInfo> GetAllPlaylists();

  // Finds media files from |contents| or |url| and adds them to given
  // |playlist_id|.
  void RequestDownloadMediaFilesFromContents(const std::string& playlist_id,
                                             content::WebContents* contents);
  void RequestDownloadMediaFilesFromPage(const std::string& playlist_id,
                                         const std::string& url);

  // Add given |items| to the |playlist_id|. Usually follows after
  // FindMediaFilesFromContents().
  void RequestDownloadMediaFilesFromItems(
      const std::string& playlist_id,
      const std::vector<PlaylistItemInfo>& items);

  // Unlike Request methods above, do nothing with prefs or downloading. Just
  // find media files from given |contents| and return them via callback.
  using FindMediaFilesCallback =
      base::OnceCallback<void(base::WeakPtr<content::WebContents>,
                              const std::vector<PlaylistItemInfo>&)>;
  void FindMediaFilesFromContents(content::WebContents* contents,
                                  FindMediaFilesCallback callback);

  void RecoverPlaylistItem(const std::string& id);

  // Add |item_ids| to playlist's item list
  bool AddItemsToPlaylist(const std::string& playlist_id,
                          const std::vector<std::string>& item_ids);

  // Remove a item from a list. When |remove_item| is true, the item preference
  // and local data will be removed together.
  bool RemoveItemFromPlaylist(const PlaylistId& playlist_id,
                              const PlaylistItemId& item_id,
                              bool remove_item = true);
  bool MoveItem(const PlaylistId& from,
                const PlaylistId& to,
                const PlaylistItemId& item);

  // Removes Item value from prefs and related cached data.
  void DeletePlaylistItemData(const std::string& id);
  // Removes only cached data.
  void DeletePlaylistLocalData(const std::string& id);
  void DeleteAllPlaylistItems();

  void AddObserver(PlaylistServiceObserver* observer);
  void RemoveObserver(PlaylistServiceObserver* observer);

  bool GetThumbnailPath(const std::string& id, base::FilePath* thumbnail_path);
  bool GetMediaPath(const std::string& id, base::FilePath* media_path);

  base::FilePath GetPlaylistItemDirPath(const std::string& id) const;

  // Update |web_prefs| if we want for |web_contents|.
  void ConfigureWebPrefsForBackgroundWebContents(
      content::WebContents* web_contents,
      blink::web_pref::WebPreferences* web_prefs);

 private:
  friend class ::CosmeticFilteringPlaylistFlagEnabledTest;
  friend class ::PlaylistRenderFrameObserverBrowserTest;

  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, CreatePlaylist);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, CreatePlaylistItem);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, MediaDownloadFailed);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, ThumbnailFailed);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, MediaRecoverTest);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, DeleteItem);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, RemoveAndRestoreLocalData);

  // KeyedService overrides:
  void Shutdown() override;

  // PlaylistMediaFileDownloadManager::Delegate overrides:
  void OnMediaFileDownloadProgressed(const std::string& id,
                                     int64_t total_bytes,
                                     int64_t received_bytes,
                                     int percent_complete,
                                     base::TimeDelta remaining_time) override;
  void OnMediaFileReady(const std::string& id,
                        const std::string& media_file_path) override;
  void OnMediaFileGenerationFailed(const std::string& id) override;
  bool IsValidPlaylistItem(const std::string& id) override;

  // PlaylistThumbnailDownloader::Delegate overrides:
  // Called when thumbnail image file is downloaded.
  void OnThumbnailDownloaded(const std::string& id,
                             const base::FilePath& path) override;

  bool ShouldDownloadOnBackground(content::WebContents* contents) const;

  void OnPlaylistItemDirCreated(const PlaylistItemInfo& info,
                                bool directory_ready);

  void CreatePlaylistItem(const PlaylistItemInfo& info);
  void DownloadThumbnail(const PlaylistItemInfo& info);
  void DownloadMediaFile(const PlaylistItemInfo& info);

  base::SequencedTaskRunner* task_runner();

  void CleanUpMalformedPlaylistItems();

  // Delete orphaned playlist item directories that are not included in prefs.
  void CleanUpOrphanedPlaylistItemDirs();
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

  content::WebContents* GetBackgroundWebContentsForTesting();

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
