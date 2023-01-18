/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/browser/playlist_service.h"

#include <algorithm>
#include <utility>

#include "base/bind.h"
#include "base/containers/flat_set.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/json/values_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/task/thread_pool.h"
#include "base/token.h"
#include "brave/components/playlist/browser/playlist_constants.h"
#include "brave/components/playlist/browser/playlist_data_source.h"
#include "brave/components/playlist/browser/playlist_service_observer.h"
#include "brave/components/playlist/browser/playlist_types.h"
#include "brave/components/playlist/browser/pref_names.h"
#include "brave/components/playlist/browser/type_converter.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"

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
    if (ids.find(name.BaseName().AsUTF8Unsafe()) == ids.end())
      orphaned_paths.push_back(name);
  }
  return orphaned_paths;
}

}  // namespace

PlaylistService::PlaylistService(content::BrowserContext* context,
                                 MediaDetectorComponentManager* manager,
                                 std::unique_ptr<Delegate> delegate)
    : delegate_(std::move(delegate)),
      base_dir_(context->GetPath().Append(kBaseDirName)),
      prefs_(user_prefs::UserPrefs::Get(context)) {
  content::URLDataSource::Add(context,
                              std::make_unique<PlaylistDataSource>(this));
  media_file_download_manager_ =
      std::make_unique<PlaylistMediaFileDownloadManager>(context, this,
                                                         base_dir_);
  thumbnail_downloader_ =
      std::make_unique<PlaylistThumbnailDownloader>(context, this);
  download_request_manager_ =
      std::make_unique<PlaylistDownloadRequestManager>(context, manager);

  // This is for cleaning up malformed items during development. Once we
  // release Playlist feature officially, we should migrate items
  // instead of deleting them.
  CleanUpMalformedPlaylistItems();

  CleanUpOrphanedPlaylistItemDirs();
}

PlaylistService::~PlaylistService() = default;

void PlaylistService::Shutdown() {
  service_observers_.Clear();
  download_request_manager_.reset();
  media_file_download_manager_.reset();
  thumbnail_downloader_.reset();
  download_request_manager_.reset();
  task_runner_.reset();
}

void PlaylistService::AddMediaFilesFromContentsToPlaylist(
    const std::string& playlist_id,
    content::WebContents* contents,
    bool cache) {
  DCHECK(contents);
  if (!contents->GetPrimaryMainFrame())
    return;

  VLOG(2) << __func__
          << " download media from WebContents to playlist: " << playlist_id;

  PlaylistDownloadRequestManager::Request request;
  if (ShouldDownloadOnBackground(contents))
    request.url_or_contents = contents->GetVisibleURL().spec();
  else
    request.url_or_contents = contents->GetWeakPtr();

  request.callback = base::BindOnce(
      &PlaylistService::AddMediaFilesFromItems, weak_factory_.GetWeakPtr(),
      playlist_id.empty() ? GetDefaultSaveTargetListID() : playlist_id, cache);
  download_request_manager_->GetMediaFilesFromPage(std::move(request));
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

    auto new_item = mojom::PlaylistItem::New();
    new_item->id = new_item_id;
    playlist->items.push_back(std::move(new_item));
  }

  playlists_update->Set(playlist_id, ConvertPlaylistToValue(playlist));
  return true;
}

bool PlaylistService::RemoveItemFromPlaylist(const PlaylistId& playlist_id,
                                             const PlaylistItemId& item_id,
                                             bool remove_item) {
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
    if (it == target_playlist->items.end())
      return true;

    target_playlist->items.erase(it, it + 1);
    playlists_update->Set(target_playlist_id,
                          ConvertPlaylistToValue(target_playlist));
  }

  // TODO(sko) Once we can support to share an item between playlists, we should
  // check if other playlists have this item before deleting this item
  // permanantly.
  if (remove_item)
    DeletePlaylistItemData(*item_id);
  return true;
}

