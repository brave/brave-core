/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlist/playlist_service.h"

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
#include "brave/components/playlist/playlist_constants.h"
#include "brave/components/playlist/playlist_data_source.h"
#include "brave/components/playlist/playlist_service_helper.h"
#include "brave/components/playlist/playlist_service_observer.h"
#include "brave/components/playlist/playlist_types.h"
#include "brave/components/playlist/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/user_prefs/user_prefs.h"
#include "content/public/browser/browser_context.h"
#include "services/preferences/public/cpp/dictionary_value_update.h"
#include "services/preferences/public/cpp/scoped_pref_update.h"

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
                                 MediaDetectorComponentManager* manager)
    : base_dir_(context->GetPath().Append(kBaseDirName)),
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
  download_request_manager_.reset();
  media_file_download_manager_.reset();
  thumbnail_downloader_.reset();
  download_request_manager_.reset();
  task_runner_.reset();
}

void PlaylistService::RequestDownloadMediaFilesFromContents(
    const std::string& playlist_id,
    content::WebContents* contents) {
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

  request.callback =
      base::BindOnce(&PlaylistService::RequestDownloadMediaFilesFromItems,
                     base::Unretained(this),
                     playlist_id.empty() ? kDefaultPlaylistID : playlist_id);
  download_request_manager_->GetMediaFilesFromPage(std::move(request));
}

void PlaylistService::RequestDownloadMediaFilesFromPage(
    const std::string& playlist_id,
    const std::string& url) {
  VLOG(2) << __func__ << " " << playlist_id << " " << url;
  PlaylistDownloadRequestManager::Request request;
  request.url_or_contents = url;
  request.callback =
      base::BindOnce(&PlaylistService::RequestDownloadMediaFilesFromItems,
                     base::Unretained(this),
                     playlist_id.empty() ? kDefaultPlaylistID : playlist_id);
  download_request_manager_->GetMediaFilesFromPage(std::move(request));
}

bool PlaylistService::AddItemsToPlaylist(
    const std::string& playlist_id,
    const std::vector<std::string>& item_ids) {
  prefs::ScopedDictionaryPrefUpdate playlists_update(prefs_, kPlaylistsPref);
  std::unique_ptr<prefs::DictionaryValueUpdate> target_playlist_update;
  if (!playlists_update->GetDictionary(playlist_id, &target_playlist_update)) {
    LOG(ERROR) << __func__ << " Playlist " << playlist_id << " not found";
    return false;
  }

  base::Value::List* ids_list = nullptr;
  target_playlist_update->GetListWithoutPathExpansion(kPlaylistItemsKey,
                                                      &ids_list);
  DCHECK(ids_list) << __func__ << " Playlist " << playlist_id
                   << " doesn't have |items| field";

  for (const auto& id : item_ids) {
    DCHECK(!id.empty());
    // Skip if this is already in items.
    if (auto iter = base::ranges::find_if(
            *ids_list,
            [&id](const auto& item) { return item.GetString() == id; });
        iter != ids_list->end()) {
      continue;
    }

    ids_list->Append(id);
  }

  target_playlist_update->Set(
      kPlaylistItemsKey,
      base::Value::ToUniquePtrValue(base::Value(std::move(*ids_list))));
  return true;
}

