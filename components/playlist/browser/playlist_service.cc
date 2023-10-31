/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_service.h"

#include <algorithm>
#include <utility>

#include "base/check_is_test.h"
#include "base/containers/flat_set.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/token.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_sync_bridge.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/playlist/browser/type_converter.h"
#include "brave/components/playlist/common/features.h"
#include "brave/components/sync/protocol/playlist_specifics.pb.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "net/base/filename_util.h"

namespace playlist {
namespace {

constexpr base::FilePath::StringPieceType kBaseDirName =
    FILE_PATH_LITERAL("playlist");

constexpr base::FilePath::StringPieceType kThumbnailFileName =
    FILE_PATH_LITERAL("thumbnail");

std::vector<base::FilePath> GetOrphanedPaths(
    const base::FilePath& base_dir,
    const base::flat_set<std::string>& ids) {
  std::vector<base::FilePath> orphaned_paths;
  base::FileEnumerator dirs(base_dir, false, base::FileEnumerator::DIRECTORIES);
  for (base::FilePath name = dirs.Next(); !name.empty(); name = dirs.Next()) {
    if (ids.find(name.BaseName().AsUTF8Unsafe()) == ids.end()) {
      orphaned_paths.push_back(name);
    }
  }
  return orphaned_paths;
}

}  // namespace

PlaylistService::PlaylistService(content::BrowserContext* context,
                                 PrefService* local_state,
                                 syncer::OnceModelTypeStoreFactory create_store_callback,
                                 MediaDetectorComponentManager* manager,
                                 std::unique_ptr<Delegate> delegate,
                                 base::Time browser_first_run_time)
    : delegate_(std::move(delegate)),
      base_dir_(context->GetPath().Append(kBaseDirName)),
      playlist_p3a_(local_state, browser_first_run_time),
      prefs_(user_prefs::UserPrefs::Get(context)),
      sync_bridge_(this, std::move(create_store_callback)) {
  media_file_download_manager_ =
      std::make_unique<PlaylistMediaFileDownloadManager>(context, this);
  thumbnail_downloader_ =
      std::make_unique<PlaylistThumbnailDownloader>(context, this);
  download_request_manager_ =
      std::make_unique<PlaylistDownloadRequestManager>(context, manager);

  // This is for cleaning up malformed items during development. Once we
  // release Playlist feature officially, we should migrate items
  // instead of deleting them.
  CleanUpMalformedPlaylistItems();
  MigratePlaylistValues();

  CleanUpOrphanedPlaylistItemDirs();
}

PlaylistService::~PlaylistService() = default;

void PlaylistService::Shutdown() {
  observers_.Clear();
  download_request_manager_.reset();
  media_file_download_manager_.reset();
  thumbnail_downloader_.reset();
  download_request_manager_.reset();
  task_runner_.reset();
#if BUILDFLAG(IS_ANDROID)
  receivers_.Clear();
#endif  // BUILDFLAG(IS_ANDROID)
}

void PlaylistService::AddMediaFilesFromContentsToPlaylist(
    const std::string& playlist_id,
    content::WebContents* contents,
    bool cache) {
  DCHECK(contents);
  if (!contents->GetPrimaryMainFrame()) {
    return;
  }

  VLOG(2) << __func__
          << " download media from WebContents to playlist: " << playlist_id;

  PlaylistDownloadRequestManager::Request request;
  if (ShouldGetMediaFromBackgroundWebContents(contents)) {
    request.url_or_contents = contents->GetVisibleURL().spec();
  } else {
    request.url_or_contents = contents->GetWeakPtr();
  }

  request.should_force_fake_ua = ShouldUseFakeUA(contents->GetVisibleURL());
  request.callback = base::BindOnce(
      &PlaylistService::AddMediaFilesFromItems, weak_factory_.GetWeakPtr(),
      playlist_id.empty() ? GetDefaultSaveTargetListID() : playlist_id, cache,
      base::NullCallback());
  download_request_manager_->GetMediaFilesFromPage(std::move(request));
}

bool PlaylistService::AddItemsToPlaylist(
    const std::string& playlist_id,
    const std::vector<std::string>& item_ids) {
  DCHECK(!playlist_id.empty());

  auto playlist = sync_bridge_.GetPlaylistDetails(playlist_id);
  
  // ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
  // base::Value::Dict* target_playlist = playlists_update->FindDict(playlist_id);
  if (!playlist) {
    LOG(ERROR) << __func__ << " Playlist " << playlist_id << " not found";
    return false;
  }

  // auto playlist = ConvertValueToPlaylist(*target_playlist,
  //                                        prefs_->GetDict(kPlaylistItemsPref));
  for (const auto& new_item_id : item_ids) {
    // We're considering adding item to which it was belong as success.
    if (base::ranges::find_if(playlist->playlist_item_ids(),
                              [&new_item_id](const auto& id) {
                                return id == new_item_id;
                              }) != playlist->playlist_item_ids().end()) {
      continue;
    }

    // Update the item's parent lists.
    auto new_item = sync_bridge_.GetItemDetails(new_item_id);
    if (!new_item) {
      // Ignore items that do not exist
      continue;
    }
    new_item->add_playlist_ids(new_item->id());
    // UpdatePlaylistItemValue(new_item->id,
    //                         base::Value(ConvertPlaylistItemToValue(new_item)));

    playlist->add_playlist_item_ids(new_item->id());

    sync_bridge_.SaveItemDetails(*new_item);
  }

  // playlists_update->Set(playlist_id, ConvertPlaylistToValue(playlist));
  sync_bridge_.SavePlaylistDetails(*playlist);

  for (auto& observer : observers_) {
    for (const auto& item_id : item_ids) {
      observer->OnItemAddedToList(playlist_id, item_id);
    }
  }

  return true;
}

void PlaylistService::CopyItemToPlaylist(
    const std::vector<std::string>& item_ids,
    const std::string& playlist_id) {
  // We don't copy the playlist item deeply and just add item id to playlist.
  AddItemsToPlaylist(playlist_id, item_ids);
}

bool PlaylistService::RemoveItemFromPlaylist(const PlaylistId& playlist_id,
                                             const PlaylistItemId& item_id,
                                             bool delete_item) {
  VLOG(2) << __func__ << " " << *playlist_id << " " << *item_id;

  DCHECK(!item_id->empty());

  // {
    // ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
  auto target_playlist_id =
      playlist_id->empty() ? kDefaultPlaylistID : *playlist_id;
    // base::Value::Dict* playlist_value =
    //     playlists_update->FindDict(target_playlist_id);
  auto playlist = sync_bridge_.GetPlaylistDetails(target_playlist_id);
  if (!playlist) {
    VLOG(2) << __func__ << " Playlist " << playlist_id << " not found";
    return false;
  }

    // auto target_playlist = ConvertValueToPlaylist(
    //     *playlist_value, prefs_->GetDict(kPlaylistItemsPref));
  auto it = base::ranges::find(
      playlist->playlist_item_ids(),
      *item_id);
  // Consider this as success since the item is already removed.
  if (it == playlist->playlist_item_ids().end()) {
    return true;
  }

  playlist->mutable_playlist_item_ids()->erase(it);
  sync_bridge_.SavePlaylistDetails(*playlist);
    // playlists_update->Set(target_playlist_id,
    //                       ConvertPlaylistToValue(target_playlist));
  // }

  // Try to remove |playlist_id| from item->parents or delete the this item
  // if there's no other parent playlist.
  // auto item = GetPlaylistItem(*item_id);
  auto item = sync_bridge_.GetItemDetails(*item_id);
  if (!item) {
    VLOG(2) << __func__ << " Playlist item " << *item_id << " not found";
    return false;
  }
  if (delete_item && item->playlist_ids_size() == 1) {
    DCHECK_EQ(item->playlist_ids().at(0), *playlist_id);
    // DeletePlaylistItemData(item->id);
    ClearItemCache(item->id());
    sync_bridge_.DeleteItemDetails(item->id());
    return true;
  }

  // There're other playlists referencing this. Don't delete item
  // and update the item's parent playlists data.
  auto iter = base::ranges::find(item->playlist_ids(), *playlist_id);
  DCHECK(iter != item->playlist_ids().end());
  item->mutable_playlist_ids()->erase(iter);
  sync_bridge_.SaveItemDetails(*item);
  // UpdatePlaylistItemValue(item->id,
  //                         base::Value(ConvertPlaylistItemToValue(item)));

  for (auto& observer : observers_) {
    observer->OnItemRemovedFromList(*playlist_id, item->id());
  }

  return true;
}

void PlaylistService::ReorderItemFromPlaylist(
    const std::string& playlist_id,
    const std::string& item_id,
    int16_t position,
    ReorderItemFromPlaylistCallback callback) {
  VLOG(2) << __func__ << " " << playlist_id << " " << item_id;

  DCHECK(!item_id.empty());

  auto target_playlist_id =
      playlist_id.empty() ? kDefaultPlaylistID : playlist_id;

  // {
    // ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
    // base::Value::Dict* playlist_value =
    //     playlists_update->FindDict(target_playlist_id);

  auto playlist = sync_bridge_.GetPlaylistDetails(target_playlist_id);
  DCHECK(playlist) << " Playlist " << playlist_id << " not found";

    // auto target_playlist = ConvertValueToPlaylist(
    //     *playlist_value, prefs_->GetDict(kPlaylistItemsPref));
  DCHECK_GT(playlist->playlist_item_ids_size(), position);
  auto* playlist_item_ids = playlist->mutable_playlist_item_ids();
  auto it = base::ranges::find(
      *playlist_item_ids,
      item_id);
  DCHECK(it != playlist->playlist_item_ids().end());

  auto old_position = std::distance(playlist_item_ids->begin(), it);
  if (old_position == position) {
    return;
  }

  playlist_item_ids->SwapElements(old_position, position);

  sync_bridge_.SavePlaylistDetails(*playlist);
  // }

  for (auto& observer : observers_) {
    observer->OnPlaylistUpdated(GetPlaylist(target_playlist_id));
  }

  std::move(callback).Run(true);
}

bool PlaylistService::MoveItem(const PlaylistId& from,
                               const PlaylistId& to,
                               const PlaylistItemId& item) {
  if (!RemoveItemFromPlaylist(from, item, /* delete_item = */ false)) {
    LOG(ERROR) << "Failed to remove item from playlist";
    return false;
  }

  if (!AddItemsToPlaylist(*to, {*item})) {
    LOG(ERROR) << "Failed to add item to playlist";

    // Try to recover.
    AddItemsToPlaylist(*from, {*item});
    return false;
  }
  NotifyPlaylistChanged(mojom::PlaylistEvent::kItemMoved, *from);
  return true;
}

void PlaylistService::AddMediaFilesFromItems(
    const std::string& playlist_id,
    bool cache,
    AddMediaFilesCallback callback,
    std::vector<mojom::PlaylistItemPtr> items) {
  if (items.empty()) {
    if (callback) {
      std::move(callback).Run({});
    }
    return;
  }

  auto target_playlist_id =
      playlist_id.empty() ? GetDefaultSaveTargetListID() : playlist_id;

  base::flat_set<GURL> already_added_media;
  base::ranges::transform(
      GetAllPlaylistItems(),
      std::inserter(already_added_media, already_added_media.end()),
      [](const auto& item) { return item->media_source; });

  std::vector<mojom::PlaylistItemPtr> filtered_items;
  base::ranges::for_each(
      items, [&already_added_media, &filtered_items](auto& item) {
        if (already_added_media.count(item->media_source)) {
          DVLOG(2) << "Skipping creating item: [id] " << item->id
                   << " [media url]:" << item->media_source
                   << " - The media source is already added";
          return;
        }
        filtered_items.push_back(std::move(item));
      });
  if (filtered_items.empty()) {
    if (callback) {
      std::move(callback).Run({});
    }
    return;
  }

  base::ranges::for_each(filtered_items, [this, cache](auto& item) {
    CreatePlaylistItem(item, cache);
  });

  std::vector<std::string> ids;
  base::ranges::transform(filtered_items, std::back_inserter(ids),
                          [](const auto& item) { return item->id; });
  AddItemsToPlaylist(target_playlist_id, ids);

  if (callback) {
    std::move(callback).Run(std::move(filtered_items));
  }
}

void PlaylistService::NotifyPlaylistChanged(mojom::PlaylistEvent playlist_event,
                                            const std::string& playlist_id) {
  VLOG(2) << __func__ << ": params=" << playlist_event;
  for (auto& observer : observers_) {
    observer->OnEvent(playlist_event, playlist_id);
  }
}

void PlaylistService::NotifyMediaFilesUpdated(
    const GURL& url,
    std::vector<mojom::PlaylistItemPtr> items) {
  if (items.empty()) {
    return;
  }

  DVLOG(2) << __FUNCTION__ << " Media files from " << url.spec()
           << " were updated: count =>" << items.size();

  for (auto& observer : observers_) {
    std::vector<mojom::PlaylistItemPtr> cloned_items;
    base::ranges::transform(
        items, std::back_inserter(cloned_items),
        [](const auto& item_ptr) { return item_ptr->Clone(); });
    observer->OnMediaFilesUpdated(url, std::move(cloned_items));
  }
}

void PlaylistService::DownloadMediaFile(const mojom::PlaylistItemPtr& item,
                                        bool update_media_src_and_retry_on_fail,
                                        DownloadMediaFileCallback callback) {
  VLOG(2) << __func__;
  DCHECK(item);

  auto job = std::make_unique<PlaylistMediaFileDownloadManager::DownloadJob>();
  job->item = item.Clone();
  job->on_progress_callback =
      base::BindRepeating(&PlaylistService::OnMediaFileDownloadProgressed,
                          weak_factory_.GetWeakPtr());
  job->on_finish_callback = base::BindOnce(
      &PlaylistService::OnMediaFileDownloadFinished, weak_factory_.GetWeakPtr(),
      update_media_src_and_retry_on_fail, std::move(callback));

  media_file_download_manager_->DownloadMediaFile(std::move(job));
}

base::FilePath PlaylistService::GetPlaylistItemDirPath(
    const std::string& id) const {
  return base_dir_.AppendASCII(id);
}

void PlaylistService::ConfigureWebPrefsForBackgroundWebContents(
    content::WebContents* web_contents,
    blink::web_pref::WebPreferences* web_prefs) {
  download_request_manager_->ConfigureWebPrefsForBackgroundWebContents(
      web_contents, web_prefs);
}

base::WeakPtr<PlaylistService> PlaylistService::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
}

