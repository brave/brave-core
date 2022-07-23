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
      std::make_unique<PlaylistDownloadRequestManager>(context, this, manager);
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

void PlaylistService::RequestDownloadMediaFilesFromPage(
    const std::string& playlist_id,
    const std::string& url) {
  // TODO(sko) consider |playlist_id|
  download_request_manager_->GetMediaFilesFromPage(url);
}

void PlaylistService::RemoveItemFromPlaylist(const std::string& playlist_id,
                                             const std::string& item_id) {
  VLOG(2) << __func__ << " " << playlist_id << " " << item_id;
  // TODO(sko) consider |playlist_id|
  DeletePlaylistItem(item_id);
}

void PlaylistService::OnPlaylistCreationParamsReady(
    const PlaylistItemInfo& params) {
  CreatePlaylistItem(params);
}

void PlaylistService::NotifyPlaylistItemChanged(
    const PlaylistItemChangeParams& params) {
  VLOG(2) << __func__ << ": params="
          << PlaylistItemChangeParams::GetPlaylistChangeTypeAsString(
                 params.change_type);

  for (PlaylistServiceObserver& obs : observers_)
    obs.OnPlaylistItemStatusChanged(params);
}

bool PlaylistService::HasPrefStorePlaylistItem(const std::string& id) const {
  auto* items = prefs_->Get(kPlaylistItemsPref);
  DCHECK(items);
  const base::Value* playlist_info = items->FindDictKey(id);
  return !!playlist_info;
}

void PlaylistService::GenerateMediafileForPlaylistItem(
    const PlaylistItemInfo& info) {
  VLOG(2) << __func__;
  media_file_download_manager_->GenerateMediaFileForPlaylistItem(info);
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

void PlaylistService::RemovePlaylistItem(const std::string& id) {
  prefs::ScopedDictionaryPrefUpdate update(prefs_, kPlaylistItemsPref);
  auto playlist_items = update.Get();
  playlist_items->Remove(id);
}

void PlaylistService::CreatePlaylistItem(const PlaylistItemInfo& params) {
  VLOG(2) << __func__;

  PlaylistItemInfo info = params;
  info.id = base::Token::CreateRandom().ToString();

  UpdatePlaylistItemValue(info.id, GetValueFromPlaylistItemInfo(info));

  NotifyPlaylistItemChanged({PlaylistItemChangeParams::Type::kAdded, info.id});

  task_runner()->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(&base::CreateDirectory, GetPlaylistItemDirPath(info.id)),
      base::BindOnce(&PlaylistService::OnPlaylistItemDirCreated,
                     weak_factory_.GetWeakPtr(), info));
}

void PlaylistService::OnPlaylistItemDirCreated(const PlaylistItemInfo& info,
                                               bool directory_ready) {
  VLOG(2) << __func__;
  if (!directory_ready) {
    NotifyPlaylistItemChanged(
        {PlaylistItemChangeParams::Type::kAborted, info.id});
    return;
  }

  DownloadThumbnail(info);
  GenerateMediafileForPlaylistItem(info);
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
    NotifyPlaylistItemChanged(
        {PlaylistItemChangeParams::Type::kThumbnailFailed, id});
    return;
  }

  const base::Value* value = prefs_->Get(kPlaylistItemsPref)->FindDictKey(id);
  DCHECK(value);
  if (value) {
    base::Value copied_value = value->Clone();
    copied_value.SetStringKey(kPlaylistItemThumbnailPathKey,
                              path.AsUTF8Unsafe());
    UpdatePlaylistItemValue(id, std::move(copied_value));
    NotifyPlaylistItemChanged(
        {PlaylistItemChangeParams::Type::kThumbnailReady, id});
  }
}

base::Value PlaylistService::GetAllPlaylistItems() {
  base::Value playlist(base::Value::Type::LIST);
  for (const auto it : prefs_->Get(kPlaylistItemsPref)->GetDict())
    playlist.Append(it.second.Clone());

  return playlist;
}

