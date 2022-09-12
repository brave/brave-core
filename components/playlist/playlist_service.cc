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
  CleanUp();
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
  VLOG(2) << __func__
          << " download media from WebContents to playlist: " << playlist_id;

  DCHECK(contents);

  PlaylistDownloadRequestManager::Request request;
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

void PlaylistService::RemoveItemFromPlaylist(const std::string& playlist_id,
                                             const std::string& item_id) {
  VLOG(2) << __func__ << " " << playlist_id << " " << item_id;

  DCHECK(!item_id.empty());

  {
    prefs::ScopedDictionaryPrefUpdate playlists_update(prefs_, kPlaylistsPref);
    std::unique_ptr<prefs::DictionaryValueUpdate> a_playlist_update;
    if (!playlists_update->GetDictionary(
            playlist_id.empty() ? kDefaultPlaylistID : playlist_id,
            &a_playlist_update)) {
      LOG(ERROR) << __func__ << " Playlist " << playlist_id << " not found";
      return;
    }

    base::ListValue* item_ids = nullptr;
    if (!a_playlist_update->GetList(kPlaylistItemsKey, &item_ids)) {
      NOTREACHED() << __func__ << " Playlist " << playlist_id
                   << " doesn't have |items| field";
      return;
    }

    auto& id_list = item_ids->GetList();
    auto it = base::ranges::find_if(id_list, [&item_id](const auto& id) {
      return id.GetString() == item_id;
    });
    if (it == id_list.end())
      return;

    id_list.erase(it);

    a_playlist_update->Set(kPlaylistItemsKey,
                           base::Value::ToUniquePtrValue(std::move(*item_ids)));
  }

  // TODO(sko) Once we can support to share an item between playlists, we should
  // check if other playlists have this item before deleting this item
  // permanantly.
  DeletePlaylistItem(item_id);
}

void PlaylistService::RequestDownloadMediaFilesFromItems(
    const std::string& playlist_id,
    const std::vector<PlaylistItemInfo>& params) {
  if (params.empty())
    return;

  {
    prefs::ScopedDictionaryPrefUpdate playlists_update(prefs_, kPlaylistsPref);
    std::unique_ptr<prefs::DictionaryValueUpdate> a_playlist_update;
    if (!playlists_update->GetDictionary(playlist_id, &a_playlist_update)) {
      LOG(ERROR) << __func__ << " Playlist " << playlist_id << " not found";
      return;
    }

    base::ListValue* item_ids = nullptr;
    if (!a_playlist_update->GetList(kPlaylistItemsKey, &item_ids)) {
      NOTREACHED() << __func__ << " Playlist " << playlist_id
                   << " doesn't have |items| field";
      return;
    }

    for (const auto& item : params) {
      DCHECK(!item.id.empty());
      item_ids->Append(item.id);
    }

    a_playlist_update->Set(kPlaylistItemsKey,
                           base::Value::ToUniquePtrValue(std::move(*item_ids)));
  }

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
  const base::Value::Dict& items = prefs_->GetValueDict(kPlaylistItemsPref);
  const base::Value::Dict* playlist_info = items.FindDict(id);
  return !!playlist_info;
}

void PlaylistService::DownloadMediaFile(const PlaylistItemInfo& info) {
  VLOG(2) << __func__;
  media_file_download_manager_->DownloadMediaFile(info);
}

base::FilePath PlaylistService::GetPlaylistItemDirPath(
    const std::string& id) const {
  return base_dir_.AppendASCII(id);
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
  VLOG(2) << __func__ << " " << info.thumbnail_path;

  if (GURL thumbnail_url(info.thumbnail_path);
      thumbnail_url.is_valid() && !thumbnail_url.SchemeIsFile()) {
    thumbnail_downloader_->DownloadThumbnail(
        info.id, thumbnail_url,
        GetPlaylistItemDirPath(info.id).Append(kThumbnailFileName));
  }
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

  const base::Value::Dict* value =
      prefs_->GetValueDict(kPlaylistItemsPref).FindDict(id);
  DCHECK(value);
  if (value) {
    base::Value::Dict copied_value = value->Clone();
    copied_value.Set(kPlaylistItemThumbnailPathKey, path.AsUTF8Unsafe());
    UpdatePlaylistItemValue(id, base::Value(std::move(copied_value)));
    NotifyPlaylistChanged(
        {PlaylistChangeParams::Type::kItemThumbnailReady, id});
  }
}