void PlaylistService::FindMediaFilesFromContents(
    content::WebContents* contents,
    FindMediaFilesFromContentsCallback callback) {
  DCHECK(contents);

  PlaylistDownloadRequestManager::Request request;
  auto current_url = contents->GetVisibleURL();
  if (ShouldGetMediaFromBackgroundWebContents(contents)) {
    request.url_or_contents = current_url.spec();
  } else {
    request.url_or_contents = contents->GetWeakPtr();
  }

  request.should_force_fake_ua = ShouldUseFakeUA(contents->GetVisibleURL());
  request.callback = base::BindOnce(std::move(callback), current_url);
  download_request_manager_->GetMediaFilesFromPage(std::move(request));
}

void PlaylistService::GetAllPlaylists(GetAllPlaylistsCallback callback) {
  std::move(callback).Run(GetAllPlaylists());
}

void PlaylistService::GetPlaylist(const std::string& id,
                                  GetPlaylistCallback callback) {
  std::move(callback).Run(GetPlaylist(id));
}

void PlaylistService::GetAllPlaylistItems(
    GetAllPlaylistItemsCallback callback) {
  std::move(callback).Run(GetAllPlaylistItems());
}

std::vector<mojom::PlaylistItemPtr> PlaylistService::GetAllPlaylistItems() {
  std::vector<mojom::PlaylistItemPtr> items;
  for (const auto& item : sync_bridge_.GetAllItemDetails()) {
    items.push_back(ConvertPBToPlaylistItem(item, prefs_->GetDict(kPlaylistCache)));
  }
  return items;
}