void PlaylistService::ReorderItemFromPlaylist(const std::string& playlist_id,
                                              const std::string& item_id,
                                              int16_t position) {
  VLOG(2) << __func__ << " " << playlist_id << " " << item_id;

  DCHECK(!item_id.empty());

  ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
  auto target_playlist_id =
      playlist_id.empty() ? kDefaultPlaylistID : playlist_id;
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
  if (old_position == position)
    return;

  if (old_position < position) {
    std::rotate(it, it + 1, target_playlist->items.begin() + position + 1);
  } else {
    std::rotate(target_playlist->items.begin() + position, it, it + 1);
  }
  playlists_update->Set(target_playlist_id,
                        ConvertPlaylistToValue(target_playlist));
}

bool PlaylistService::MoveItem(const PlaylistId& from,
                               const PlaylistId& to,
                               const PlaylistItemId& item) {
  if (!RemoveItemFromPlaylist(from, item, /* remove_item = */ false)) {
    LOG(ERROR) << "Failed to remove item from playlist";
    return false;
  }

  if (!AddItemsToPlaylist(*to, {*item})) {
    LOG(ERROR) << "Failed to add item to playlist";

    // Try to recover.
    AddItemsToPlaylist(*from, {*item});
    return false;
  }

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemDeleted, *from});
  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemAdded, *to});
  return true;
}

void PlaylistService::AddMediaFilesFromItems(
    const std::string& playlist_id,
    bool cache,
    std::vector<mojom::PlaylistItemPtr> items) {
  if (items.empty())
    return;

  std::vector<std::string> ids;
  base::ranges::transform(items, std::back_inserter(ids),
                          [](const auto& item) { return item->id; });
  AddItemsToPlaylist(
      playlist_id.empty() ? GetDefaultSaveTargetListID() : playlist_id, ids);

  base::ranges::for_each(items, [this, cache](const auto& item) {
    CreatePlaylistItem(item, cache);
  });
}

void PlaylistService::NotifyPlaylistChanged(
    const PlaylistChangeParams& params) {
  VLOG(2) << __func__ << ": params="
          << PlaylistChangeParams::GetPlaylistChangeTypeAsString(
                 params.change_type);

  for (PlaylistServiceObserver& obs : observers_)
    obs.OnPlaylistStatusChanged(params);

  // TODO(sko) Send proper events based on |params|
  for (auto& service_observer : service_observers_)
    service_observer->OnEvent(mojom::PlaylistEvent::kUpdated);
}

bool PlaylistService::HasPrefStorePlaylistItem(const std::string& id) const {
  const auto& items = prefs_->GetDict(kPlaylistItemsPref);
  const base::Value::Dict* playlist_info = items.FindDict(id);
  return !!playlist_info;
}