bool PlaylistService::RemoveItemFromPlaylist(const PlaylistId& playlist_id,
                                             const PlaylistItemId& item_id,
                                             bool remove_item) {
  VLOG(2) << __func__ << " " << *playlist_id << " " << *item_id;

  DCHECK(!item_id->empty());

  {
    prefs::ScopedDictionaryPrefUpdate playlists_update(prefs_, kPlaylistsPref);
    std::unique_ptr<prefs::DictionaryValueUpdate> target_playlist_update;
    if (!playlists_update->GetDictionary(
            playlist_id->empty() ? kDefaultPlaylistID : *playlist_id,
            &target_playlist_update)) {
      VLOG(2) << __func__ << " Playlist " << playlist_id << " not found";
      return false;
    }

    base::Value::List* item_ids = nullptr;
    target_playlist_update->GetListWithoutPathExpansion(kPlaylistItemsKey,
                                                        &item_ids);
    DCHECK(item_ids) << __func__ << " Playlist " << playlist_id
                     << " doesn't have |items| field";

    auto it = base::ranges::find_if(*item_ids, [&item_id](const auto& id) {
      return id.GetString() == *item_id;
    });
    // Consider this as success as the item is already removed.
    if (it == item_ids->end())
      return true;

    item_ids->erase(it);

    target_playlist_update->Set(
        kPlaylistItemsKey,
        base::Value::ToUniquePtrValue(base::Value(std::move(*item_ids))));
  }

  // TODO(sko) Once we can support to share an item between playlists, we should
  // check if other playlists have this item before deleting this item
  // permanantly.
  if (remove_item)
    DeletePlaylistItemData(*item_id);
  return true;
}

void PlaylistService::RequestDownloadMediaFilesFromItems(
    const std::string& playlist_id,
    const std::vector<PlaylistItemInfo>& params) {
  if (params.empty())
    return;

  std::vector<std::string> ids;
  base::ranges::transform(params, std::back_inserter(ids),
                          [](const auto& item) { return item.id; });
  AddItemsToPlaylist(playlist_id, ids);

  base::ranges::for_each(
      params, [this](const auto& info) { CreatePlaylistItem(info); });
}

void PlaylistService::NotifyPlaylistChanged(
    const PlaylistChangeParams& params) {
  VLOG(2) << __func__ << ": params="
          << PlaylistChangeParams::GetPlaylistChangeTypeAsString(
                 params.change_type);

  for (PlaylistServiceObserver& obs : observers_)
    obs.OnPlaylistStatusChanged(params);
}

bool PlaylistService::HasPrefStorePlaylistItem(const std::string& id) const {
  const auto& items = prefs_->GetDict(kPlaylistItemsPref);
  const base::Value::Dict* playlist_info = items.FindDict(id);
  return !!playlist_info;
}

void PlaylistService::DownloadMediaFile(const PlaylistItemInfo& info) {
  VLOG(2) << __func__;
  LOG(ERROR) << "BravePlaylist"
             << "DownloadMediaFile";
  media_file_download_manager_->DownloadMediaFile(info);
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

content::WebContents* PlaylistService::GetBackgroundWebContentsForTesting() {
  return download_request_manager_->GetBackgroundWebContentsForTesting();
}

void PlaylistService::UpdatePlaylistItemValue(const std::string& id,
                                              base::Value value) {
  prefs::ScopedDictionaryPrefUpdate update(prefs_, kPlaylistItemsPref);
  auto playlist_items = update.Get();
  playlist_items->Set(id, base::Value::ToUniquePtrValue(std::move(value)));
}

void PlaylistService::RemovePlaylistItemValue(const std::string& id) {
  prefs::ScopedDictionaryPrefUpdate update(prefs_, kPlaylistItemsPref);
  auto playlist_items = update.Get();
  playlist_items->Remove(id);
}

void PlaylistService::CreatePlaylistItem(const PlaylistItemInfo& params) {
  VLOG(2) << __func__;

  UpdatePlaylistItemValue(params.id,
                          base::Value(GetValueFromPlaylistItemInfo(params)));

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemAdded, params.id});

  task_runner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::CreateDirectory, GetPlaylistItemDirPath(params.id)),
      base::BindOnce(&PlaylistService::OnPlaylistItemDirCreated,
                     weak_factory_.GetWeakPtr(), params));
}

bool PlaylistService::ShouldDownloadOnBackground(
    content::WebContents* contents) const {
  return download_request_manager_->media_detector_component_manager()
      ->ShouldHideMediaSrcAPI(contents->GetVisibleURL());
}

