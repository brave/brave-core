/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_SERVICE_H_
#define BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/gtest_prod_util.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/playlist/browser/playlist_media_file_download_manager.h"
#include "brave/components/playlist/browser/playlist_p3a.h"
#include "brave/components/playlist/browser/playlist_streaming.h"
#include "brave/components/playlist/browser/playlist_thumbnail_downloader.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_member.h"
#include "mojo/public/cpp/bindings/remote_set.h"

#if BUILDFLAG(IS_ANDROID)
#include "mojo/public/cpp/bindings/receiver_set.h"
#endif  // BUILDFLAG(IS_ANDROID)

class GURL;

namespace base {
class SequencedTaskRunner;
}  // namespace base

namespace content {
class BrowserContext;
class WebContents;
}  // namespace content

class CosmeticFilteringPlaylistFlagEnabledTest;
class PlaylistMediaDiscoveryBrowserTest;
class PrefService;

namespace gfx {
class Image;
}  // namespace gfx

namespace playlist {

class MediaDetectorComponentManager;
class PlaylistBackgroundWebContentses;

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

    virtual void SanitizeImage(
        std::unique_ptr<std::string> image,
        base::OnceCallback<void(scoped_refptr<base::RefCountedBytes>)>
            callback) = 0;

    virtual content::WebContents* GetActiveWebContents() = 0;

    virtual void EnabledStateChanged(bool enabled) = 0;
  };

  using PlaylistId = base::StrongAlias<class PlaylistIdTag, std::string>;
  using PlaylistItemId =
      base::StrongAlias<class PlaylistItemIdTag, std::string>;

  PlaylistService(content::BrowserContext* context,
                  PrefService* local_state,
                  MediaDetectorComponentManager* manager,
                  std::unique_ptr<Delegate> delegate,
                  base::Time browser_first_run_time);
  ~PlaylistService() override;
  PlaylistService(const PlaylistService&) = delete;
  PlaylistService& operator=(const PlaylistService&) = delete;

#if BUILDFLAG(IS_ANDROID)
  mojo::PendingRemote<mojom::PlaylistService> MakeRemote();