void PlaylistService::DownloadMediaFile(const mojom::PlaylistItemPtr& item) {
  VLOG(2) << __func__;
  media_file_download_manager_->DownloadMediaFile(item);
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

void PlaylistService::GetAllPlaylists(GetAllPlaylistsCallback callback) {
  std::vector<mojom::PlaylistPtr> playlists;
  const auto& items_dict = prefs_->GetDict(kPlaylistItemsPref);
  for (const auto [id, playlist_value] : prefs_->GetDict(kPlaylistsPref)) {
    DCHECK(playlist_value.is_dict());
    playlists.push_back(
        ConvertValueToPlaylist(playlist_value.GetDict(), items_dict));
  }

  std::move(callback).Run(std::move(playlists));
}

void PlaylistService::GetPlaylist(const std::string& id,
                                  GetPlaylistCallback callback) {
  const auto& playlists = prefs_->GetDict(kPlaylistsPref);
  if (!playlists.contains(id)) {
    LOG(ERROR) << __func__ << " playlist with id<" << id << "> not found";
    std::move(callback).Run({});
    return;
  }
  auto* playlist_dict = playlists.FindDict(id);
  DCHECK(playlist_dict);

  const auto& items_dict = prefs_->GetDict(kPlaylistItemsPref);
  std::move(callback).Run(ConvertValueToPlaylist(*playlist_dict, items_dict));
}

void PlaylistService::GetAllPlaylistItems(
    GetAllPlaylistItemsCallback callback) {
  std::vector<mojom::PlaylistItemPtr> items;
  for (const auto it : prefs_->GetDict(kPlaylistItemsPref)) {
    items.push_back(ConvertValueToPlaylistItem(it.second.GetDict()));
  }

  std::move(callback).Run(std::move(items));
}

void PlaylistService::GetPlaylistItem(const std::string& id,
                                      GetPlaylistItemCallback callback) {
  DCHECK(!id.empty());
  const auto* item_value = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  DCHECK(item_value);
  if (!item_value) {
    std::move(callback).Run({});
    return;
  }

  return std::move(callback).Run(ConvertValueToPlaylistItem(*item_value));
}

void PlaylistService::AddMediaFilesFromPageToPlaylist(
    const std::string& playlist_id,
    const GURL& url) {
  VLOG(2) << __func__ << " " << playlist_id << " " << url;
  PlaylistDownloadRequestManager::Request request;
  request.url_or_contents = url.spec();
  request.callback = base::BindOnce(
      &PlaylistService::AddMediaFilesFromItems, weak_factory_.GetWeakPtr(),
      playlist_id.empty() ? GetDefaultSaveTargetListID() : playlist_id,
      prefs_->GetBoolean(playlist::kPlaylistCacheByDefault));
  download_request_manager_->GetMediaFilesFromPage(std::move(request));
}

void PlaylistService::AddMediaFilesFromActiveTabToPlaylist(
    const std::string& playlist_id) {
  DCHECK(delegate_);

  auto* contents = delegate_->GetActiveWebContents();
  if (!contents)
    return;

  AddMediaFilesFromContentsToPlaylist(
      playlist_id, contents, prefs_->GetBoolean(kPlaylistCacheByDefault));
}

void PlaylistService::FindMediaFilesFromActiveTab(
    FindMediaFilesFromActiveTabCallback callback) {
  DCHECK(delegate_);

  auto* contents = delegate_->GetActiveWebContents();
  if (!contents) {
    std::move(callback).Run({});
    return;
  }

  PlaylistDownloadRequestManager::Request request;
  request.url_or_contents = contents->GetWeakPtr();
  request.callback = std::move(callback);
  download_request_manager_->GetMediaFilesFromPage(std::move(request));
}

void PlaylistService::AddMediaFiles(std::vector<mojom::PlaylistItemPtr> items,
                                    const std::string& playlist_id) {
  AddMediaFilesFromItems(playlist_id,
                         prefs_->GetBoolean(playlist::kPlaylistCacheByDefault),
                         std::move(items));
}

void PlaylistService::RemoveItemFromPlaylist(const std::string& playlist_id,
                                             const std::string& item_id) {
  RemoveItemFromPlaylist(PlaylistId(playlist_id), PlaylistItemId(item_id));
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

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemUpdated, item->id});
}

void PlaylistService::CreatePlaylist(mojom::PlaylistPtr playlist,
                                     CreatePlaylistCallback callback) {
  do {
    playlist->id = base::Token::CreateRandom().ToString();
  } while (playlist->id == kDefaultPlaylistID);

  ScopedDictPrefUpdate playlists_update(prefs_, kPlaylistsPref);
  playlists_update->Set(playlist->id.value(), ConvertPlaylistToValue(playlist));

  NotifyPlaylistChanged(
      {PlaylistChangeParams::Type::kListCreated, playlist->id.value()});

  std::move(callback).Run(playlist.Clone());
}

content::WebContents* PlaylistService::GetBackgroundWebContentsForTesting() {
  return download_request_manager_
      ->GetBackgroundWebContentsForTesting();  // IN-TEST
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

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemAdded, item->id});

  GetTaskRunner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::CreateDirectory, GetPlaylistItemDirPath(item->id)),
      base::BindOnce(&PlaylistService::OnPlaylistItemDirCreated,
                     weak_factory_.GetWeakPtr(), item.Clone(), cache));
}

bool PlaylistService::ShouldDownloadOnBackground(
    content::WebContents* contents) const {
  return download_request_manager_->media_detector_component_manager()
      ->ShouldHideMediaSrcAPI(contents->GetVisibleURL());
}

void PlaylistService::OnPlaylistItemDirCreated(mojom::PlaylistItemPtr item,
                                               bool cache,
                                               bool directory_ready) {
  VLOG(2) << __func__;
  if (!directory_ready) {
    NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemAborted, item->id});
    return;
  }

  DownloadThumbnail(item);
  if (cache)
    DownloadMediaFile(item);
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