void PlaylistService::OnPlaylistItemDirCreated(const PlaylistItemInfo& info,
                                               bool directory_ready) {
  VLOG(2) << __func__;
  if (!directory_ready) {
    NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemAborted, info.id});
    return;
  }

  DownloadThumbnail(info);
  DownloadMediaFile(info);
}

void PlaylistService::DownloadThumbnail(const PlaylistItemInfo& info) {
  VLOG(2) << __func__ << " " << info.thumbnail_src;

  if (info.thumbnail_path != info.thumbnail_src) {
    // Already downloaded.
    return;
  }

  thumbnail_downloader_->DownloadThumbnail(
      info.id, GURL(info.thumbnail_src),
      GetPlaylistItemDirPath(info.id).Append(kThumbnailFileName));
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
  if (value) {
    base::Value::Dict copied_value = value->Clone();
    copied_value.Set(kPlaylistItemThumbnailPathKey, path.AsUTF8Unsafe());
    UpdatePlaylistItemValue(id, base::Value(std::move(copied_value)));
    NotifyPlaylistChanged(
        {PlaylistChangeParams::Type::kItemThumbnailReady, id});
  }
}

void PlaylistService::CreatePlaylist(PlaylistInfo& info) {
  do {
    info.id = base::Token::CreateRandom().ToString();
  } while (info.id == kDefaultPlaylistID);

  base::Value::Dict playlist;
  playlist.Set(kPlaylistIDKey, info.id);
  playlist.Set(kPlaylistNameKey, info.name);
  playlist.Set(kPlaylistItemsKey, base::Value::List());

  prefs::ScopedDictionaryPrefUpdate playlists_update(prefs_, kPlaylistsPref);
  playlists_update.Get()->Set(
      info.id, std::make_unique<base::Value>(std::move(playlist)));

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kListCreated, info.id});
}

void PlaylistService::RemovePlaylist(const std::string& playlist_id) {
  if (playlist_id == kDefaultPlaylistID)
    return;

  DCHECK(!playlist_id.empty());
  std::unique_ptr<base::Value::List> id_list;
  {
    prefs::ScopedDictionaryPrefUpdate playlists_update(prefs_, kPlaylistsPref);
    std::unique_ptr<prefs::DictionaryValueUpdate> target_playlist_update;
    if (!playlists_update->GetDictionary(playlist_id,
                                         &target_playlist_update)) {
      LOG(ERROR) << __func__ << " Playlist " << playlist_id << " not found";
      return;
    }

    base::Value::List* item_ids = nullptr;
    if (!target_playlist_update->GetListWithoutPathExpansion(kPlaylistItemsKey,
                                                             &item_ids)) {
      NOTREACHED() << __func__ << " Playlist " << playlist_id
                   << " doesn't have |items| field";
      return;
    }

    id_list = std::make_unique<base::Value::List>(std::move(*item_ids));
    playlists_update->Remove(playlist_id);
  }

  // TODO(sko) Iterating this will cause a callback to be called a lot of
  // times.
  DCHECK(id_list);
  for (const auto& item_id : *id_list)
    DeletePlaylistItemData(item_id.GetString());

  NotifyPlaylistChanged(
      {PlaylistChangeParams::Type::kListRemoved, playlist_id});
}

