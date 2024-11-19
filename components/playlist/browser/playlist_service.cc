
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
#include "base/functional/callback_helpers.h"
#include "base/json/values_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "brave/components/playlist/browser/media_detector_component_manager.h"
#include "brave/components/playlist/browser/playlist_background_web_contentses.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_tab_helper.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/playlist/browser/type_converter.h"
#include "brave/components/playlist/common/features.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/web_contents.h"
#include "net/base/filename_util.h"
#include "url/gurl.h"

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
                                 MediaDetectorComponentManager* manager,
                                 std::unique_ptr<Delegate> delegate,
                                 base::Time browser_first_run_time)
    : delegate_(std::move(delegate)),
      base_dir_(context->GetPath().Append(kBaseDirName)),
      playlist_p3a_(local_state, browser_first_run_time),
      prefs_(user_prefs::UserPrefs::Get(context)) {
  CHECK(base::FeatureList::IsEnabled(features::kPlaylist));

  media_file_download_manager_ =
      std::make_unique<PlaylistMediaFileDownloadManager>(context, this);
  thumbnail_downloader_ =
      std::make_unique<PlaylistThumbnailDownloader>(context, this);
  background_web_contentses_ =
      std::make_unique<PlaylistBackgroundWebContentses>(context, this);
  playlist_streaming_ = std::make_unique<PlaylistStreaming>(context);
  media_detector_component_manager_ = manager;

  enabled_pref_.Init(kPlaylistEnabledPref, prefs_.get(),
                     base::BindRepeating(&PlaylistService::OnEnabledPrefChanged,
                                         weak_factory_.GetWeakPtr()));

  // This is for cleaning up malformed items during development. Once we
  // release Playlist feature officially, we should migrate items
  // instead of deleting them.
  MigratePlaylistValues();

  CleanUpOrphanedPlaylistItemDirs();
}

PlaylistService::~PlaylistService() = default;

void PlaylistService::Shutdown() {
  observers_.Clear();
  background_web_contentses_.reset();
  media_file_download_manager_.reset();
  thumbnail_downloader_.reset();
  task_runner_.reset();
  playlist_streaming_.reset();
#if BUILDFLAG(IS_ANDROID)
  receivers_.Clear();
#endif  // BUILDFLAG(IS_ANDROID)
}

void PlaylistService::AddMediaFilesFromContentsToPlaylist(
    const std::string& playlist_id,
    content::WebContents* contents,
    bool cache,
    base::OnceCallback<void(std::vector<mojom::PlaylistItemPtr>)> callback) {
  CHECK(*enabled_pref_) << "Playlist pref must be enabled";

  DCHECK(contents);
  if (!contents->GetPrimaryMainFrame()) {
    return;
  }

  VLOG(2) << __func__
          << " download media from WebContents to playlist: " << playlist_id;

  auto* tab_helper = PlaylistTabHelper::FromWebContents(contents);
  CHECK(tab_helper);
  const auto& found_items = tab_helper->found_items();
  if (found_items.empty()) {
    if (callback) {
      return std::move(callback).Run({});
    }
    return;
  }

  std::vector<mojom::PlaylistItemPtr> items;
  base::ranges::transform(found_items, std::back_inserter(items),
                          &mojom::PlaylistItemPtr::Clone);
  AddMediaFiles(std::move(items), playlist_id, cache, std::move(callback));
}