void PlaylistService::GetPlaylistItem(const std::string& id,
                                      GetPlaylistItemCallback callback) {
  return std::move(callback).Run(GetPlaylistItem(id));
}

mojom::PlaylistItemPtr PlaylistService::GetPlaylistItem(const std::string& id) {
  DCHECK(!id.empty());
  auto item = sync_bridge_.GetItemDetails(id);
  DCHECK(item);
  if (!item) {
    return {};
  }

  return ConvertPBToPlaylistItem(*item, prefs_->GetDict(kPlaylistCache));
}

mojom::PlaylistPtr PlaylistService::GetPlaylist(const std::string& id) {
  auto playlist = sync_bridge_.GetPlaylistDetails(id);
  auto items = sync_bridge_.GetItemDetailsForPlaylist(id);

  if (!playlist) {
    LOG(ERROR) << __func__ << " playlist with id<" << id << "> not found";
    return {};
  }

  playlist_p3a_.ReportNewUsage();

  return ConvertPBToPlaylist(*playlist, items, prefs_->GetDict(kPlaylistCache));
}

std::vector<mojom::PlaylistPtr> PlaylistService::GetAllPlaylists() {
  std::vector<mojom::PlaylistPtr> playlists;

  auto global_details = sync_bridge_.GetGlobalDetails().value_or(sync_pb::PlaylistGlobalDetails());
  
  for (const auto& id : global_details.ordered_playlist_ids()) {
    playlists.push_back(GetPlaylist(id));
  }

  playlist_p3a_.ReportNewUsage();

  return playlists;
}