void PlaylistService::OnThumbnailDownloaded(const std::string& id,
                                            const base::FilePath& path) {
  DCHECK(IsValidPlaylistItem(id));

  if (path.empty()) {
    VLOG(2) << __func__ << ": thumbnail fetching failed for " << id;
    NotifyPlaylistChanged(
        {PlaylistChangeParams::Type::kItemThumbnailFailed, id});
    return;
  }

  const auto* value = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  DCHECK(value);
  auto playlist_item = ConvertValueToPlaylistItem(*value);
  playlist_item->thumbnail_path = GURL(path.AsUTF8Unsafe());
  UpdatePlaylistItemValue(
      id, base::Value(ConvertPlaylistItemToValue(playlist_item)));
  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemThumbnailReady, id});
}

void PlaylistService::RemovePlaylist(const std::string& playlist_id) {
  if (playlist_id == kDefaultPlaylistID)
    return;

  DCHECK(!playlist_id.empty());
  base::Value::List id_list;
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
    for (const auto& items : playlist->items)
      id_list.Append(base::Value(items->id));

    playlists_update->Remove(playlist_id);
  }

  // TODO(sko) Iterating this will cause a callback to be called a lot of
  // times.
  for (const auto& item_id : id_list)
    DeletePlaylistItemData(item_id.GetString());

  NotifyPlaylistChanged(
      {PlaylistChangeParams::Type::kListRemoved, playlist_id});
}

void PlaylistService::RecoverLocalDataForItem(const std::string& id) {
  const auto* item_value = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  if (!item_value) {
    LOG(ERROR) << __func__ << ": Invalid playlist id for recovery: " << id;
    return;
  }

  if (ConvertValueToPlaylistItem(*item_value)->cached) {
    VLOG(2) << __func__ << ": This is ready to play(" << id << ")";
    return;
  }

  GetPlaylistItem(
      id,
      base::BindOnce(
          [](base::WeakPtr<PlaylistService> service,
             mojom::PlaylistItemPtr item) {
            DCHECK(service);
            DCHECK(!item->id.empty());

            auto on_check_if_path_exists = [](base::OnceClosure on_exists,
                                              base::OnceClosure on_not_exists,
                                              bool exists) {
              if (exists)
                std::move(on_exists).Run();
              else
                std::move(on_not_exists).Run();
            };

            auto on_path_exists = base::BindOnce(
                [](base::WeakPtr<PlaylistService> service,
                   mojom::PlaylistItemPtr item) {
                  service->OnPlaylistItemDirCreated(
                      item->Clone(),
                      /* cache = */ true,
                      /* directory_created = */ true);
                },
                service, item->Clone());

            auto on_path_not_exists = base::BindOnce(
                [](base::WeakPtr<PlaylistService> service,
                   mojom::PlaylistItemPtr item) {
                  if (!service)
                    return;

                  service->GetTaskRunner()->PostTaskAndReplyWithResult(
                      FROM_HERE,
                      base::BindOnce(&base::CreateDirectory,
                                     service->GetPlaylistItemDirPath(item->id)),
                      base::BindOnce(&PlaylistService::OnPlaylistItemDirCreated,
                                     service, item->Clone(),
                                     /* cache = */ true));
                },
                service, item->Clone());

            service->GetTaskRunner()->PostTaskAndReplyWithResult(
                FROM_HERE,
                base::BindOnce(&base::PathExists,
                               service->GetPlaylistItemDirPath(item->id)),
                base::BindOnce(std::move(on_check_if_path_exists),
                               std::move(on_path_exists),
                               std::move(on_path_not_exists)));
          },
          weak_factory_.GetWeakPtr()));
}

void PlaylistService::RemoveLocalDataForItemsInPlaylist(
    const std::string& playlist_id) {
  const auto* item_value =
      prefs_->GetDict(kPlaylistsPref).FindDict(playlist_id);
  DCHECK(item_value);

  auto playlist =
      ConvertValueToPlaylist(*item_value, prefs_->GetDict(kPlaylistItemsPref));
  for (const auto& item : playlist->items)
    RemoveLocalDataForItem(item);
}

void PlaylistService::DeletePlaylistItemData(const std::string& id) {
  media_file_download_manager_->CancelDownloadRequest(id);
  thumbnail_downloader_->CancelDownloadRequest(id);

  RemovePlaylistItemValue(id);

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemDeleted, id});

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
  RemoveLocalDataForItem(playlist_item);
}