#endif  // BUILDFLAG(IS_ANDROID)

  bool GetThumbnailPath(const std::string& id, base::FilePath* thumbnail_path);

  void DownloadThumbnail(const GURL& url,
                         base::OnceCallback<void(gfx::Image)> callback);

  // This returns candidate path, which possibly doesn't have file extension.
  // Don't depend on this method to access the actual cache on the local disk.
  bool GetMediaPath(const std::string& id, base::FilePath* media_path);

  base::FilePath GetPlaylistItemDirPath(const std::string& id) const;

  base::WeakPtr<PlaylistService> GetWeakPtr();

  // Synchronous versions of mojom::PlaylistService implementations
  std::vector<mojom::PlaylistItemPtr> GetAllPlaylistItems();
  mojom::PlaylistItemPtr GetPlaylistItem(const std::string& id);
  mojom::PlaylistPtr GetPlaylist(const std::string& id);
  std::vector<mojom::PlaylistPtr> GetAllPlaylists();

  // mojom::PlaylistService:
  // TODO(sko) Make getters without callbacks and simplify codes in
  // PlaylistService and tests.
  void GetAllPlaylists(GetAllPlaylistsCallback callback) override;
  void GetPlaylist(const std::string& id,
                   GetPlaylistCallback callback) override;
  void GetAllPlaylistItems(GetAllPlaylistItemsCallback callback) override;
  void GetPlaylistItem(const std::string& id,
                       GetPlaylistItemCallback callback) override;

  void AddMediaFilesFromActiveTabToPlaylist(
      const std::string& playlist_id,
      bool can_cache,
      AddMediaFilesFromActiveTabToPlaylistCallback callback) override;
  void FindMediaFilesFromActiveTab() override;
  void AddMediaFiles(std::vector<mojom::PlaylistItemPtr> items,
                     const std::string& playlist_id,
                     bool can_cache,
                     AddMediaFilesCallback callback) override;
  void CopyItemToPlaylist(const std::vector<std::string>& item_ids,
                          const std::string& playlist_id) override;
  void RemoveItemFromPlaylist(const std::string& playlist_id,
                              const std::string& item_id) override;
  void MoveItem(const std::string& from_playlist_id,
                const std::string& to_playlist_id,
                const std::string& item_id) override;
  void ReorderItemFromPlaylist(
      const std::string& playlist_id,
      const std::string& item_id,
      int16_t position,
      ReorderItemFromPlaylistCallback callback) override;
  void UpdateItem(mojom::PlaylistItemPtr item) override;
  void UpdateItemLastPlayedPosition(const std::string& playlist_item_id,
                                    int32_t last_played_position) override;
  void UpdateItemHlsMediaFilePath(const std::string& playlist_item_id,
                                  const std::string& hls_media_file_path,
                                  int64_t updated_file_size) override;
  void RecoverLocalDataForItem(
      const std::string& item_id,
      bool update_media_src_before_recovery,
      RecoverLocalDataForItemCallback callback) override;
  void RemoveLocalDataForItem(const std::string& item_id) override;
  void RemoveLocalDataForItemsInPlaylist(
      const std::string& playlist_id) override;

  void CreatePlaylist(mojom::PlaylistPtr playlist,
                      CreatePlaylistCallback callback) override;
  void ReorderPlaylist(const std::string& playlist_id,
                       int16_t position,
                       ReorderPlaylistCallback callback) override;
  void RemovePlaylist(const std::string& playlist_id) override;
  void RenamePlaylist(const std::string& playlist_id,
                      const std::string& playlist_name,
                      RenamePlaylistCallback callback) override;

  void ResetAll() override;

  // Please note that mojo-based observer mechanism is asynchronous, which means
  // it could be notified much later than callback from each operation.
  void AddObserver(
      mojo::PendingRemote<mojom::PlaylistServiceObserver> observer) override;

  void OnMediaDetected(GURL url, std::vector<mojom::PlaylistItemPtr> items);

  bool HasPlaylistItem(const std::string& id) const;

  void RequestStreamingQuery(
      const std::string& query_id,
      const std::string& url,
      const std::string& methodm,
      mojo::PendingRemote<mojom::PlaylistStreamingObserver> observer) override;
  void ClearAllQueries() override;
  void CancelQuery(const std::string& query_id) override;
  void OnResponseStarted(const std::string& url, const int64_t content_length);
  void OnDataReceived(api_request_helper::ValueOrError result);
  void OnDataComplete(api_request_helper::APIRequestResult result);

  bool playlist_enabled() const { return *enabled_pref_; }

  const std::string& GetMediaSourceAPISuppressorScript() const;
  std::string GetMediaDetectorScript(const GURL& url) const;

 private:
  friend class ::CosmeticFilteringPlaylistFlagEnabledTest;
  friend class PlaylistBrowserTest;
  friend class ::PlaylistMediaDiscoveryBrowserTest;

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
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, ReorderItemFromPlaylist);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, RemoveItemFromPlaylist);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, ResetAll);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest,
                           CleanUpOrphanedPlaylistItemDirs);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceUnitTest, MediaFileExtension);
  FRIEND_TEST_ALL_PREFIXES(PlaylistServiceWithFakeUAUnitTest,
                           ShouldAlwaysGetMediaFromBackgroundWebContents);

  void SetUpForTesting() const;

  // Finds media files from |contents| or |url| and adds them to given
  // |playlist_id|.
  void AddMediaFilesFromContentsToPlaylist(
      const std::string& playlist_id,
      content::WebContents* contents,
      bool cache,
      base::OnceCallback<void(std::vector<mojom::PlaylistItemPtr>)> callback);

  void AddMediaFilesFromItems(const std::string& playlist_id,
                              bool cache,
                              AddMediaFilesCallback callback,
                              std::vector<mojom::PlaylistItemPtr> items);

  void CreatePlaylistItem(const mojom::PlaylistItemPtr& item, bool cache);
  void DownloadThumbnail(const mojom::PlaylistItemPtr& item);

  using DownloadMediaFileCallback =
      base::OnceCallback<void(mojom::PlaylistItemPtr)>;
  void DownloadMediaFile(const mojom::PlaylistItemPtr& item,
                         bool update_media_src_and_retry_on_fail,
                         DownloadMediaFileCallback callback);

  void MigratePlaylistValues();

  // Delete orphaned playlist item directories that are not included in prefs.
  void CleanUpOrphanedPlaylistItemDirs();
  void OnGetOrphanedPaths(const std::vector<base::FilePath>& paths);

  // TODO(sko) Remove this version
  // https://github.com/brave/brave-browser/issues/30735
  void NotifyPlaylistChanged(mojom::PlaylistEvent playlist_event,
                             const std::string& playlist_id);

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

  std::string GetDefaultSaveTargetListID();

  // Remove a item from a list. When |delete_item| is true, the item preference
  // and local data will be removed if there's no other playlists referencing
  // the item.
  bool RemoveItemFromPlaylist(const PlaylistId& playlist_id,
                              const PlaylistItemId& item_id,
                              bool delete_item);

  bool MoveItem(const PlaylistId& from,
                const PlaylistId& to,
                const PlaylistItemId& item);

  // Add |item_ids| to playlist's item list
  bool AddItemsToPlaylist(const std::string& playlist_id,
                          const std::vector<std::string>& item_ids);

  // Removes Item value from prefs and related cached data.
  void DeletePlaylistItemData(const std::string& id);
  void DeleteAllPlaylistItems();

  void RecoverLocalDataForItemImpl(const mojom::PlaylistItemPtr& item,
                                   bool update_media_src_and_retry_on_fail,
                                   RecoverLocalDataForItemCallback callback);
  void RemoveLocalDataForItemImpl(const mojom::PlaylistItemPtr& item);

  void OnPlaylistItemDirCreated(
      mojom::PlaylistItemPtr item,
      base::OnceCallback<void(mojom::PlaylistItemPtr, bool)> callback,
      bool directory_ready);

  void OnMediaFileDownloadProgressed(const mojom::PlaylistItemPtr& item,
                                     int64_t total_bytes,
                                     int64_t received_bytes,
                                     int percent_complete,
                                     base::TimeDelta remaining_time);
  void OnMediaFileDownloadFinished(
      bool update_media_src_and_retry_on_fail,
      DownloadMediaFileCallback callback,
      mojom::PlaylistItemPtr item,
      const base::expected<
          PlaylistMediaFileDownloadManager::DownloadResult,
          PlaylistMediaFileDownloadManager::DownloadFailureReason>& result);

  void OnEnabledPrefChanged();

  // KeyedService overrides:
  void Shutdown() override;

  // PlaylistMediaFileDownloadManager::Delegate
  bool IsValidPlaylistItem(const std::string& id) override;
  base::FilePath GetMediaPathForPlaylistItemItem(
      const std::string& id) override;
  base::SequencedTaskRunner* GetTaskRunner() override;

  // PlaylistThumbnailDownloader::Delegate overrides:
  void SanitizeImage(
      std::unique_ptr<std::string> image,
      base::OnceCallback<void(scoped_refptr<base::RefCountedBytes>)> callback)
      override;
  // Called when thumbnail image file is downloaded.
  void OnThumbnailDownloaded(const std::string& id,
                             const base::FilePath& path) override;

  std::unique_ptr<Delegate> delegate_;

  const base::FilePath base_dir_;

  mojo::RemoteSet<mojom::PlaylistServiceObserver> observers_;
  mojo::Remote<mojom::PlaylistStreamingObserver> streaming_observer_;

  std::unique_ptr<PlaylistMediaFileDownloadManager>
      media_file_download_manager_;
  std::unique_ptr<PlaylistThumbnailDownloader> thumbnail_downloader_;

  std::unique_ptr<PlaylistStreaming> playlist_streaming_;

  std::unique_ptr<PlaylistBackgroundWebContentses> background_web_contentses_;

  raw_ptr<MediaDetectorComponentManager> media_detector_component_manager_;

  PlaylistP3A playlist_p3a_;

  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  raw_ptr<PrefService> prefs_ = nullptr;

  BooleanPrefMember enabled_pref_;

#if BUILDFLAG(IS_ANDROID)
  mojo::ReceiverSet<mojom::PlaylistService> receivers_;
#endif  // BUILDFLAG(IS_ANDROID)

  base::WeakPtrFactory<PlaylistService> weak_factory_{this};
};

}  // namespace playlist

#endif  // BRAVE_COMPONENTS_PLAYLIST_BROWSER_PLAYLIST_SERVICE_H_