bool PlaylistService::HasPlaylistItem(const std::string& id) const {
  return sync_bridge_.HasItemDetails(id);
}

sync::PlaylistSyncBridge* PlaylistService::GetSyncBridge() {
  return &sync_bridge_;
}

void PlaylistService::AddMediaFilesFromPageToPlaylist(
    const std::string& playlist_id,
    const GURL& url,
    bool can_cache) {
  VLOG(2) << __func__ << " " << playlist_id << " " << url;
  PlaylistDownloadRequestManager::Request request;
  request.url_or_contents = url.spec();
  request.callback = base::BindOnce(
      &PlaylistService::AddMediaFilesFromItems, weak_factory_.GetWeakPtr(),
      playlist_id.empty() ? GetDefaultSaveTargetListID() : playlist_id,
      /* cache= */ can_cache &&
          prefs_->GetBoolean(playlist::kPlaylistCacheByDefault),
      base::NullCallback()),
  download_request_manager_->GetMediaFilesFromPage(std::move(request));
}

void PlaylistService::AddMediaFilesFromActiveTabToPlaylist(
    const std::string& playlist_id,
    bool can_cache) {
  DCHECK(delegate_);

  auto* contents = delegate_->GetActiveWebContents();
  if (!contents) {
    return;
  }

  AddMediaFilesFromContentsToPlaylist(
      playlist_id, contents,
      /* cache= */ can_cache && prefs_->GetBoolean(kPlaylistCacheByDefault));
}

void PlaylistService::FindMediaFilesFromActiveTab(
    FindMediaFilesFromActiveTabCallback callback) {
  DCHECK(delegate_);

  auto* contents = delegate_->GetActiveWebContents();
  if (!contents) {
    std::move(callback).Run({}, {});
    return;
  }

  FindMediaFilesFromContents(contents, std::move(callback));
}

void PlaylistService::AddMediaFiles(std::vector<mojom::PlaylistItemPtr> items,
                                    const std::string& playlist_id,
                                    bool can_cache,
                                    AddMediaFilesCallback callback) {
  AddMediaFilesFromItems(
      playlist_id,
      /* cache= */ can_cache &&
          prefs_->GetBoolean(playlist::kPlaylistCacheByDefault),
      std::move(callback), std::move(items));
}

void PlaylistService::RemoveItemFromPlaylist(const std::string& playlist_id,
                                             const std::string& item_id) {
  RemoveItemFromPlaylist(PlaylistId(playlist_id), PlaylistItemId(item_id),
                         /*delete_item=*/true);
}

void PlaylistService::MoveItem(const std::string& from_playlist_id,
                               const std::string& to_playlist_id,
                               const std::string& item_id) {
  MoveItem(PlaylistId(from_playlist_id), PlaylistId(to_playlist_id),
           PlaylistItemId(item_id));
}

void PlaylistService::UpdateItem(mojom::PlaylistItemPtr item) {
  sync_bridge_.SaveItemDetails(ConvertPlaylistItemToPB(item));
  NotifyPlaylistChanged(mojom::PlaylistEvent::kItemUpdated, item->id);
}

void PlaylistService::UpdateItemLastPlayedPosition(
    const std::string& id,
    int32_t last_played_position) {
  if (!HasPlaylistItem(id)) {
    return;
  }

  auto item = GetPlaylistItem(id);
  item->last_played_position = last_played_position;
  UpdateItem(std::move(item));
}

void PlaylistService::CreatePlaylist(mojom::PlaylistPtr playlist,
                                     CreatePlaylistCallback callback) {
  do {
    playlist->id = base::Token::CreateRandom().ToString();
  } while (playlist->id == kDefaultPlaylistID);

  sync_bridge_.SavePlaylistDetails(ConvertPlaylistToPB(playlist));
  auto global_details = sync_bridge_.GetGlobalDetails().value_or(sync_pb::PlaylistGlobalDetails());
  global_details.mutable_ordered_playlist_ids()->Add(std::string(*playlist->id));
  sync_bridge_.SaveGlobalDetails(global_details);

  NotifyPlaylistChanged(mojom::PlaylistEvent::kListCreated,
                        playlist->id.value());

  std::move(callback).Run(playlist.Clone());
}

void PlaylistService::ReorderPlaylist(const std::string& playlist_id,
                                      int16_t position,
                                      ReorderPlaylistCallback callback) {
  // {
  auto global_details = sync_bridge_.GetGlobalDetails().value_or(sync_pb::PlaylistGlobalDetails());
  DCHECK_LT(position, global_details.ordered_playlist_ids().size());

    // ScopedListPrefUpdate playlist_order_update(prefs_, kPlaylistOrderPref);
  auto* playlist_ids = global_details.mutable_ordered_playlist_ids();
  auto it = base::ranges::find(*playlist_ids, playlist_id);
  if (it == playlist_ids->end()) {
    std::move(callback).Run(false);
    return;
  }
  auto old_position = std::distance(playlist_ids->begin(), it);
  if (old_position == position) {
    std::move(callback).Run(true);
    return;
  }

  playlist_ids->SwapElements(old_position, position);
  // }

  sync_bridge_.SaveGlobalDetails(global_details);

  std::move(callback).Run(true);
}

content::WebContents* PlaylistService::GetBackgroundWebContentsForTesting() {
  return download_request_manager_
      ->GetBackgroundWebContentsForTesting();  // IN-TEST
}

