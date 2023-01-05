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
#include "brave/components/playlist/mojom/playlist.mojom.h"
#include "brave/components/playlist/playlist_download_request_manager.h"
#include "brave/components/playlist/playlist_media_file_download_manager.h"
#include "brave/components/playlist/playlist_thumbnail_downloader.h"
#include "brave/components/playlist/playlist_types.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/remote_set.h"

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
                        public PlaylistThumbnailDownloader::Delegate,
                        public mojom::PlaylistService {
 public:
  class Delegate {
   public:
    Delegate() = default;
    Delegate(const Delegate&) = delete;
    Delegate& operator=(const Delegate&) = delete;
    virtual ~Delegate() = default;

    virtual content::WebContents* GetWebContents(int32_t window_id,
                                                 int32_t tab_id) = 0;
  };

  using PlaylistId = base::StrongAlias<class PlaylistIdTag, std::string>;
  using PlaylistItemId =
      base::StrongAlias<class PlaylistItemIdTag, std::string>;

  PlaylistService(content::BrowserContext* context,
                  MediaDetectorComponentManager* manager,
                  std::unique_ptr<Delegate> delegate);
  ~PlaylistService() override;
  PlaylistService(const PlaylistService&) = delete;
  PlaylistService& operator=(const PlaylistService&) = delete;

  void AddObserver(
      mojo::PendingRemote<mojom::PlaylistServiceObserver> observer);

  bool GetThumbnailPath(const std::string& id, base::FilePath* thumbnail_path);
  bool GetMediaPath(const std::string& id, base::FilePath* media_path);

  base::FilePath GetPlaylistItemDirPath(const std::string& id) const;

  // Update |web_prefs| if we want for |web_contents|.
  void ConfigureWebPrefsForBackgroundWebContents(
      content::WebContents* web_contents,
      blink::web_pref::WebPreferences* web_prefs);

  // mojom::Service:
  void GetAllPlaylists(GetAllPlaylistsCallback callback) override;
  void GetPlaylist(const std::string& id,
                   GetPlaylistCallback callback) override;
  void GetAllPlaylistItems(GetAllPlaylistItemsCallback callback) override;
  void GetPlaylistItem(const std::string& id,
                       GetPlaylistItemCallback callback) override;

  void AddMediaFilesFromPageToPlaylist(const std::string& playlist_id,
                                       const GURL& url) override;
  void AddMediaFilesFromTabToPlaylist(int32_t window_id,
                                      int32_t tab_id,
                                      const std::string& playlist_id) override;
  void FindMediaFilesFromTab(int32_t window_id,
                             int32_t tab_id,
                             FindMediaFilesFromTabCallback callback) override;
  void AddMediaFiles(std::vector<mojom::PlaylistItemPtr> items,
                     const std::string& playlist_id) override;
  void RemoveItemFromPlaylist(const std::string& playlist_id,
                              const std::string& item_id) override;
  void MoveItem(const std::string& from_playlist_id,
                const std::string& to_playlist_id,
                const std::string& item_id) override;
  void UpdateItem(mojom::PlaylistItemPtr item) override;
  void RecoverLocalDataForItem(const std::string& item_id) override;
  void RemoveLocalDataForItem(const std::string& item_id) override;

  void CreatePlaylist(mojom::PlaylistPtr playlist,
                      CreatePlaylistCallback callback) override;
  void RemovePlaylist(const std::string& playlist_id) override;

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
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, CachingBehavior);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, DefaultSaveTargetListID);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, AddItemsToList);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, MoveItem);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, DefaultSaveTargetListID);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, UpdateItem);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, CreateAndRemovePlaylist);

  void AddObserverForTest(PlaylistServiceObserver* observer);
  void RemoveObserverForTest(PlaylistServiceObserver* observer);

  // KeyedService overrides:
  void Shutdown() override;

  // Finds media files from |contents| or |url| and adds them to given
  // |playlist_id|.
  void AddMediaFilesFromContentsToPlaylist(const std::string& playlist_id,
                                           content::WebContents* contents,
                                           bool cache);

  void AddMediaFilesFromItems(const std::string& playlist_id,
                              bool cache,
                              std::vector<mojom::PlaylistItemPtr> items);

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

  void OnPlaylistItemDirCreated(mojom::PlaylistItemPtr item,
                                bool cache,
                                bool directory_ready);

  void CreatePlaylistItem(const mojom::PlaylistItemPtr& item, bool cache);
  void DownloadThumbnail(const mojom::PlaylistItemPtr& item);
  void DownloadMediaFile(const mojom::PlaylistItemPtr& item);

  base::SequencedTaskRunner* GetTaskRunner();

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

  std::string GetDefaultSaveTargetListID();

  // Remove a item from a list. When |remove_item| is true, the item preference
  // and local data will be removed together.
  bool RemoveItemFromPlaylist(const PlaylistId& playlist_id,
                              const PlaylistItemId& item_id,
                              bool remove_item = true);

  bool MoveItem(const PlaylistId& from,
                const PlaylistId& to,
                const PlaylistItemId& item);

  // Add |item_ids| to playlist's item list
  bool AddItemsToPlaylist(const std::string& playlist_id,
                          const std::vector<std::string>& item_ids);

  // Removes Item value from prefs and related cached data.
  void DeletePlaylistItemData(const std::string& id);
  void DeleteAllPlaylistItems();

  std::unique_ptr<Delegate> delegate_;

  const base::FilePath base_dir_;
  base::ObserverList<PlaylistServiceObserver> observers_;

  mojo::RemoteSet<mojom::PlaylistServiceObserver> service_observers_;

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
