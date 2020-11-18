/* Copyright (c) 2020 The Brave Authors. All rights reserved.
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
#include "base/task/post_task.h"
#include "base/task_runner_util.h"
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

const base::FilePath::StringType kBaseDirName(FILE_PATH_LITERAL("playlist"));

const base::FilePath::StringType kThumbnailFileName(
    FILE_PATH_LITERAL("thumbnail"));

void DeleteDir(const base::FilePath& path) {
  base::DeletePathRecursively(path);
}

PlaylistInfo CreatePlaylistInfo(const CreatePlaylistParams& params) {
  PlaylistInfo p;
  p.id = base::Token::CreateRandom().ToString();
  p.playlist_name = params.playlist_name;
  p.create_params = params;
  return p;
}

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

bool DoGenerateHTMLFileOnTaskRunner(const base::FilePath& html_file_path) {
  constexpr char kHTMLTemplate[] =
      "<video id='v' controls autoplay "
      "onplay='a=document.getElementById(\"a\");a.currentTime=this.currentTime;"
      "a.play();' onpause='a=document.getElementById(\"a\");a.pause()'><source "
      "src='video_file.mp4' type='video/mp4' /></video> <video id='a' autoplay "
      "style='display:none'><source src='audio_file.m4a' type='audio/mp4' "
      "/></video>";

  base::DeleteFile(html_file_path);
  return base::WriteFile(html_file_path, kHTMLTemplate);
}

}  // namespace

PlaylistService::PlaylistService(content::BrowserContext* context,
                                 PlaylistYoutubeDownComponentManager* manager)
    : base_dir_(context->GetPath().Append(kBaseDirName)),
      prefs_(user_prefs::UserPrefs::Get(context)),
      weak_factory_(this) {
  content::URLDataSource::Add(context,
                              std::make_unique<PlaylistDataSource>(this));
  media_file_download_manager_.reset(
      new PlaylistMediaFileDownloadManager(context, this, base_dir_));
  thumbnail_downloader_.reset(new PlaylistThumbnailDownloader(context, this));
  download_request_manager_.reset(
      new PlaylistDownloadRequestManager(context, this, manager));

  CleanUp();
}

PlaylistService::~PlaylistService() = default;

void PlaylistService::Shutdown() {
  download_request_manager_.reset();
}

void PlaylistService::RequestDownload(const std::string& url) {
  download_request_manager_->GeneratePlaylistCreateParamsForYoutubeURL(url);
}

void PlaylistService::OnPlaylistCreationParamsReady(
    const CreatePlaylistParams& params) {
  CreatePlaylistItem(params);
}

void PlaylistService::NotifyPlaylistChanged(
    const PlaylistChangeParams& params) {
  VLOG(2) << __func__ << ": params="
          << PlaylistChangeParams::GetPlaylistChangeTypeAsString(
                 params.change_type);

  for (PlaylistServiceObserver& obs : observers_)
    obs.OnPlaylistItemStatusChanged(params);
}

bool PlaylistService::HasPrefStorePlaylistItem(const std::string& id) const {
  const base::Value* playlist_info =
      prefs_->Get(kPlaylistItems)->FindDictKey(id);
  return !!playlist_info;
}

void PlaylistService::GenerateMediafileForPlaylistItem(const std::string& id) {
  const base::Value* playlist_info =
      prefs_->Get(kPlaylistItems)->FindDictKey(id);
  if (!playlist_info) {
    LOG(ERROR) << __func__ << ": Invalid playlist id for recover: " << id;
    return;
  }

  VLOG(2) << __func__;
  media_file_download_manager_->GenerateMediaFileForPlaylistItem(
      *playlist_info);
}

base::FilePath PlaylistService::GetPlaylistItemDirPath(
    const std::string& id) const {
  return base_dir_.AppendASCII(id);
}

void PlaylistService::UpdatePlaylistValue(const std::string& id,
                                          base::Value value) {
  prefs::ScopedDictionaryPrefUpdate update(prefs_, kPlaylistItems);
  auto playlist_items = update.Get();
  playlist_items->Set(id, base::Value::ToUniquePtrValue(std::move(value)));
}

void PlaylistService::RemovePlaylist(const std::string& id) {
  prefs::ScopedDictionaryPrefUpdate update(prefs_, kPlaylistItems);
  auto playlist_items = update.Get();
  playlist_items->Remove(id, nullptr);
}

void PlaylistService::CreatePlaylistItem(const CreatePlaylistParams& params) {
  VLOG(2) << __func__;
  const PlaylistInfo info = CreatePlaylistInfo(params);
  UpdatePlaylistValue(info.id, GetValueFromPlaylistInfo(info));

  NotifyPlaylistChanged(
      {PlaylistChangeParams::ChangeType::kChangeTypeAdded, info.id});

  base::PostTaskAndReplyWithResult(
      task_runner(), FROM_HERE,
      base::BindOnce(&base::CreateDirectory, GetPlaylistItemDirPath(info.id)),
      base::BindOnce(&PlaylistService::OnPlaylistItemDirCreated,
                     weak_factory_.GetWeakPtr(), info.id));
}

void PlaylistService::OnPlaylistItemDirCreated(const std::string& id,
                                               bool directory_ready) {
  VLOG(2) << __func__;
  if (!directory_ready) {
    NotifyPlaylistChanged(
        {PlaylistChangeParams::ChangeType::kChangeTypeAborted, id});
    return;
  }

  DownloadThumbnail(id);
  GenerateMediafileForPlaylistItem(id);
}

void PlaylistService::DownloadThumbnail(const std::string& id) {
  const base::Value* item_value = prefs_->Get(kPlaylistItems)->FindDictKey(id);
  DCHECK(item_value);
  const base::Value* create_params_value =
      item_value->FindDictKey(kPlaylistCreateParamsKey);
  DCHECK(create_params_value);
  const std::string* thumbnail_url =
      create_params_value->FindStringKey(kPlaylistPlaylistThumbnailUrlKey);
  if (!thumbnail_url || thumbnail_url->empty()) {
    VLOG(2) << __func__ << ": thumbnail url is not available.";
    return;
  }

  thumbnail_downloader_->DownloadThumbnail(
      id, GURL(*thumbnail_url),
      GetPlaylistItemDirPath(id).Append(kThumbnailFileName));
}

void PlaylistService::OnThumbnailDownloaded(const std::string& id,
                                            const base::FilePath& path) {
  DCHECK(IsValidPlaylistItem(id));

  if (path.empty()) {
    VLOG(2) << __func__ << ": thumbnail fetching failed for " << id;
    NotifyPlaylistChanged(
        {PlaylistChangeParams::ChangeType::kChangeTypeThumbnailFailed, id});
    return;
  }

  const base::Value* value = prefs_->Get(kPlaylistItems)->FindDictKey(id);
  DCHECK(value);
  if (value) {
    base::Value copied_value = value->Clone();
    copied_value.SetStringKey(kPlaylistThumbnailPathKey, path.AsUTF8Unsafe());
    UpdatePlaylistValue(id, std::move(copied_value));
    NotifyPlaylistChanged(
        {PlaylistChangeParams::ChangeType::kChangeTypeThumbnailReady, id});
  }
}

base::Value PlaylistService::GetAllPlaylistItems() {
  base::Value playlist(base::Value::Type::LIST);
  for (const auto& it : prefs_->Get(kPlaylistItems)->DictItems()) {
    base::Value item = it.second.Clone();
    item.RemoveKey(kPlaylistCreateParamsKey);
    playlist.Append(std::move(item));
  }

  return playlist;
}

base::Value PlaylistService::GetPlaylistItem(const std::string& id) {
  const base::Value* item_value_ptr =
      prefs_->Get(kPlaylistItems)->FindDictKey(id);
  if (item_value_ptr) {
    base::Value item = item_value_ptr->Clone();
    item.RemoveKey(kPlaylistCreateParamsKey);
    return item;
  }
  return {};
}

void PlaylistService::RecoverPlaylistItem(const std::string& id) {
  const base::Value* playlist_info =
      prefs_->Get(kPlaylistItems)->FindDictKey(id);
  if (!playlist_info) {
    LOG(ERROR) << __func__ << ": Invalid playlist id for recover: " << id;
    return;
  }
  base::Optional<bool> ready = playlist_info->FindBoolPath(kPlaylistReadyKey);
  if (*ready) {
    VLOG(2) << __func__ << ": This is ready to play(" << id << ")";
    return;
  }

  VLOG(2) << __func__ << ": This is in recovering playlist item(" << id << ")";

  const std::string* thumbnail_path_str =
      playlist_info->FindStringPath(kPlaylistThumbnailPathKey);
  const bool has_thumbnail =
      !!thumbnail_path_str && !thumbnail_path_str->empty();
  if (!has_thumbnail)
    DownloadThumbnail(id);

  const std::string* video_media_file_path =
      playlist_info->FindStringPath(kPlaylistVideoMediaFilePathKey);
  const std::string* audio_media_file_path =
      playlist_info->FindStringPath(kPlaylistAudioMediaFilePathKey);
  // Only try to regenerate if partial ready or there is no media file.
  if (!video_media_file_path || video_media_file_path->empty() ||
      !audio_media_file_path || audio_media_file_path->empty()) {
    VLOG(2) << __func__ << ": Regenerate media file";
    GenerateMediafileForPlaylistItem(id);
  }
}

void PlaylistService::DeletePlaylistItem(const std::string& id) {
  media_file_download_manager_->CancelDownloadRequest(id);
  thumbnail_downloader_->CancelDownloadRequest(id);
  RemovePlaylist(id);

  NotifyPlaylistChanged(
      {PlaylistChangeParams::ChangeType::kChangeTypeDeleted, id});

  // TODO(simonhong): Delete after getting cancel complete message from all
  // downloader.
  // Delete assets from filesystem after updating db.
  task_runner()->PostTask(
      FROM_HERE, base::BindOnce(&DeleteDir, GetPlaylistItemDirPath(id)));
}

void PlaylistService::DeleteAllPlaylistItems() {
  VLOG(2) << __func__;

  // Cancel currently generated playlist if needed and pending thumbnail
  // download jobs.
  media_file_download_manager_->CancelAllDownloadRequests();
  thumbnail_downloader_->CancelAllDownloadRequests();

  prefs_->ClearPref(kPlaylistItems);

  NotifyPlaylistChanged(
      {PlaylistChangeParams::ChangeType::kChangeTypeAllDeleted, ""});

  CleanUp();
}

void PlaylistService::AddObserver(PlaylistServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void PlaylistService::RemoveObserver(PlaylistServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

void PlaylistService::OnMediaFileReady(const std::string& id,
                                       const std::string& audio_file_path,
                                       const std::string& video_file_path) {
  VLOG(2) << __func__ << ": " << id;
  DCHECK(IsValidPlaylistItem(id));

  const base::Value* item_value_ptr =
      prefs_->Get(kPlaylistItems)->FindDictKey(id);
  base::Value item = item_value_ptr->Clone();

  item.SetBoolKey(kPlaylistReadyKey, true);
  item.SetStringKey(kPlaylistAudioMediaFilePathKey, audio_file_path);
  item.SetStringKey(kPlaylistVideoMediaFilePathKey, video_file_path);
  UpdatePlaylistValue(id, std::move(item));

  NotifyPlaylistChanged(
      {PlaylistChangeParams::ChangeType::kChangeTypePlayReady, id});

  GenerateIndexHTMLFile(GetPlaylistItemDirPath(id));
}

void PlaylistService::OnMediaFileGenerationFailed(const std::string& id) {
  VLOG(2) << __func__ << ": " << id;

  DCHECK(IsValidPlaylistItem(id));

  const base::Value* item_value_ptr =
      prefs_->Get(kPlaylistItems)->FindDictKey(id);
  base::Value item = item_value_ptr->Clone();

  item.SetBoolKey(kPlaylistReadyKey, false);
  item.SetStringKey(kPlaylistAudioMediaFilePathKey, "");
  item.SetStringKey(kPlaylistVideoMediaFilePathKey, "");

  UpdatePlaylistValue(id, std::move(item));

  thumbnail_downloader_->CancelDownloadRequest(id);
  NotifyPlaylistChanged(
      {PlaylistChangeParams::ChangeType::kChangeTypeAborted, id});
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
    task_runner()->PostTask(FROM_HERE, base::BindOnce(&DeleteDir, path));
  }
}

void PlaylistService::CleanUp() {
  base::Value playlist = GetAllPlaylistItems();

  base::flat_set<std::string> ids;
  for (const auto& item : playlist.GetList()) {
    const std::string* id = item.FindStringKey(kPlaylistIDKey);
    DCHECK(id);
    if (id)
      ids.insert(*id);
  }

  base::PostTaskAndReplyWithResult(
      task_runner(), FROM_HERE,
      base::BindOnce(&GetOrphanedPaths, base_dir_, ids),
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

void PlaylistService::GenerateIndexHTMLFile(
    const base::FilePath& playlist_path) {
  auto html_file_path = playlist_path.Append(FILE_PATH_LITERAL("index.html"));
  base::PostTaskAndReplyWithResult(
      task_runner(), FROM_HERE,
      base::BindOnce(&DoGenerateHTMLFileOnTaskRunner, html_file_path),
      base::BindOnce(&PlaylistService::OnHTMLFileGenerated,
                     weak_factory_.GetWeakPtr()));
}

void PlaylistService::OnHTMLFileGenerated(bool generated) {
  if (!generated)
    LOG(ERROR) << "couldn't create HTML file for play";
}

base::SequencedTaskRunner* PlaylistService::task_runner() {
  if (!task_runner_) {
    task_runner_ = base::CreateSequencedTaskRunner(
        {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return task_runner_.get();
}

}  // namespace playlist