std::string PlaylistService::GetDefaultSaveTargetListID() {
  auto id = prefs_->GetString(kPlaylistDefaultSaveTargetListID);
  if (!sync_bridge_.HasPlaylistDetails(id)) {
    prefs_->SetString(kPlaylistDefaultSaveTargetListID, kDefaultPlaylistID);
    id = kDefaultPlaylistID;
  }
  return id;
}

void PlaylistService::CreatePlaylistItem(const mojom::PlaylistItemPtr& item,
                                         bool cache) {
  VLOG(2) << __func__;

  sync_bridge_.SaveItemDetails(ConvertPlaylistItemToPB(item));
  NotifyPlaylistChanged(mojom::PlaylistEvent::kItemAdded, item->id);
  for (auto& observer : observers_) {
    observer->OnItemCreated(item.Clone());
  }

  auto on_dir_created = base::BindOnce(
      [](PlaylistService* service, bool cache, mojom::PlaylistItemPtr item,
         bool dir_created) {
        if (!dir_created) {
          return;
        }

        service->DownloadThumbnail(item);
        if (cache) {
          service->DownloadMediaFile(
              item,
              /*update_media_src_and_retry_on_fail=*/false,
              base::NullCallback());
        }
      },
      this, cache);

  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::CreateDirectory, GetPlaylistItemDirPath(item->id)),
      base::BindOnce(&PlaylistService::OnPlaylistItemDirCreated,
                     weak_factory_.GetWeakPtr(), item.Clone(),
                     std::move(on_dir_created)));

  playlist_p3a_.ReportNewUsage();
}

bool PlaylistService::ShouldGetMediaFromBackgroundWebContents(
    content::WebContents* contents) const {
  if (base::FeatureList::IsEnabled(features::kPlaylistFakeUA)) {
    return true;
  }

  CHECK(contents);
  const auto& url = contents->GetVisibleURL();

  return ShouldUseFakeUA(url) ||
         download_request_manager_->media_detector_component_manager()
             ->ShouldHideMediaSrcAPI(url);
}

bool PlaylistService::ShouldUseFakeUA(const GURL& url) const {
  if (base::FeatureList::IsEnabled(features::kPlaylistFakeUA)) {
    return true;
  }

  return download_request_manager_->media_detector_component_manager()
      ->ShouldUseFakeUA(url);
}

void PlaylistService::OnPlaylistItemDirCreated(
    mojom::PlaylistItemPtr item,
    base::OnceCallback<void(mojom::PlaylistItemPtr, bool)> callback,
    bool directory_ready) {
  VLOG(2) << __func__;
  if (!directory_ready) {
    NotifyPlaylistChanged(mojom::PlaylistEvent::kItemAborted, item->id);
  }

  std::move(callback).Run(std::move(item), directory_ready);
}

void PlaylistService::DownloadThumbnail(const mojom::PlaylistItemPtr& item) {
  VLOG(2) << __func__ << " " << item->thumbnail_source;

  if (item->thumbnail_path != item->thumbnail_source) {
    // Already downloaded.
    return;
  }

  thumbnail_downloader_->DownloadThumbnail(
      item->id, GURL(item->thumbnail_source),
      GetPlaylistItemDirPath(item->id).Append(kThumbnailFileName));
}

void PlaylistService::SanitizeImage(
    std::unique_ptr<std::string> image,
    base::OnceCallback<void(scoped_refptr<base::RefCountedBytes>)> callback) {
  if (!delegate_) {
    CHECK_IS_TEST();
    auto bytes = base::MakeRefCounted<base::RefCountedBytes>(
        reinterpret_cast<const unsigned char*>(image->data()), image->size());
    std::move(callback).Run(bytes);
    return;
  }

  delegate_->SanitizeImage(std::move(image), std::move(callback));
}

void PlaylistService::OnThumbnailDownloaded(const std::string& id,
                                            const base::FilePath& path) {
  DCHECK(IsValidPlaylistItem(id));

  if (path.empty()) {
    VLOG(2) << __func__ << ": thumbnail fetching failed for " << id;
    NotifyPlaylistChanged(mojom::PlaylistEvent::kItemThumbnailFailed, id);
    return;
  }

  UpdateItemCache(id, "file://" + path.AsUTF8Unsafe() /*thumbnail_path=*/,
      {} /*media_path=*/, {} /*cached=*/, {} /*media_file_bytes=*/);
  NotifyPlaylistChanged(mojom::PlaylistEvent::kItemThumbnailReady, id);
}

void PlaylistService::RemovePlaylist(const std::string& playlist_id) {
  if (playlist_id == kDefaultPlaylistID) {
    return;
  }

  DCHECK(!playlist_id.empty());

  // {
    // ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
    // base::Value::Dict* target_playlist =
        // playlists_update->FindDict(playlist_id);
  auto playlist = sync_bridge_.GetPlaylistDetails(playlist_id);
  if (!playlist) {
    LOG(ERROR) << __func__ << " Playlist " << playlist_id << " not found";
    return;
  }

  for (const auto& item_id : playlist->playlist_item_ids()) {
    RemoveItemFromPlaylist(PlaylistId(playlist_id), PlaylistItemId(item_id),
                           /* delete= */ true);
  }

  sync_bridge_.DeletePlaylistDetails(playlist_id);

  auto global_details = sync_bridge_.GetGlobalDetails().value_or(sync_pb::PlaylistGlobalDetails());
  auto* ordered_playlist_ids = global_details.mutable_ordered_playlist_ids();
  auto it = base::ranges::find(*ordered_playlist_ids, playlist_id);
  if (it != ordered_playlist_ids->end()) {
    ordered_playlist_ids->erase(it);
  }
  sync_bridge_.SaveGlobalDetails(global_details);
    // playlists_update->Remove(playlist_id);

    // ScopedListPrefUpdate playlists_order_update(prefs_, kPlaylistOrderPref);
    // playlists_order_update->EraseValue(base::Value(playlist_id));
  // }

  NotifyPlaylistChanged(mojom::PlaylistEvent::kListRemoved, playlist_id);
}