void PlaylistService::CreatePlaylist(const PlaylistInfo& info) {
  std::string id;
  do {
    id = base::Token::CreateRandom().ToString();
  } while (id == kDefaultPlaylistID);

  base::Value::Dict playlist;
  playlist.Set(kPlaylistIDKey, id);
  playlist.Set(kPlaylistNameKey, info.name);
  playlist.Set(kPlaylistItemsKey, base::Value::List());

  prefs::ScopedDictionaryPrefUpdate playlists_update(prefs_, kPlaylistsPref);
  playlists_update.Get()->Set(
      id, std::make_unique<base::Value>(std::move(playlist)));

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kListCreated, id});
}

void PlaylistService::RemovePlaylist(const std::string& playlist_id) {
  if (playlist_id == kDefaultPlaylistID)
    return;

  DCHECK(!playlist_id.empty());
  std::unique_ptr<base::Value::List> id_list;
  {
    prefs::ScopedDictionaryPrefUpdate playlists_update(prefs_, kPlaylistsPref);
    std::unique_ptr<prefs::DictionaryValueUpdate> a_playlist_update;
    if (!playlists_update->GetDictionary(playlist_id, &a_playlist_update)) {
      LOG(ERROR) << __func__ << " Playlist " << playlist_id << " not found";
      return;
    }

    base::ListValue* item_ids = nullptr;
    if (!a_playlist_update->GetList(kPlaylistItemsKey, &item_ids)) {
      NOTREACHED() << __func__ << " Playlist " << playlist_id
                   << " doesn't have |items| field";
      return;
    }

    id_list =
        std::make_unique<base::Value::List>(std::move(item_ids->GetList()));
    playlists_update->Remove(playlist_id);
  }

  // TODO(sko) Iterating this will cause a callback to be called a lot of
  // times.
  DCHECK(id_list);
  for (const auto& item_id : *id_list)
    DeletePlaylistItem(item_id.GetString());

  NotifyPlaylistChanged(
      {PlaylistChangeParams::Type::kListRemoved, playlist_id});
}

std::vector<PlaylistItemInfo> PlaylistService::GetAllPlaylistItems() {
  std::vector<PlaylistItemInfo> items;
  for (const auto it : prefs_->GetValueDict(kPlaylistItemsPref)) {
    const base::Value::Dict& dict = it.second.GetDict();
    DCHECK(dict.contains(playlist::kPlaylistItemIDKey));
    DCHECK(dict.contains(playlist::kPlaylistItemTitleKey));
    DCHECK(dict.contains(playlist::kPlaylistItemMediaFilePathKey));
    DCHECK(dict.contains(playlist::kPlaylistItemThumbnailPathKey));
    DCHECK(dict.contains(playlist::kPlaylistItemReadyKey));

    PlaylistItemInfo item;
    item.id = *dict.FindString(playlist::kPlaylistItemIDKey);
    item.title = *dict.FindString(playlist::kPlaylistItemTitleKey);
    item.thumbnail_path =
        *dict.FindString(playlist::kPlaylistItemThumbnailPathKey);
    item.media_file_path =
        *dict.FindString(playlist::kPlaylistItemMediaFilePathKey);
    item.ready = *dict.FindBool(playlist::kPlaylistItemReadyKey);
    items.push_back(std::move(item));
  }

  return items;
}