std::vector<PlaylistItemInfo> PlaylistService::GetAllPlaylistItems() {
  std::vector<PlaylistItemInfo> items;
  for (const auto it : prefs_->GetDict(kPlaylistItemsPref)) {
    const auto& dict = it.second.GetDict();
    DCHECK(dict.contains(playlist::kPlaylistItemIDKey));
    DCHECK(dict.contains(playlist::kPlaylistItemTitleKey));
    DCHECK(dict.contains(playlist::kPlaylistItemPageSrcKey));
    DCHECK(dict.contains(playlist::kPlaylistItemMediaSrcKey));
    DCHECK(dict.contains(playlist::kPlaylistItemThumbnailSrcKey));
    DCHECK(dict.contains(playlist::kPlaylistItemMediaFilePathKey));
    DCHECK(dict.contains(playlist::kPlaylistItemThumbnailPathKey));
    DCHECK(dict.contains(playlist::kPlaylistItemMediaFileCachedKey));

    PlaylistItemInfo item;
    item.id = *dict.FindString(playlist::kPlaylistItemIDKey);
    item.title = *dict.FindString(playlist::kPlaylistItemTitleKey);
    item.page_src = *dict.FindString(playlist::kPlaylistItemPageSrcKey);
    item.thumbnail_src =
        *dict.FindString(playlist::kPlaylistItemThumbnailSrcKey);
    item.thumbnail_path =
        *dict.FindString(playlist::kPlaylistItemThumbnailPathKey);
    item.media_src = *dict.FindString(playlist::kPlaylistItemMediaSrcKey);
    item.media_file_path =
        *dict.FindString(playlist::kPlaylistItemMediaFilePathKey);
    item.media_file_cached =
        *dict.FindBool(playlist::kPlaylistItemMediaFileCachedKey);
    if (auto* duration = dict.Find(playlist::kPlaylistItemDurationKey)) {
      item.duration =
          base::ValueToTimeDelta(duration).value_or(base::TimeDelta());
    }
    if (auto* author = dict.FindString(playlist::kPlaylistItemAuthorKey))
      item.author = *author;
    items.push_back(std::move(item));
  }

  return items;
}

PlaylistItemInfo PlaylistService::GetPlaylistItem(const std::string& id) {
  DCHECK(!id.empty());
  const auto* item_value = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  DCHECK(item_value);
  if (!item_value)
    return {};

  DCHECK(item_value->contains(playlist::kPlaylistItemIDKey));
  DCHECK(item_value->contains(playlist::kPlaylistItemTitleKey));
  DCHECK(item_value->contains(playlist::kPlaylistItemPageSrcKey));
  DCHECK(item_value->contains(playlist::kPlaylistItemMediaSrcKey));
  DCHECK(item_value->contains(playlist::kPlaylistItemThumbnailSrcKey));
  DCHECK(item_value->contains(playlist::kPlaylistItemMediaFilePathKey));
  DCHECK(item_value->contains(playlist::kPlaylistItemThumbnailPathKey));
  DCHECK(item_value->contains(playlist::kPlaylistItemMediaFileCachedKey));

  PlaylistItemInfo item;
  item.id = *item_value->FindString(playlist::kPlaylistItemIDKey);
  item.title = *item_value->FindString(playlist::kPlaylistItemTitleKey);
  item.page_src = *item_value->FindString(playlist::kPlaylistItemPageSrcKey);
  item.thumbnail_src =
      *item_value->FindString(playlist::kPlaylistItemThumbnailSrcKey);
  item.thumbnail_path =
      *item_value->FindString(playlist::kPlaylistItemThumbnailPathKey);
  item.media_src = *item_value->FindString(playlist::kPlaylistItemMediaSrcKey);
  item.media_file_path =
      *item_value->FindString(playlist::kPlaylistItemMediaFilePathKey);
  item.media_file_cached =
      *item_value->FindBool(playlist::kPlaylistItemMediaFileCachedKey);
  if (auto* duration = item_value->Find(playlist::kPlaylistItemDurationKey))
    item.duration =
        base::ValueToTimeDelta(duration).value_or(base::TimeDelta());
  if (auto* author = item_value->FindString(playlist::kPlaylistItemAuthorKey))
    item.author = *author;

  return item;
}

absl::optional<PlaylistInfo> PlaylistService::GetPlaylist(
    const std::string& id) {
  const auto& playlists = prefs_->GetDict(kPlaylistsPref);
  if (!playlists.contains(id)) {
    LOG(ERROR) << __func__ << " playlist with id<" << id << "> not found";
    return {};
  }
  auto* playlist = playlists.FindDict(id);
  DCHECK(playlist);

  PlaylistInfo info;
  info.id = *playlist->FindString(kPlaylistIDKey);
  info.name = *playlist->FindString(kPlaylistNameKey);
  for (const auto& item_id_value : *playlist->FindList(kPlaylistItemsKey))
    info.items.push_back(GetPlaylistItem(item_id_value.GetString()));

  return info;
}