void PlaylistService::ResetAll() {
  // Resets all on-going downloads
  thumbnail_downloader_->CancelAllDownloadRequests();
  media_file_download_manager_->CancelAllDownloadRequests();

  // Resets preference ---------------------------------------------------------
  prefs_->ClearPref(kPlaylistCacheByDefault);
  prefs_->ClearPref(kPlaylistDefaultSaveTargetListID);

  auto items = GetAllPlaylistItems();
  prefs_->ClearPref(kPlaylistItemsPref);
  for (const auto& item : items) {
    for (auto& observer : observers_) {
      observer->OnItemDeleted(item->id);
    }
  }

  prefs_->ClearPref(kPlaylistsPref);
  prefs_->ClearPref(kPlaylistOrderPref);

  sync_bridge_.ResetAll();

  // Removes data on disk
  // ------------------------------------------------------
  GetTaskRunner()->PostTask(FROM_HERE,
                            base::GetDeletePathRecursivelyCallback(base_dir_));
}

void PlaylistService::RenamePlaylist(const std::string& playlist_id,
                                     const std::string& playlist_name,
                                     RenamePlaylistCallback callback) {
  // ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
  auto target_playlist_id =
      playlist_id.empty() ? kDefaultPlaylistID : playlist_id;
  auto playlist = sync_bridge_.GetPlaylistDetails(playlist_id);
  // base::Value::Dict* playlist_value =
      // playlists_update->FindDict(target_playlist_id);
  DCHECK(playlist) << " Playlist " << playlist_id << " not found";

  playlist->set_name(playlist_name);
  // auto target_playlist = ConvertValueToPlaylist(
  //     *playlist_value, prefs_->GetDict(kPlaylistItemsPref));
  sync_bridge_.SavePlaylistDetails(*playlist);

  std::move(callback).Run(GetPlaylist(playlist_id));
}

void PlaylistService::RecoverLocalDataForItem(
    const std::string& id,
    bool update_media_src_before_recovery,
    RecoverLocalDataForItemCallback callback) {
  auto item = GetPlaylistItem(id);
  if (!item) {
    LOG(ERROR) << __func__ << ": Invalid playlist item id for recovery: " << id;
    if (callback) {
      std::move(callback).Run({});
    }
    return;
  }

  if (!update_media_src_before_recovery) {
    RecoverLocalDataForItemImpl(std::move(item),
                                /*update_media_src_and_retry_on_fail=*/true,
                                std::move(callback));
    return;
  }

  // Before recovering data, try to update item's media source by visiting the
  // original page first.
  auto update_media_src_and_recover = base::BindOnce(
      [](base::WeakPtr<PlaylistService> service,
         mojom::PlaylistItemPtr old_item,
         RecoverLocalDataForItemCallback callback,
         std::vector<mojom::PlaylistItemPtr> found_items) {
        if (!service) {
          return;
        }

        DCHECK(old_item);
        if (found_items.empty()) {
          // In this case, just try recovering with existing data.
          service->RecoverLocalDataForItemImpl(
              std::move(old_item),
              /*update_media_src_and_retry_on_fail=*/false,
              std::move(callback));
          return;
        }

#if DCHECK_IS_ON()
        if (found_items.size() > 1u) {
          DLOG(ERROR)
              << "We don't expect this as we can't decide which one can "
                 "replace the existing one.";
        }
#endif  // DCHECK_IS_ON()

        // The item's other data could have been updated.
        mojom::PlaylistItemPtr new_item =
            service->GetPlaylistItem(old_item->id);
        DCHECK(new_item);
        DCHECK(!new_item->cached);
        DCHECK_EQ(new_item->media_source, old_item->media_source);
        DCHECK_EQ(new_item->media_path, old_item->media_path);
        new_item->media_source = found_items.front()->media_source;
        service->sync_bridge_.SaveItemDetails(ConvertPlaylistItemToPB(new_item));

        service->RecoverLocalDataForItemImpl(
            std::move(new_item),
            /*update_media_src_and_retry_on_fail=*/false, std::move(callback));
      },
      weak_factory_.GetWeakPtr(), item->Clone(), std::move(callback));

  PlaylistDownloadRequestManager::Request request;
  DCHECK(!item->page_source.spec().empty());
  request.url_or_contents = item->page_source.spec();
  request.should_force_fake_ua = ShouldUseFakeUA(item->page_source);
  request.callback = std::move(update_media_src_and_recover);
  download_request_manager_->GetMediaFilesFromPage(std::move(request));
}

void PlaylistService::RemoveLocalDataForItemsInPlaylist(
    const std::string& playlist_id) {
  auto items = sync_bridge_.GetItemDetailsForPlaylist(playlist_id);

  for (const auto& item : items) {
    RemoveLocalDataForItemImpl(ConvertPBToPlaylistItem(item, prefs_->GetDict(kPlaylistCache)));
  }
}

void PlaylistService::DeletePlaylistItemData(const std::string& id) {
  media_file_download_manager_->CancelDownloadRequest(id);
  thumbnail_downloader_->CancelDownloadRequest(id);

  sync_bridge_.DeleteItemDetails(id);
  NotifyPlaylistChanged(mojom::PlaylistEvent::kItemDeleted, id);
  for (auto& observer : observers_) {
    observer->OnItemDeleted(id);
  }

  // TODO(simonhong): Delete after getting cancel complete message from all
  // downloader.
  // Delete assets from filesystem after updating db.
  GetTaskRunner()->PostTask(FROM_HERE, base::GetDeletePathRecursivelyCallback(
                                           GetPlaylistItemDirPath(id)));
}

void PlaylistService::RemoveLocalDataForItem(const std::string& id) {
  auto item = GetPlaylistItem(id);
  DCHECK(item);
  RemoveLocalDataForItemImpl(item);
}