PlaylistItemInfo PlaylistService::GetPlaylistItem(const std::string& id) {
  DCHECK(!id.empty());
  auto* item_value = prefs_->GetValueDict(kPlaylistItemsPref).FindDict(id);
  DCHECK(item_value);
  if (!item_value)
    return {};

  DCHECK(item_value->contains(playlist::kPlaylistItemIDKey));
  DCHECK(item_value->contains(playlist::kPlaylistItemTitleKey));
  DCHECK(item_value->contains(playlist::kPlaylistItemMediaFilePathKey));
  DCHECK(item_value->contains(playlist::kPlaylistItemThumbnailPathKey));
  DCHECK(item_value->contains(playlist::kPlaylistItemReadyKey));

  PlaylistItemInfo item;
  item.id = *item_value->FindString(playlist::kPlaylistItemIDKey);
  item.title = *item_value->FindString(playlist::kPlaylistItemTitleKey);
  item.thumbnail_path =
      *item_value->FindString(playlist::kPlaylistItemThumbnailPathKey);
  item.media_file_path =
      *item_value->FindString(playlist::kPlaylistItemMediaFilePathKey);
  item.ready = *item_value->FindBool(playlist::kPlaylistItemReadyKey);

  return item;
}

absl::optional<PlaylistInfo> PlaylistService::GetPlaylist(
    const std::string& id) {
  const base::Value::Dict& playlists = prefs_->GetValueDict(kPlaylistsPref);
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
  const base::Value::Dict& playlists = prefs_->GetValueDict(kPlaylistsPref);
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
  const base::Value::Dict* playlist_value =
      prefs_->GetValueDict(kPlaylistItemsPref).FindDict(id);
  if (!playlist_value) {
    LOG(ERROR) << __func__ << ": Invalid playlist id for recover: " << id;
    return;
  }

  bool ready = playlist_value->FindBool(kPlaylistItemReadyKey).value_or(true);
  if (ready) {
    VLOG(2) << __func__ << ": This is ready to play(" << id << ")";
    return;
  }

  VLOG(2) << __func__ << ": This is in recovering playlist item(" << id << ")";

  PlaylistItemInfo info;
  info.id = *playlist_value->FindString(kPlaylistItemIDKey);
  info.title = *playlist_value->FindString(kPlaylistItemTitleKey);

  const std::string* thumbnail_path_str =
      playlist_value->FindString(kPlaylistItemThumbnailPathKey);
  if (thumbnail_path_str)
    info.thumbnail_path = *thumbnail_path_str;

  const std::string* media_file_path =
      playlist_value->FindString(kPlaylistItemMediaFilePathKey);
  if (media_file_path)
    info.media_file_path = *media_file_path;

  if (thumbnail_path_str && !thumbnail_path_str->empty()) {
    VLOG(2) << __func__ << ": Regenerate thumbnail";
    DownloadThumbnail(info);
  }

  if (media_file_path && !media_file_path->empty()) {
    VLOG(2) << __func__ << ": Regenerate media file";
    DownloadMediaFile(info);
  }
}

void PlaylistService::DeletePlaylistItem(const std::string& id) {
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

void PlaylistService::DeleteAllPlaylistItems() {
  VLOG(2) << __func__;

  // Cancel currently generated playlist if needed and pending thumbnail
  // download jobs.
  media_file_download_manager_->CancelAllDownloadRequests();
  thumbnail_downloader_->CancelAllDownloadRequests();

  prefs_->ClearPref(kPlaylistItemsPref);

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kAllDeleted, ""});

  CleanUp();
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

  const base::Value::Dict* item_value_ptr =
      prefs_->GetValueDict(kPlaylistItemsPref).FindDict(id);
  base::Value::Dict item = item_value_ptr->Clone();
  item.Set(kPlaylistItemReadyKey, true);
  item.Set(kPlaylistItemMediaFilePathKey, media_file_path);
  UpdatePlaylistItemValue(id, base::Value(std::move(item)));

  NotifyPlaylistChanged({PlaylistChangeParams::Type::kItemPlayReady, id});
}

void PlaylistService::OnMediaFileGenerationFailed(const std::string& id) {
  VLOG(2) << __func__ << ": " << id;

  DCHECK(IsValidPlaylistItem(id));

  const base::Value::Dict* item_value_ptr =
      prefs_->GetValueDict(kPlaylistItemsPref).FindDict(id);
  base::Value::Dict item = item_value_ptr->Clone();

  item.Set(kPlaylistItemReadyKey, false);

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

void PlaylistService::CleanUp() {
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

base::SequencedTaskRunner* PlaylistService::task_runner() {
  if (!task_runner_) {
    task_runner_ = base::ThreadPool::CreateSequencedTaskRunner(
        {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return task_runner_.get();
}

}  // namespace playlist