bool PlaylistService::AddItemsToPlaylist(
    const std::string& playlist_id,
    const std::vector<std::string>& item_ids) {
  DCHECK(!playlist_id.empty());

  ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
  base::Value::Dict* target_playlist = playlists_update->FindDict(playlist_id);
  if (!target_playlist) {
    LOG(ERROR) << __func__ << " Playlist " << playlist_id << " not found";
    return false;
  }

  auto playlist = ConvertValueToPlaylist(*target_playlist,
                                         prefs_->GetDict(kPlaylistItemsPref));
  for (const auto& new_item_id : item_ids) {
    // We're considering adding item to which it was belong as success.
    if (base::ranges::find_if(playlist->items,
                              [&new_item_id](const auto& item) {
                                return item->id == new_item_id;
                              }) != playlist->items.end()) {
      continue;
    }

    // Update the item's parent lists.
    auto new_item = GetPlaylistItem(new_item_id);
    new_item->parents.push_back(playlist_id);
    UpdatePlaylistItemValue(new_item->id,
                            base::Value(ConvertPlaylistItemToValue(new_item)));

    playlist->items.push_back(std::move(new_item));
  }

  playlists_update->Set(playlist_id, ConvertPlaylistToValue(playlist));

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

  {
    ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
    auto target_playlist_id =
        playlist_id->empty() ? kDefaultPlaylistID : *playlist_id;
    base::Value::Dict* playlist_value =
        playlists_update->FindDict(target_playlist_id);
    if (!playlist_value) {
      VLOG(2) << __func__ << " Playlist " << playlist_id << " not found";
      return false;
    }

    auto target_playlist = ConvertValueToPlaylist(
        *playlist_value, prefs_->GetDict(kPlaylistItemsPref));
    auto it = base::ranges::find_if(
        target_playlist->items,
        [&item_id](const auto& item) { return item->id == *item_id; });
    // Consider this as success since the item is already removed.
    if (it == target_playlist->items.end()) {
      return true;
    }

    target_playlist->items.erase(it, it + 1);
    playlists_update->Set(target_playlist_id,
                          ConvertPlaylistToValue(target_playlist));
  }

  // Try to remove |playlist_id| from item->parents or delete the this item
  // if there's no other parent playlist.
  auto item = GetPlaylistItem(*item_id);
  if (delete_item && item->parents.size() == 1) {
    DCHECK_EQ(item->parents.front(), *playlist_id);
    DeletePlaylistItemData(item->id);
    return true;
  }

  // There're other playlists referencing this. Don't delete item
  // and update the item's parent playlists data.
  auto iter = base::ranges::find(item->parents, *playlist_id);
  DCHECK(iter != item->parents.end());
  item->parents.erase(iter);
  UpdatePlaylistItemValue(item->id,
                          base::Value(ConvertPlaylistItemToValue(item)));
  for (auto& observer : observers_) {
    observer->OnItemRemovedFromList(*playlist_id, item->id);
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

  {
    ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
    base::Value::Dict* playlist_value =
        playlists_update->FindDict(target_playlist_id);
    DCHECK(playlist_value) << " Playlist " << playlist_id << " not found";

    auto target_playlist = ConvertValueToPlaylist(
        *playlist_value, prefs_->GetDict(kPlaylistItemsPref));
    DCHECK_GT(target_playlist->items.size(), static_cast<size_t>(position));
    auto it = base::ranges::find_if(
        target_playlist->items,
        [&item_id](const auto& item) { return item->id == item_id; });
    DCHECK(it != target_playlist->items.end());

    auto old_position = std::distance(target_playlist->items.begin(), it);
    if (old_position == position) {
      return;
    }

    if (old_position < position) {
      std::rotate(it, it + 1, target_playlist->items.begin() + position + 1);
    } else {
      std::rotate(target_playlist->items.begin() + position, it, it + 1);
    }
    playlists_update->Set(target_playlist_id,
                          ConvertPlaylistToValue(target_playlist));
  }

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
  base::ranges::for_each(filtered_items, [&target_playlist_id](auto& item) {
    item->parents.push_back(target_playlist_id);
  });

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

bool PlaylistService::HasPrefStorePlaylistItem(const std::string& id) const {
  const auto& items = prefs_->GetDict(kPlaylistItemsPref);
  const base::Value::Dict* playlist_info = items.FindDict(id);
  return !!playlist_info;
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

  for (auto& observer : observers_) {
    observer->OnMediaFileDownloadScheduled(item->id);
  }
}

base::FilePath PlaylistService::GetPlaylistItemDirPath(
    const std::string& id) const {
  return base_dir_.AppendASCII(id);
}

base::WeakPtr<PlaylistService> PlaylistService::GetWeakPtr() {
  return weak_factory_.GetWeakPtr();
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
  for (const auto it : prefs_->GetDict(kPlaylistItemsPref)) {
    items.push_back(ConvertValueToPlaylistItem(it.second.GetDict()));
  }
  return items;
}

void PlaylistService::GetPlaylistItem(const std::string& id,
                                      GetPlaylistItemCallback callback) {
  return std::move(callback).Run(GetPlaylistItem(id));
}

mojom::PlaylistItemPtr PlaylistService::GetPlaylistItem(const std::string& id) {
  DCHECK(!id.empty());
  const auto* item_value = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  DCHECK(item_value);
  if (!item_value) {
    return {};
  }

  return ConvertValueToPlaylistItem(*item_value);
}

mojom::PlaylistPtr PlaylistService::GetPlaylist(const std::string& id) {
  const auto& playlists = prefs_->GetDict(kPlaylistsPref);
  if (!playlists.contains(id)) {
    LOG(ERROR) << __func__ << " playlist with id<" << id << "> not found";
    return {};
  }
  playlist_p3a_.ReportNewUsage();

  auto* playlist_dict = playlists.FindDict(id);
  DCHECK(playlist_dict);

  const auto& items_dict = prefs_->GetDict(kPlaylistItemsPref);
  return ConvertValueToPlaylist(*playlist_dict, items_dict);
}

std::vector<mojom::PlaylistPtr> PlaylistService::GetAllPlaylists() {
  std::vector<mojom::PlaylistPtr> playlists;
  const auto& playlists_dict = prefs_->GetDict(kPlaylistsPref);
  const auto& items_dict = prefs_->GetDict(kPlaylistItemsPref);

  for (const auto& id : prefs_->GetList(kPlaylistOrderPref)) {
    auto* playlist_value = playlists_dict.Find(id.GetString());
    DCHECK(playlist_value->is_dict());
    playlists.push_back(
        ConvertValueToPlaylist(playlist_value->GetDict(), items_dict));
  }

  playlist_p3a_.ReportNewUsage();

  return playlists;
}

bool PlaylistService::HasPlaylistItem(const std::string& id) const {
  return prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
}

const std::string& PlaylistService::GetMediaSourceAPISuppressorScript() const {
  return media_detector_component_manager_->GetMediaSourceAPISuppressorScript();
}

std::string PlaylistService::GetMediaDetectorScript(const GURL& url) const {
  return media_detector_component_manager_->GetMediaDetectorScript(url);
}

void PlaylistService::SetUpForTesting() const {
  media_detector_component_manager_->SetUseLocalScript();
}

void PlaylistService::AddMediaFilesFromActiveTabToPlaylist(
    const std::string& playlist_id,
    bool can_cache,
    AddMediaFilesFromActiveTabToPlaylistCallback callback) {
  DCHECK(delegate_);

  auto* contents = delegate_->GetActiveWebContents();
  if (!contents) {
    return;
  }

  AddMediaFilesFromContentsToPlaylist(
      playlist_id, contents,
      /* cache= */ can_cache && prefs_->GetBoolean(kPlaylistCacheByDefault),
      std::move(callback));
}

void PlaylistService::FindMediaFilesFromActiveTab() {
  auto* web_contents = delegate_->GetActiveWebContents();
  if (!web_contents) {
    return;
  }

  auto* tab_helper = PlaylistTabHelper::FromWebContents(web_contents);
  CHECK(tab_helper);

  const auto url = web_contents->GetLastCommittedURL();

  for (auto& observer : observers_) {
    std::vector<mojom::PlaylistItemPtr> cloned_items;
    base::ranges::transform(tab_helper->found_items(),
                            std::back_inserter(cloned_items),
                            &mojom::PlaylistItemPtr::Clone);
    observer->OnMediaFilesUpdated(url, std::move(cloned_items));
  }
}

void PlaylistService::AddMediaFiles(std::vector<mojom::PlaylistItemPtr> items,
                                    const std::string& playlist_id,
                                    bool can_cache,
                                    AddMediaFilesCallback callback) {
  auto add_media_files_from_items = base::IgnoreArgs<GURL>(base::BindOnce(
      &PlaylistService::AddMediaFilesFromItems, GetWeakPtr(), playlist_id,
      can_cache && prefs_->GetBoolean(playlist::kPlaylistCacheByDefault),
      std::move(callback)));

  if (items.size() == 1 && items[0]->is_blob_from_media_source) {
    background_web_contentses_->Add(items[0]->page_source,
                                    std::move(add_media_files_from_items));
  } else {
    std::move(add_media_files_from_items).Run(GURL(), std::move(items));
  }
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
  UpdatePlaylistItemValue(item->id,
                          base::Value(ConvertPlaylistItemToValue(item)));
  NotifyPlaylistChanged(mojom::PlaylistEvent::kItemUpdated, item->id);
  for (auto& observer : observers_) {
    observer->OnItemUpdated(item.Clone());
  }
}

void PlaylistService::UpdateItemLastPlayedPosition(
    const std::string& playlist_item_id,
    int32_t last_played_position) {
  if (!HasPlaylistItem(playlist_item_id)) {
    return;
  }

  auto item = GetPlaylistItem(playlist_item_id);
  item->last_played_position = last_played_position;
  UpdateItem(std::move(item));
}

void PlaylistService::UpdateItemHlsMediaFilePath(
    const std::string& playlist_item_id,
    const std::string& hls_media_file_path,
    int64_t updated_file_size) {
  if (!HasPlaylistItem(playlist_item_id)) {
    return;
  }

  auto item = GetPlaylistItem(playlist_item_id);
  item->hls_media_path = GURL(base::StrCat(
      {url::kFileScheme, url::kStandardSchemeSeparator, hls_media_file_path}));
  item->media_file_bytes = updated_file_size;
  UpdateItem(std::move(item));
}

void PlaylistService::CreatePlaylist(mojom::PlaylistPtr playlist,
                                     CreatePlaylistCallback callback) {
  do {
    playlist->id = base::Token::CreateRandom().ToString();
  } while (playlist->id == kDefaultPlaylistID);

  {
    ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
    playlists_update->Set(playlist->id.value(),
                          ConvertPlaylistToValue(playlist));

    ScopedListPrefUpdate playlists_order_update(prefs_, kPlaylistOrderPref);
    playlists_order_update->Append(playlist->id.value());
  }

  NotifyPlaylistChanged(mojom::PlaylistEvent::kListCreated,
                        playlist->id.value());

  std::move(callback).Run(playlist.Clone());
}

void PlaylistService::ReorderPlaylist(const std::string& playlist_id,
                                      int16_t position,
                                      ReorderPlaylistCallback callback) {
  {
    ScopedListPrefUpdate playlist_order_update(prefs_, kPlaylistOrderPref);
    auto it = base::ranges::find(*playlist_order_update, playlist_id);
    if (it == playlist_order_update->end()) {
      std::move(callback).Run(false);
      return;
    }

    auto old_position = std::distance(playlist_order_update->begin(), it);
    if (old_position == position) {
      std::move(callback).Run(true);
      return;
    }

    if (old_position < position) {
      std::rotate(it, it + 1, playlist_order_update->begin() + position + 1);
    } else {
      std::rotate(playlist_order_update->begin() + position, it, it + 1);
    }
  }

  std::move(callback).Run(true);
}

std::string PlaylistService::GetDefaultSaveTargetListID() {
  auto id = prefs_->GetString(kPlaylistDefaultSaveTargetListID);
  if (!prefs_->GetDict(kPlaylistsPref).contains(id)) {
    prefs_->SetString(kPlaylistDefaultSaveTargetListID, kDefaultPlaylistID);
    id = kDefaultPlaylistID;
  }
  return id;
}

void PlaylistService::UpdatePlaylistItemValue(const std::string& id,
                                              base::Value value) {
  ScopedDictPrefUpdate playlist_items(prefs_, kPlaylistItemsPref);
  playlist_items->Set(id, std::move(value));
}

void PlaylistService::RemovePlaylistItemValue(const std::string& id) {
  ScopedDictPrefUpdate playlist_items(prefs_, kPlaylistItemsPref);
  playlist_items->Remove(id);
}

void PlaylistService::CreatePlaylistItem(const mojom::PlaylistItemPtr& item,
                                         bool cache) {
  VLOG(2) << __func__;

  UpdatePlaylistItemValue(item->id,
                          base::Value(ConvertPlaylistItemToValue(item)));
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
    std::move(callback).Run(
        new base::RefCountedBytes(base::as_byte_span(*image)));
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

  const auto* value = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  DCHECK(value);
  auto playlist_item = ConvertValueToPlaylistItem(*value);
  playlist_item->thumbnail_path = GURL("file://" + path.AsUTF8Unsafe());
  UpdatePlaylistItemValue(
      id, base::Value(ConvertPlaylistItemToValue(playlist_item)));
  NotifyPlaylistChanged(mojom::PlaylistEvent::kItemThumbnailReady, id);
}

void PlaylistService::RemovePlaylist(const std::string& playlist_id) {
  if (playlist_id == kDefaultPlaylistID) {
    return;
  }

  DCHECK(!playlist_id.empty());

  {
    ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
    base::Value::Dict* target_playlist =
        playlists_update->FindDict(playlist_id);
    if (!target_playlist) {
      LOG(ERROR) << __func__ << " Playlist " << playlist_id << " not found";
      return;
    }

    auto playlist = ConvertValueToPlaylist(*target_playlist,
                                           prefs_->GetDict(kPlaylistItemsPref));
    for (const auto& item : playlist->items) {
      RemoveItemFromPlaylist(PlaylistId(playlist_id), PlaylistItemId(item->id),
                             /* delete= */ true);
    }

    playlists_update->Remove(playlist_id);

    ScopedListPrefUpdate playlists_order_update(prefs_, kPlaylistOrderPref);
    playlists_order_update->EraseValue(base::Value(playlist_id));
  }

  NotifyPlaylistChanged(mojom::PlaylistEvent::kListRemoved, playlist_id);
}

void PlaylistService::ResetAll() {
  // Resets all on-going downloads
  thumbnail_downloader_->CancelAllDownloadRequests();
  media_file_download_manager_->CancelAllDownloadRequests();
  playlist_streaming_->ClearAllQueries();

  // Resets preference ---------------------------------------------------------
  prefs_->ClearPref(kPlaylistCacheByDefault);
  prefs_->ClearPref(kPlaylistDefaultSaveTargetListID);

  auto items = GetAllPlaylistItems();
  prefs_->ClearPref(kPlaylistItemsPref);
  for (const auto& item : items) {
    for (auto& observer : observers_) {
      observer->OnItemLocalDataDeleted(item->id);
    }
  }

  prefs_->ClearPref(kPlaylistsPref);
  prefs_->ClearPref(kPlaylistOrderPref);

  // Removes data on disk
  // ------------------------------------------------------
  GetTaskRunner()->PostTask(FROM_HERE,
                            base::GetDeletePathRecursivelyCallback(base_dir_));
}

void PlaylistService::RenamePlaylist(const std::string& playlist_id,
                                     const std::string& playlist_name,
                                     RenamePlaylistCallback callback) {
  ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
  auto target_playlist_id =
      playlist_id.empty() ? kDefaultPlaylistID : playlist_id;
  base::Value::Dict* playlist_value =
      playlists_update->FindDict(target_playlist_id);
  DCHECK(playlist_value) << " Playlist " << playlist_id << " not found";

  auto target_playlist = ConvertValueToPlaylist(
      *playlist_value, prefs_->GetDict(kPlaylistItemsPref));

  target_playlist->name = playlist_name;
  playlists_update->Set(playlist_id, ConvertPlaylistToValue(target_playlist));
  std::move(callback).Run(target_playlist.Clone());
}

void PlaylistService::RecoverLocalDataForItem(
    const std::string& id,
    bool update_media_src_before_recovery,
    RecoverLocalDataForItemCallback callback) {
  CHECK(*enabled_pref_) << "Playlist pref must be enabled";

  const auto* item_value = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  if (!item_value) {
    LOG(ERROR) << __func__ << ": Invalid playlist id for recovery: " << id;
    if (callback) {
      std::move(callback).Run({});
    }
    return;
  }

  auto item = ConvertValueToPlaylistItem(*item_value);
  DCHECK(item);

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
        DCHECK_EQ(new_item->media_source, old_item->media_source);
        DCHECK_EQ(new_item->media_path, old_item->media_path);
        const bool was_cached = new_item->cached;
        if (was_cached)
          new_item->cached = false;
        new_item->media_source = found_items.front()->media_source;
        new_item->media_path = new_item->media_source;
        service->UpdatePlaylistItemValue(
            new_item->id, base::Value(ConvertPlaylistItemToValue(new_item)));

        if (was_cached) {
          service->NotifyPlaylistChanged(
              mojom::PlaylistEvent::kItemLocalDataRemoved, new_item->id);
        }

        service->RecoverLocalDataForItemImpl(
            std::move(new_item),
            /*update_media_src_and_retry_on_fail=*/false, std::move(callback));
      },
      weak_factory_.GetWeakPtr(), item->Clone(), std::move(callback));

  background_web_contentses_->Add(
      item->page_source,
      base::IgnoreArgs<GURL>(std::move(update_media_src_and_recover)));
}