void PlaylistService::DeleteAllPlaylistItems() {
  VLOG(2) << __func__;

  // Cancel currently generated playlist if needed and pending thumbnail
  // download jobs.
  media_file_download_manager_->CancelAllDownloadRequests();
  thumbnail_downloader_->CancelAllDownloadRequests();

  auto items = sync_bridge_.GetAllItemDetails();
  for (const auto& item : items) {
    sync_bridge_.DeleteItemDetails(item.id());
  }
  NotifyPlaylistChanged(mojom::PlaylistEvent::kAllDeleted, "");

  CleanUpOrphanedPlaylistItemDirs();
}

void PlaylistService::RecoverLocalDataForItemImpl(
    const mojom::PlaylistItemPtr& item,
    bool update_media_src_and_retry_on_fail,
    RecoverLocalDataForItemCallback callback) {
  DCHECK(!item->id.empty());

  if (item->cached) {
    VLOG(2) << __func__ << ": This is ready to play(" << item->id << ")";
    if (callback) {
      std::move(callback).Run(item.Clone());
    }
    return;
  }

  auto make_sure_path_exists = [](base::FilePath path) {
    if (base::PathExists(path)) {
      return true;
    }

    return base::CreateDirectory(path);
  };

  auto on_dir_exists = base::BindOnce(
      [](PlaylistService* service, bool update_media_src_and_retry_on_fail,
         DownloadMediaFileCallback callback, mojom::PlaylistItemPtr item,
         bool dir_created) {
        if (!dir_created) {
          // When failed, invoke callback without changing any path.
          std::move(callback).Run(std::move(item));
          return;
        }

        service->DownloadThumbnail(item);
        service->DownloadMediaFile(item, update_media_src_and_retry_on_fail,
                                   std::move(callback));
      },
      this, update_media_src_and_retry_on_fail, std::move(callback));

  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(make_sure_path_exists, GetPlaylistItemDirPath(item->id)),
      base::BindOnce(&PlaylistService::OnPlaylistItemDirCreated,
                     weak_factory_.GetWeakPtr(), item->Clone(),
                     std::move(on_dir_exists)));
}

void PlaylistService::RemoveLocalDataForItemImpl(
    const mojom::PlaylistItemPtr& item) {
  DCHECK(item);
  if (!item->cached) {
    return;
  }

  base::FilePath media_path;
  CHECK(net::FileURLToFilePath(item->media_path, &media_path));

  ClearItemCache(item->id);
  NotifyPlaylistChanged(mojom::PlaylistEvent::kItemLocalDataRemoved, item->id);

  auto delete_file = base::BindOnce(
      [](const base::FilePath& path) { base::DeleteFile(path); }, media_path);
  GetTaskRunner()->PostTask(FROM_HERE, std::move(delete_file));
}

void PlaylistService::OnMediaFileDownloadFinished(
    bool update_media_src_and_retry_on_fail,
    DownloadMediaFileCallback callback,
    mojom::PlaylistItemPtr item,
    const base::expected<
        PlaylistMediaFileDownloadManager::DownloadResult,
        PlaylistMediaFileDownloadManager::DownloadFailureReason>& result) {
  DCHECK(item);
  DCHECK(IsValidPlaylistItem(item->id));

  if (!result.has_value() &&
      result.error() !=
          PlaylistMediaFileDownloadManager::DownloadFailureReason::kCanceled &&
      update_media_src_and_retry_on_fail) {
    VLOG(2) << __func__ << ": downloading " << item->id << " from "
            << item->media_source.spec()
            << " failed. Try updating media src and download";
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(&PlaylistService::RecoverLocalDataForItem,
                                  weak_factory_.GetWeakPtr(), item->id,
                                  /*update_media_src_before_recovery=*/true,
                                  std::move(callback)));
    return;
  }

  auto media_file_path =
      result.has_value() ? result.value().media_file_path : std::string();
  auto received_bytes = result.has_value() ? result.value().received_bytes : 0;

  VLOG(2) << __func__ << ": " << item->id << " result path" << media_file_path;

  bool cached = !media_file_path.empty();
  absl::optional<std::string> media_path;
  absl::optional<uint64_t> media_file_bytes;
  if (cached) {
    media_path = "file://" + media_file_path;
    if (received_bytes) {
      media_file_bytes = received_bytes;
    }
  }
  UpdateItemCache(item->id, {} /*thumbnail_path=*/, media_path, cached, media_file_bytes);
  NotifyPlaylistChanged(cached ? mojom::PlaylistEvent::kItemCached
                                 : mojom::PlaylistEvent::kItemAborted,
                        item->id);

  if (callback) {
    std::move(callback).Run(item.Clone());
  }
}

void PlaylistService::UpdateItemCache(const std::string& item_id,
                                     const absl::optional<std::string> thumbnail_path,
                                     const absl::optional<std::string> media_path,
                                     const absl::optional<bool> cached,
                                     const absl::optional<uint64_t> media_file_bytes) {
  ScopedDictPrefUpdate update(prefs_, kPlaylistCache);
  auto* update_dict = update->EnsureDict(item_id);
  if (thumbnail_path) {
    update_dict->Set(kPlaylistCacheThumbnailPathKey, *thumbnail_path);
  }
  if (media_path) {
    update_dict->Set(kPlaylistCacheMediaPathKey, *media_path);
  }
  if (cached) {
    update_dict->Set(kPlaylistCacheCachedKey, *cached);
  }
  if (media_file_bytes) {
    update_dict->Set(kPlaylistCacheMediaFileBytesKey, static_cast<double>(*media_file_bytes));
  }
}

void PlaylistService::ClearItemCache(const std::string& item_id) {
  ScopedDictPrefUpdate update(prefs_, kPlaylistCache);
  update->Remove(item_id);
}