std::vector<PlaylistInfo> PlaylistService::GetAllPlaylists() {
  std::vector<PlaylistInfo> result;
  const auto& playlists = prefs_->GetDict(kPlaylistsPref);
  for (const auto [id, playlist_value] : playlists) {
    DCHECK(playlist_value.is_dict());
    const auto& playlist = playlist_value.GetDict();

    PlaylistInfo info;
    info.id = *playlist.FindString(kPlaylistIDKey);
    info.name = *playlist.FindString(kPlaylistNameKey);
    for (const auto& item_id_value : *playlist.FindList(kPlaylistItemsKey)) {
      info.items.push_back(GetPlaylistItem(item_id_value.GetString()));
    }
    result.push_back(std::move(info));
  }

  return result;
}

void PlaylistService::FindMediaFilesFromContents(
    content::WebContents* contents,
    FindMediaFilesCallback callback) {
  PlaylistDownloadRequestManager::Request request;
  request.url_or_contents = contents->GetWeakPtr();
  request.callback =
      base::BindOnce(std::move(callback), contents->GetWeakPtr());
  download_request_manager_->GetMediaFilesFromPage(std::move(request));
}

void PlaylistService::RecoverPlaylistItem(const std::string& id) {
  const auto* playlist_value = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  if (!playlist_value) {
    LOG(ERROR) << __func__ << ": Invalid playlist id for recovery: " << id;
    return;
  }

  auto cached = playlist_value->FindBool(kPlaylistItemMediaFileCachedKey);
  DCHECK(cached.has_value());
  if (cached.value()) {
    VLOG(2) << __func__ << ": This is ready to play(" << id << ")";
    return;
  }

  VLOG(2) << __func__ << ": This is in recovering playlist item(" << id << ")";

  PlaylistItemInfo info = GetPlaylistItem(id);
  DCHECK(!info.id.empty());

  auto on_check_if_path_exists = [](base::OnceClosure on_exists,
                                    base::OnceClosure on_not_exists,
                                    bool exists) {
    if (exists)
      std::move(on_exists).Run();
    else
      std::move(on_not_exists).Run();
  };

  auto on_path_exists = base::BindOnce(
      [](base::WeakPtr<PlaylistService> service, PlaylistItemInfo info) {
        service->OnPlaylistItemDirCreated(info, /* directory_created = */ true);
      },
      weak_factory_.GetWeakPtr(), info);

  auto on_path_not_exists = base::BindOnce(
      [](base::WeakPtr<PlaylistService> service, PlaylistItemInfo info) {
        if (!service)
          return;

        service->task_runner()->PostTaskAndReplyWithResult(
            FROM_HERE,
            base::BindOnce(&base::CreateDirectory,
                           service->GetPlaylistItemDirPath(info.id)),
            base::BindOnce(&PlaylistService::OnPlaylistItemDirCreated, service,
                           info));
      },
      weak_factory_.GetWeakPtr(), info);

  task_runner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&base::PathExists, GetPlaylistItemDirPath(id)),
      base::BindOnce(std::move(on_check_if_path_exists),
                     std::move(on_path_exists), std::move(on_path_not_exists)));
}

void PlaylistService::DeletePlaylistItemData(const std::string& id) {
  media_file_download_manager_->CancelDownloadRequest(id);
  thumbnail_downloader_->CancelDownloadRequest(id);

  RemovePlaylistItemValue(id);

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemDeleted, id});

  // TODO(simonhong): Delete after getting cancel complete message from all
  // downloader.
  // Delete assets from filesystem after updating db.
  task_runner()->PostTask(FROM_HERE, base::GetDeletePathRecursivelyCallback(
                                         GetPlaylistItemDirPath(id)));
}