void PlaylistService::RemoveLocalDataForItemsInPlaylist(
    const std::string& playlist_id) {
  const auto* item_value =
      prefs_->GetDict(kPlaylistsPref).FindDict(playlist_id);
  DCHECK(item_value);

  auto playlist =
      ConvertValueToPlaylist(*item_value, prefs_->GetDict(kPlaylistItemsPref));
  for (const auto& item : playlist->items) {
    RemoveLocalDataForItemImpl(item);
  }
}

void PlaylistService::DeletePlaylistItemData(const std::string& id) {
  media_file_download_manager_->CancelDownloadRequest(id);
  thumbnail_downloader_->CancelDownloadRequest(id);

  RemovePlaylistItemValue(id);
  NotifyPlaylistChanged(mojom::PlaylistEvent::kItemDeleted, id);
  for (auto& observer : observers_) {
    observer->OnItemLocalDataDeleted(id);
  }

  // TODO(simonhong): Delete after getting cancel complete message from all
  // downloader.
  // Delete assets from filesystem after updating db.
  GetTaskRunner()->PostTask(FROM_HERE, base::GetDeletePathRecursivelyCallback(
                                           GetPlaylistItemDirPath(id)));
}

void PlaylistService::RemoveLocalDataForItem(const std::string& id) {
  const auto* item_value = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  DCHECK(item_value);
  auto playlist_item = ConvertValueToPlaylistItem(*item_value);
  RemoveLocalDataForItemImpl(playlist_item);
}