#if BUILDFLAG(IS_ANDROID)
mojo::PendingRemote<mojom::PlaylistService> PlaylistService::MakeRemote() {
  mojo::PendingRemote<mojom::PlaylistService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}
#endif  // BUILDFLAG(IS_ANDROID)

void PlaylistService::AddObserver(
    mojo::PendingRemote<mojom::PlaylistServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

void PlaylistService::OnMediaUpdatedFromContents(
    content::WebContents* contents) {
  if (download_request_manager_->background_contents() != contents) {
    return;
  }

  // Try getting media files that are added dynamically after we've tried.
  PlaylistDownloadRequestManager::Request request;
  request.url_or_contents = contents->GetWeakPtr();
  request.callback =
      base::BindOnce(&PlaylistService::NotifyMediaFilesUpdated,
                     weak_factory_.GetWeakPtr(), contents->GetVisibleURL());
  download_request_manager_->GetMediaFilesFromPage(std::move(request));
}

void PlaylistService::OnMediaFileDownloadProgressed(
    const mojom::PlaylistItemPtr& item,
    int64_t total_bytes,
    int64_t received_bytes,
    int percent_complete,
    base::TimeDelta time_remaining) {
  DCHECK(item);
  VLOG(2) << __func__ << " " << total_bytes << " " << received_bytes << " "
          << percent_complete << " " << time_remaining;

  for (auto& observer : observers_) {
    observer->OnMediaFileDownloadProgressed(
        item->id, total_bytes, received_bytes, percent_complete,
        base::TimeDeltaToValue(time_remaining).GetString());
  }
}

bool PlaylistService::IsValidPlaylistItem(const std::string& id) {
  return sync_bridge_.HasItemDetails(id);
}

base::FilePath PlaylistService::GetMediaPathForPlaylistItemItem(
    const std::string& id) {
  base::FilePath path;
  CHECK(GetMediaPath(id, &path));
  return path;
}

void PlaylistService::OnGetOrphanedPaths(
    const std::vector<base::FilePath> orphaned_paths) {
  if (orphaned_paths.empty()) {
    VLOG(2) << __func__ << ": No orphaned playlist";
    return;
  }

  for (const auto& path : orphaned_paths) {
    VLOG(2) << __func__ << ": " << path << " is orphaned";
    GetTaskRunner()->PostTask(FROM_HERE,
                              base::GetDeletePathRecursivelyCallback(path));
  }
}

void PlaylistService::CleanUpMalformedPlaylistItems() {
  if (base::ranges::none_of(prefs_->GetDict(kPlaylistItemsPref),
                            /* has_malformed_data = */ [](const auto& pair) {
                              auto* dict = pair.second.GetIfDict();
                              DCHECK(dict);
                              return IsItemValueMalformed(*dict);
                            })) {
    return;
  }

  for (const auto* pref_key : {kPlaylistsPref, kPlaylistItemsPref}) {
    prefs_->ClearPref(pref_key);
  }
}

void PlaylistService::MigratePlaylistValues() {
  base::Value::List order = prefs_->GetList(kPlaylistOrderPref).Clone();
  MigratePlaylistOrder(prefs_->GetDict(kPlaylistsPref), order);
  prefs_->SetList(kPlaylistOrderPref, std::move(order));
}

void PlaylistService::CleanUpOrphanedPlaylistItemDirs() {
  GetAllPlaylistItems(base::BindOnce(
      [](base::WeakPtr<PlaylistService> service,
         std::vector<mojom::PlaylistItemPtr> items) {
        base::flat_set<std::string> ids;
        base::ranges::transform(items, std::inserter(ids, ids.end()),
                                [](const auto& item) {
                                  DCHECK(!item->id.empty());
                                  return item->id;
                                });

        service->GetTaskRunner()->PostTaskAndReplyWithResult(
            FROM_HERE,
            base::BindOnce(&GetOrphanedPaths, service->base_dir_,
                           std::move(ids)),
            base::BindOnce(&PlaylistService::OnGetOrphanedPaths, service));
      },
      weak_factory_.GetWeakPtr()));
}

bool PlaylistService::GetThumbnailPath(const std::string& id,
                                       base::FilePath* thumbnail_path) {
  *thumbnail_path = GetPlaylistItemDirPath(id).Append(kThumbnailFileName);
  if (thumbnail_path->ReferencesParent()) {
    thumbnail_path->clear();
    return false;
  }
  return true;
}

void PlaylistService::DownloadThumbnail(
    const GURL& url,
    base::OnceCallback<void(gfx::Image)> callback) {
  thumbnail_downloader_->DownloadThumbnail(url.spec(), url,
                                           std::move(callback));
}

bool PlaylistService::GetMediaPath(const std::string& id,
                                   base::FilePath* media_path) {
  constexpr base::FilePath::CharType kMediaFileName[] =
      FILE_PATH_LITERAL("media_file");
  CHECK(media_path);
  *media_path = GetPlaylistItemDirPath(id).Append(kMediaFileName);

  if (HasPlaylistItem(id)) {
    // Try to infer file extension from the source URL.
    auto item = GetPlaylistItem(id);
    GURL url(item->media_source);
    auto path = url.path();
    std::string extension;
    if (!path.empty()) {
      auto parts = base::SplitString(path, ".", base::TRIM_WHITESPACE,
                                     base::SPLIT_WANT_NONEMPTY);
      if (parts.size() > 1) {
        extension = parts.back();
      }
    }

    if (!extension.empty()) {
      *media_path = media_path->AddExtensionASCII(extension);
    }
  }

  DCHECK(!media_path->empty());

  if (media_path->ReferencesParent()) {
    media_path->clear();
    return false;
  }
  return true;
}

void PlaylistService::OnDataReady() {
  // TODO: implement this  
}

base::SequencedTaskRunner* PlaylistService::GetTaskRunner() {
  if (!task_runner_) {
    task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return task_runner_.get();
}

}  // namespace playlist