void PlaylistService::DeletePlaylistLocalData(const std::string& id) {
  const auto* item_value_ptr = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  base::Value::Dict item = item_value_ptr->Clone();
  item.Set(kPlaylistItemMediaFileCachedKey, false);

  const auto* thumbnail_src = item.FindString(kPlaylistItemThumbnailSrcKey);
  item.Set(kPlaylistItemThumbnailPathKey,
           thumbnail_src ? *thumbnail_src : base::EmptyString());

  const auto* media_src = item.FindString(kPlaylistItemMediaSrcKey);
  DCHECK(media_src) << "media_src shouldn't be empty";
  item.Set(kPlaylistItemMediaFilePathKey, *media_src);
  UpdatePlaylistItemValue(id, base::Value(std::move(item)));

  NotifyPlaylistChanged(
      {PlaylistChangeParams::Type::kItemLocalDataRemoved, id});

  task_runner()->PostTask(FROM_HERE, base::GetDeletePathRecursivelyCallback(
                                         GetPlaylistItemDirPath(id)));
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

void PlaylistService::AddObserver(PlaylistServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void PlaylistService::RemoveObserver(PlaylistServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void PlaylistService::OnMediaFileReady(const std::string& id,
                                       const std::string& media_file_path) {
  VLOG(2) << __func__ << ": " << id << " " << media_file_path;
  DCHECK(IsValidPlaylistItem(id));

  const auto* item_value_ptr = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  base::Value::Dict item = item_value_ptr->Clone();
  item.Set(kPlaylistItemMediaFileCachedKey, true);
  item.Set(kPlaylistItemMediaFilePathKey, media_file_path);
  UpdatePlaylistItemValue(id, base::Value(std::move(item)));

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemCached, id});
}

void PlaylistService::OnMediaFileGenerationFailed(const std::string& id) {
  VLOG(2) << __func__ << ": " << id;

  DCHECK(IsValidPlaylistItem(id));

  const auto* item_value_ptr = prefs_->GetDict(kPlaylistItemsPref).FindDict(id);
  base::Value::Dict item = item_value_ptr->Clone();

  item.Set(kPlaylistItemMediaFileCachedKey, false);

  UpdatePlaylistItemValue(id, base::Value(std::move(item)));

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
    task_runner()->PostTask(FROM_HERE,
                            base::GetDeletePathRecursivelyCallback(path));
  }
}

void PlaylistService::CleanUpMalformedPlaylistItems() {
  if (base::ranges::none_of(
          prefs_->GetDict(kPlaylistItemsPref),
          /* has_malformed_data = */ [](const auto& pair) {
            auto* dict = pair.second.GetIfDict();
            DCHECK(dict);

            DCHECK(dict->contains(playlist::kPlaylistItemIDKey));

            // As of 2022. Sep., properties of PlaylistItemInfo was updated.
            return !dict->contains(playlist::kPlaylistItemPageSrcKey) ||
                   !dict->contains(playlist::kPlaylistItemMediaSrcKey) ||
                   !dict->contains(playlist::kPlaylistItemThumbnailSrcKey) ||
                   !dict->contains(playlist::kPlaylistItemMediaFileCachedKey);
          })) {
    return;
  }

  for (const auto* pref_key : {kPlaylistsPref, kPlaylistItemsPref})
    prefs_->ClearPref(pref_key);
}

void PlaylistService::CleanUpOrphanedPlaylistItemDirs() {
  base::flat_set<std::string> ids;
  base::ranges::transform(GetAllPlaylistItems(), std::inserter(ids, ids.end()),
                          [](const auto& item) {
                            DCHECK(!item.id.empty());
                            return item.id;
                          });

  task_runner()->PostTaskAndReplyWithResult(
      FROM_HERE, base::BindOnce(&GetOrphanedPaths, base_dir_, std::move(ids)),
      base::BindOnce(&PlaylistService::OnGetOrphanedPaths,
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

base::SequencedTaskRunner* PlaylistService::task_runner() {
  if (!task_runner_) {
    task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return task_runner_.get();
}

}  // namespace playlist