void PlaylistService::DeleteAllPlaylistItems() {
  VLOG(2) << __func__;

  // Cancel currently generated playlist if needed and pending thumbnail
  // download jobs.
  media_file_download_manager_->CancelAllDownloadRequests();
  thumbnail_downloader_->CancelAllDownloadRequests();

  prefs_->ClearPref(kPlaylistItemsPref);

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kAllDeleted, ""});

  CleanUpOrphanedPlaylistItemDirs();
}

void PlaylistService::RemoveLocalDataForItem(
    const mojom::PlaylistItemPtr& item) {
  DCHECK(item);
  if (!item->cached)
    return;

  item->cached = false;
  DCHECK(item->media_source.is_valid()) << "media_source should be valid";
  item->media_path = item->media_source;
  UpdatePlaylistItemValue(item->id,
                          base::Value(ConvertPlaylistItemToValue(item)));

  NotifyPlaylistChanged(
      {PlaylistChangeParams::Type::kItemLocalDataRemoved, item->id});

  base::FilePath media_path;
  if (GetMediaPath(item->id, &media_path)) {
    auto delete_file = base::BindOnce(
        [](const base::FilePath& path) { base::DeleteFile(path); }, media_path);
    GetTaskRunner()->PostTask(FROM_HERE, std::move(delete_file));
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
  service_observers_.Add(std::move(observer));
}

void PlaylistService::AddObserverForTest(PlaylistServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void PlaylistService::RemoveObserverForTest(PlaylistServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void PlaylistService::OnMediaFileDownloadProgressed(
    const std::string& id,
    int64_t total_bytes,
    int64_t received_bytes,
    int percent_complete,
    base::TimeDelta time_remaining) {
  VLOG(2) << __func__ << " " << total_bytes << " " << received_bytes << " "
          << percent_complete << " " << time_remaining;

  for (auto& observer : observers_) {
    observer.OnMediaFileDownloadProgressed(id, total_bytes, received_bytes,
                                           percent_complete, time_remaining);
  }

  for (auto& service_observer : service_observers_) {
    service_observer->OnMediaFileDownloadProgressed(
        id, total_bytes, received_bytes, percent_complete,
        base::TimeDeltaToValue(time_remaining).GetString());
  }
}

void PlaylistService::OnMediaFileReady(const std::string& id,
                                       const std::string& media_file_path) {
  VLOG(2) << __func__ << ": " << id << " " << media_file_path;
  DCHECK(IsValidPlaylistItem(id));

  const auto* item_value = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  DCHECK(item_value);

  auto playlist_item = ConvertValueToPlaylistItem(*item_value);
  playlist_item->cached = true;
  playlist_item->media_path = GURL("file://" + media_file_path);
  UpdatePlaylistItemValue(
      id, base::Value(ConvertPlaylistItemToValue(playlist_item)));

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemCached, id});
}

void PlaylistService::OnMediaFileGenerationFailed(const std::string& id) {
  VLOG(2) << __func__ << ": " << id;

  DCHECK(IsValidPlaylistItem(id));

  const auto* item_value = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  DCHECK(item_value);
  auto playlist_item = ConvertValueToPlaylistItem(*item_value);
  playlist_item->cached = false;
  UpdatePlaylistItemValue(
      id, base::Value(ConvertPlaylistItemToValue(playlist_item)));

  thumbnail_downloader_->CancelDownloadRequest(id);
  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemAborted, id});
}

bool PlaylistService::IsValidPlaylistItem(const std::string& id) {
  return HasPrefStorePlaylistItem(id);
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

  for (const auto* pref_key : {kPlaylistsPref, kPlaylistItemsPref})
    prefs_->ClearPref(pref_key);
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

bool PlaylistService::GetMediaPath(const std::string& id,
                                   base::FilePath* media_path) {
  // TODO(sko) The extension of the media file can differ.
  // We should iterate files in the directory and find the match.
  *media_path = GetPlaylistItemDirPath(id).Append(
      PlaylistMediaFileDownloadManager::kMediaFileName);
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

}  // namespace playlist