void PlaylistService::DeleteAllPlaylistItems() {
  VLOG(2) << __func__;

  // Cancel currently generated playlist if needed and pending thumbnail
  // download jobs.
  media_file_download_manager_->CancelAllDownloadRequests();
  thumbnail_downloader_->CancelAllDownloadRequests();

  prefs_->ClearPref(kPlaylistItemsPref);
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

  item->cached = false;
  item->media_file_bytes = 0;
  DCHECK(item->media_source.is_valid()) << "media_source should be valid";
  item->media_path = item->media_source;
  UpdatePlaylistItemValue(item->id,
                          base::Value(ConvertPlaylistItemToValue(item)));
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
  if (!IsValidPlaylistItem(item->id)) {
    // As this callback is async, the item could have been removed.
    return;
  }

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

  // The item's other data could have been updated.
  item = GetPlaylistItem(item->id);

  item->cached = !media_file_path.empty();
  if (item->cached) {
    item->media_path = GURL("file://" + media_file_path);
    if (received_bytes) {
      item->media_file_bytes = received_bytes;
    }
  }
  UpdatePlaylistItemValue(item->id,
                          base::Value(ConvertPlaylistItemToValue(item)));
  NotifyPlaylistChanged(item->cached ? mojom::PlaylistEvent::kItemCached
                                     : mojom::PlaylistEvent::kItemAborted,
                        item->id);
  if (item->cached) {
    for (auto& observer : observers_) {
      observer->OnItemCached(item.Clone());
    }
  }

  if (callback) {
    std::move(callback).Run(item.Clone());
  }
}