base::Value PlaylistService::GetPlaylistItem(const std::string& id) {
  if (const base::Value* item_value_ptr =
          prefs_->Get(kPlaylistItemsPref)->FindDictKey(id)) {
    return item_value_ptr->Clone();
  }

  return {};
}

void PlaylistService::RecoverPlaylistItem(const std::string& id) {
  const base::Value* playlist_value =
      prefs_->Get(kPlaylistItemsPref)->FindDictKey(id);
  if (!playlist_value) {
    LOG(ERROR) << __func__ << ": Invalid playlist id for recover: " << id;
    return;
  }

  absl::optional<bool> ready =
      playlist_value->FindBoolPath(kPlaylistItemReadyKey);
  if (*ready) {
    VLOG(2) << __func__ << ": This is ready to play(" << id << ")";
    return;
  }

  VLOG(2) << __func__ << ": This is in recovering playlist item(" << id << ")";

  PlaylistItemInfo info;
  info.id = *playlist_value->FindStringKey(kPlaylistItemIDKey);
  info.title = *playlist_value->FindStringKey(kPlaylistItemTitleKey);

  const std::string* thumbnail_path_str =
      playlist_value->FindStringPath(kPlaylistItemThumbnailPathKey);
  if (thumbnail_path_str)
    info.thumbnail_path = *thumbnail_path_str;

  const std::string* media_file_path =
      playlist_value->FindStringPath(kPlaylistItemMediaFilePathKey);
  if (media_file_path)
    info.media_file_path = *media_file_path;

  if (thumbnail_path_str && !thumbnail_path_str->empty()) {
    VLOG(2) << __func__ << ": Regenerate thumbnail";
    DownloadThumbnail(info);
  }

  if (media_file_path && !media_file_path->empty()) {
    VLOG(2) << __func__ << ": Regenerate media file";
    GenerateMediafileForPlaylistItem(info);
  }
}

void PlaylistService::DeletePlaylistItem(const std::string& id) {
  media_file_download_manager_->CancelDownloadRequest(id);
  thumbnail_downloader_->CancelDownloadRequest(id);
  RemovePlaylistItem(id);

  NotifyPlaylistItemChanged({PlaylistItemChangeParams::Type::kDeleted, id});

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

  NotifyPlaylistItemChanged({PlaylistItemChangeParams::Type::kAllDeleted, ""});

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

  const base::Value* item_value_ptr =
      prefs_->Get(kPlaylistItemsPref)->FindDictKey(id);
  base::Value item = item_value_ptr->Clone();

  item.SetBoolKey(kPlaylistItemReadyKey, true);
  item.SetStringKey(kPlaylistItemMediaFilePathKey, media_file_path);
  UpdatePlaylistItemValue(id, std::move(item));

  NotifyPlaylistItemChanged({PlaylistItemChangeParams::Type::kPlayReady, id});
}

void PlaylistService::OnMediaFileGenerationFailed(const std::string& id) {
  VLOG(2) << __func__ << ": " << id;

  DCHECK(IsValidPlaylistItem(id));

  const base::Value* item_value_ptr =
      prefs_->Get(kPlaylistItemsPref)->FindDictKey(id);
  base::Value item = item_value_ptr->Clone();

  item.SetBoolKey(kPlaylistItemReadyKey, false);

  UpdatePlaylistItemValue(id, std::move(item));

  thumbnail_downloader_->CancelDownloadRequest(id);
  NotifyPlaylistItemChanged({PlaylistItemChangeParams::Type::kAborted, id});
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
  base::Value playlist = GetAllPlaylistItems();

  base::flat_set<std::string> ids;
  for (const auto& item : playlist.GetList()) {
    const std::string* id = item.FindStringKey(kPlaylistItemIDKey);
    DCHECK(id);
    if (id)
      ids.insert(*id);
  }

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