void PlaylistService::OnEnabledPrefChanged() {
  if (!*enabled_pref_) {
    background_web_contentses_->Reset();
    thumbnail_downloader_->CancelAllDownloadRequests();
    media_file_download_manager_->CancelAllDownloadRequests();
  }

  if (delegate_) {
    delegate_->EnabledStateChanged(*enabled_pref_);
  }
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

void PlaylistService::OnMediaDetected(
    GURL url,
    std::vector<mojom::PlaylistItemPtr> items) {
  if (!*enabled_pref_) {
    return;
  }

  for (auto& observer : observers_) {
    std::vector<mojom::PlaylistItemPtr> cloned_items;
    base::ranges::transform(items, std::back_inserter(cloned_items),
                            &mojom::PlaylistItemPtr::Clone);
    observer->OnMediaFilesUpdated(url, std::move(cloned_items));
  }
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
  return HasPrefStorePlaylistItem(id);
}

base::FilePath PlaylistService::GetMediaPathForPlaylistItemItem(
    const std::string& id) {
  base::FilePath path;
  CHECK(GetMediaPath(id, &path));
  return path;
}

void PlaylistService::OnGetOrphanedPaths(
    const std::vector<base::FilePath>& orphaned_paths) {
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

void PlaylistService::MigratePlaylistValues() {
  // Migration code here should be gone after a few versions
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

base::SequencedTaskRunner* PlaylistService::GetTaskRunner() {
  if (!task_runner_) {
    task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return task_runner_.get();
}

void PlaylistService::RequestStreamingQuery(
    const std::string& query_id,
    const std::string& url,
    const std::string& method,
    mojo::PendingRemote<mojom::PlaylistStreamingObserver> streaming_observer) {
  if (streaming_observer_.is_bound()) {
    streaming_observer_.reset();
  }

  streaming_observer_.Bind(std::move(streaming_observer));
  mojo::Remote<mojom::PlaylistStreamingObserver> observer;
  observer.Bind(std::move(streaming_observer));
  playlist_streaming_->RequestStreamingQuery(
      query_id, url, method,
      base::BindOnce(&PlaylistService::OnResponseStarted,
                     weak_factory_.GetWeakPtr()),
      base::BindRepeating(&PlaylistService::OnDataReceived,
                          weak_factory_.GetWeakPtr()),
      base::BindOnce(&PlaylistService::OnDataComplete,
                     weak_factory_.GetWeakPtr()));
}

void PlaylistService::ClearAllQueries() {
  playlist_streaming_->ClearAllQueries();
}

void PlaylistService::CancelQuery(const std::string& query_id) {
  playlist_streaming_->CancelQuery(query_id);
}

void PlaylistService::OnResponseStarted(const std::string& url,
                                        const int64_t content_length) {
  streaming_observer_->OnResponseStarted(url, content_length);
}

void PlaylistService::OnDataReceived(api_request_helper::ValueOrError result) {
  if (!result.has_value()) {
    return;
  }

  std::vector<uint8_t> data_received(result.value().GetString().begin(),
                                     result.value().GetString().end());
  streaming_observer_->OnDataReceived(data_received);
}

void PlaylistService::OnDataComplete(
    api_request_helper::APIRequestResult result) {
  if (result.Is2XXResponseCode()) {
    streaming_observer_->OnDataCompleted();
  }
}

}  // namespace playlist
