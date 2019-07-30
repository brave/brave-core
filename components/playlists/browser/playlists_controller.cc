/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/playlists/browser/playlists_controller.h"

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
#include "brave/components/playlists/browser/playlists_constants.h"
#include "brave/components/playlists/browser/playlists_controller_observer.h"
#include "brave/components/playlists/browser/playlists_db_controller.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"

namespace {

constexpr unsigned int kRetriesCountOnNetworkChange = 1;
const base::FilePath::StringType kDatabaseDirName(
    FILE_PATH_LITERAL("playlists_db"));
const base::FilePath::StringType kThumbnailFileName(
    FILE_PATH_LITERAL("thumbnail"));

base::FilePath::StringType GetPlaylistIDDirName(
    const std::string& playlist_id) {
#if defined(OS_WIN)
  return base::UTF8ToUTF16(playlist_id);
#else
  return playlist_id;
#endif
}

PlaylistInfo CreatePlaylistInfo(const CreatePlaylistParams& params) {
  PlaylistInfo p;
  p.id = base::Token::CreateRandom().ToString();
  p.playlist_name = params.playlist_name;
  p.create_params = params;
  return p;
}

base::Value GetValueFromMediaFile(const MediaFileInfo& info) {
  base::Value media_file(base::Value::Type::DICTIONARY);
  media_file.SetStringKey(kPlaylistsMediaFileUrlKey, info.media_file_url);
  media_file.SetStringKey(kPlaylistsMediaFileTitleKey, info.media_file_title);
  return media_file;
}

base::Value GetValueFromMediaFiles(
    const std::vector<MediaFileInfo>& media_files) {
  base::Value media_files_value(base::Value::Type::LIST);
  for (const MediaFileInfo& info : media_files)
    media_files_value.GetList().push_back(GetValueFromMediaFile(info));
  return media_files_value;
}

base::Value GetValueFromCreateParams(const CreatePlaylistParams& params) {
  base::Value create_params_value(base::Value::Type::DICTIONARY);
  create_params_value.SetStringKey(kPlaylistsPlaylistThumbnailUrlKey,
                                   params.playlist_thumbnail_url);
  create_params_value.SetStringKey(kPlaylistsPlaylistNameKey,
                                   params.playlist_name);
  create_params_value.SetKey(kPlaylistsVideoMediaFilesKey,
                             GetValueFromMediaFiles(params.video_media_files));
  create_params_value.SetKey(kPlaylistsAudioMediaFilesKey,
                             GetValueFromMediaFiles(params.audio_media_files));
  return create_params_value;
}

base::Value GetTitleValueFromCreateParams(const CreatePlaylistParams& params) {
  base::Value titles_value(base::Value::Type::LIST);
  for (const MediaFileInfo& info : params.video_media_files)
    titles_value.GetList().emplace_back(info.media_file_title);
  return titles_value;
}

base::Value GetValueFromPlaylistInfo(const PlaylistInfo& info) {
  base::Value playlist_value(base::Value::Type::DICTIONARY);
  playlist_value.SetStringKey(kPlaylistsIDKey, info.id);
  playlist_value.SetStringKey(kPlaylistsPlaylistNameKey, info.playlist_name);
  playlist_value.SetStringKey(kPlaylistsThumbnailPathKey, info.thumbnail_path);
  playlist_value.SetStringKey(kPlaylistsVideoMediaFilePathKey,
                              info.video_media_file_path);
  playlist_value.SetStringKey(kPlaylistsAudioMediaFilePathKey,
                              info.audio_media_file_path);
  playlist_value.SetBoolKey(kPlaylistsPartialReadyKey, info.partial_ready);
  playlist_value.SetKey(kPlaylistsTitlesKey,
                        GetTitleValueFromCreateParams(info.create_params));
  playlist_value.SetKey(kPlaylistsCreateParamsKey,
                        GetValueFromCreateParams(info.create_params));
  return playlist_value;
}

net::NetworkTrafficAnnotationTag GetNetworkTrafficAnnotationTagForURLLoad() {
  return net::DefineNetworkTrafficAnnotation("playlists_controller", R"(
      semantics {
        sender: "Brave Playlists Controller"
        description:
          "Fetching thumbnail image for newly created playlist"
        trigger:
          "User-initiated for creating new playlists "
        data:
          "Thumbnail for playlist"
        destination: WEBSITE
      }
      policy {
        cookies_allowed: NO
      })");
}

base::Value GetPlaylistValueFromPlaylistInfoJSON(
    const std::string& playlist_info_json) {
  base::Value playlist(base::Value::Type::DICTIONARY);
  if (playlist_info_json.empty())
    return playlist;

  base::Optional<base::Value> playlist_info =
      base::JSONReader::Read(playlist_info_json);
  if (!playlist_info)
    return playlist;

  const std::string* id = playlist_info->FindStringKey(kPlaylistsIDKey);
  if (!id)
    return playlist;
  playlist.SetStringKey(kPlaylistsIDKey, std::move(*id));

  const std::string* name =
      playlist_info->FindStringKey(kPlaylistsPlaylistNameKey);
  if (name)
    playlist.SetStringKey(kPlaylistsPlaylistNameKey, std::move(*name));

  base::Value* title = playlist_info->FindListKey(kPlaylistsTitlesKey);
  if (title)
    playlist.SetKey(kPlaylistsTitlesKey, std::move(*title));

  const std::string* thumbnail =
      playlist_info->FindStringKey(kPlaylistsThumbnailPathKey);
  if (thumbnail)
    playlist.SetStringKey(kPlaylistsThumbnailPathKey, std::move(*thumbnail));

  const std::string* video_media_files =
      playlist_info->FindStringKey(kPlaylistsVideoMediaFilePathKey);
  if (video_media_files)
    playlist.SetStringKey(kPlaylistsVideoMediaFilePathKey,
                          std::move(*video_media_files));

  const std::string* audio_media_files =
      playlist_info->FindStringKey(kPlaylistsAudioMediaFilePathKey);
  if (audio_media_files)
    playlist.SetStringKey(kPlaylistsAudioMediaFilePathKey,
                          std::move(*audio_media_files));

  base::Optional<bool> partial_ready =
      playlist_info->FindBoolKey(kPlaylistsPartialReadyKey);
  if (partial_ready != base::nullopt)
    playlist.SetBoolKey(kPlaylistsPartialReadyKey, partial_ready.value());

  return playlist;
}

std::vector<base::FilePath> GetOrphanedPaths(
    const base::FilePath& base_dir,
    const base::flat_set<std::string>& ids) {
  std::vector<base::FilePath> orphaned_paths;
  base::FileEnumerator dirs(base_dir, false, base::FileEnumerator::DIRECTORIES);
  for (base::FilePath name = dirs.Next(); !name.empty(); name = dirs.Next()) {
    std::string base_name =
#if defined(OS_WIN)
        base::UTF16ToUTF8(name.BaseName().value());
#else
        name.BaseName().value();
#endif
    if (base_name == "playlists_db")
      continue;

    if (ids.find(base_name) == ids.end())
      orphaned_paths.push_back(name);
  }
  return orphaned_paths;
}

}  // namespace

PlaylistsController::PlaylistsController(content::BrowserContext* context)
    : context_(context),
      url_loader_factory_(
          content::BrowserContext::GetDefaultStoragePartition(context)
              ->GetURLLoaderFactoryForBrowserProcess()),
      weak_factory_(this) {}

PlaylistsController::~PlaylistsController() {
  io_task_runner()->DeleteSoon(FROM_HERE, std::move(db_controller_));
}

bool PlaylistsController::Init(const base::FilePath& base_dir) {
  if (initialization_in_progress_)
    return true;

  initialization_in_progress_ = true;
  base_dir_ = base_dir;
  db_controller_.reset(
      new PlaylistsDBController(base_dir.Append(kDatabaseDirName)));
  video_media_file_controller_.reset(new PlaylistsMediaFileController(
      this, context_, FILE_PATH_LITERAL("video_file"),
      kPlaylistsVideoMediaFilePathKey,
      kPlaylistsCreateParamsVideoMediaFilesPathKey));
  audio_media_file_controller_.reset(new PlaylistsMediaFileController(
      this, context_, FILE_PATH_LITERAL("audio_file"),
      kPlaylistsAudioMediaFilePathKey,
      kPlaylistsCreateParamsAudioMediaFilesPathKey));

  return base::PostTaskAndReplyWithResult(
      io_task_runner(), FROM_HERE,
      base::BindOnce(&PlaylistsDBController::Init,
                     base::Unretained(db_controller_.get())),
      base::BindOnce(&PlaylistsController::OnDBInitialized,
                     weak_factory_.GetWeakPtr()));
}

void PlaylistsController::OnDBInitialized(bool initialized) {
  DCHECK(initialization_in_progress_);

  initialization_in_progress_ = false;
  initialized_ = initialized;
  VLOG(2) << __func__ << ": " << initialized_;

  for (PlaylistsControllerObserver& obs : observers_)
    obs.OnPlaylistsInitialized(initialized_);

  if (initialized_)
    CleanUp();
}

void PlaylistsController::NotifyPlaylistChanged(
    const PlaylistsChangeParams& params) {
  for (PlaylistsControllerObserver& obs : observers_)
    obs.OnPlaylistsChanged(params);
}

void PlaylistsController::DownloadThumbnail(base::Value&& playlist_value) {
  VLOG(2) << __func__;
  const std::string* thumbnail_url =
      playlist_value.FindStringPath(kPlaylistsCreateParamsThumbnailUrlPathKey);
  DCHECK(thumbnail_url);
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = GURL(*thumbnail_url);
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  auto loader = network::SimpleURLLoader::Create(
      std::move(request), GetNetworkTrafficAnnotationTagForURLLoad());
  loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);

  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(loader));

  const std::string* playlist_id =
      playlist_value.FindStringKey(kPlaylistsIDKey);
  DCHECK(playlist_id);
  base::FilePath thumbnail_path =
      base_dir_.Append(GetPlaylistIDDirName(*playlist_id))
          .Append(kThumbnailFileName);
  iter->get()->DownloadToFile(
      url_loader_factory_.get(),
      base::BindOnce(&PlaylistsController::OnThumbnailDownloaded,
                     base::Unretained(this), std::move(playlist_value),
                     std::move(iter)),
      thumbnail_path);
}

void PlaylistsController::OnThumbnailDownloaded(
    base::Value&& playlist_value,
    SimpleURLLoaderList::iterator it,
    base::FilePath path) {
  // When delete all is requested during the thumbnail downloading, we should
  // just return. |url_loaders_| is cleared.
  if (url_loaders_.empty())
    return;

  url_loaders_.erase(it);

  const std::string* playlist_id =
      playlist_value.FindStringKey(kPlaylistsIDKey);
  DCHECK(playlist_id);

  // When fetching thumbnail fails, go to generate media file step.
  if (path.empty()) {
    VLOG(2) << __func__
            << ": thumbnail fetching failed. goto media file generation step";

    NotifyPlaylistChanged(
        {PlaylistsChangeParams::ChangeType::CHANGE_TYPE_THUMBNAIL_FAILED,
         *playlist_id});

    MoveToMediaFileGenerationStep(std::move(playlist_value));
    return;
  }

  const std::string thumbnail_path =
#if defined(OS_WIN)
      base::UTF16ToUTF8(path.value());
#else
      path.value();
#endif
  playlist_value.SetStringKey(kPlaylistsThumbnailPathKey, thumbnail_path);

  std::string output;
  base::JSONWriter::Write(playlist_value, &output);
  PutThumbnailReadyPlaylistToDB(*playlist_id, output,
                                std::move(playlist_value));
}

void PlaylistsController::PutThumbnailReadyPlaylistToDB(
    const std::string& key,
    const std::string& json_value,
    base::Value&& playlist_value) {
  base::PostTaskAndReplyWithResult(
      io_task_runner(), FROM_HERE,
      base::BindOnce(&PlaylistsDBController::Put,
                     base::Unretained(db_controller_.get()), key, json_value),
      base::BindOnce(&PlaylistsController::OnPutThumbnailReadyPlaylist,
                     weak_factory_.GetWeakPtr(), std::move(playlist_value)));
}

void PlaylistsController::OnPutThumbnailReadyPlaylist(
    base::Value&& playlist_value,
    bool result) {
  VLOG(2) << __func__;
  const std::string* playlist_id =
      playlist_value.FindStringKey(kPlaylistsIDKey);
  DCHECK(playlist_id);
  if (!result) {
    NotifyPlaylistChanged(
        {PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ABORTED, *playlist_id});
    return;
  }

  NotifyPlaylistChanged(
      {PlaylistsChangeParams::ChangeType::CHANGE_TYPE_THUMBNAIL_READY,
       *playlist_id});

  MoveToMediaFileGenerationStep(std::move(playlist_value));
}

void PlaylistsController::MoveToMediaFileGenerationStep(
    base::Value&& playlist_value) {
  VLOG(2) << __func__;
  // Add to pending jobs
  pending_media_file_creation_jobs_.push(std::move(playlist_value));

  // If either media file controller is generating a playlist media file,
  // delay the next playlist generation. It will be triggered when the current
  // one is finished.
  if (!video_media_file_controller_->in_progress() &&
      !audio_media_file_controller_->in_progress()) {
    GenerateMediaFiles();
  }
}

void PlaylistsController::GenerateMediaFiles() {
  DCHECK(!video_media_file_controller_->in_progress() &&
         !audio_media_file_controller_->in_progress());
  DCHECK(!pending_media_file_creation_jobs_.empty());

  base::Value video_value(std::move(pending_media_file_creation_jobs_.front()));
  base::Value audio_value = video_value.Clone();
  pending_media_file_creation_jobs_.pop();
  VLOG(2) << __func__ << ": "
          << *video_value.FindStringKey(kPlaylistsPlaylistNameKey);

  video_media_file_controller_->GenerateSingleMediaFile(std::move(video_value),
                                                        base_dir_);
  audio_media_file_controller_->GenerateSingleMediaFile(std::move(audio_value),
                                                        base_dir_);
}

// Store PlaylistInfo to db after getting thumbnail and notify it.
// Then notify again when it's ready to play.
// TODO(simonhong): Add basic validation for |params|.
bool PlaylistsController::CreatePlaylist(const CreatePlaylistParams& params) {
  DCHECK(initialized_);
  PlaylistInfo p = CreatePlaylistInfo(params);

  base::Value value = GetValueFromPlaylistInfo(p);
  std::string output;
  base::JSONWriter::Write(value, &output);
  PutInitialPlaylistToDB(p.id, output, std::move(value));

  return true;
}

void PlaylistsController::PutInitialPlaylistToDB(const std::string& key,
                                                 const std::string& json_value,
                                                 base::Value&& playlist_value) {
  base::PostTaskAndReplyWithResult(
      io_task_runner(), FROM_HERE,
      base::BindOnce(&PlaylistsDBController::Put,
                     base::Unretained(db_controller_.get()), key, json_value),
      base::BindOnce(&PlaylistsController::OnPutInitialPlaylist,
                     weak_factory_.GetWeakPtr(), std::move(playlist_value)));
}

void PlaylistsController::OnPutInitialPlaylist(base::Value&& playlist_value,
                                               bool result) {
  const std::string* playlist_id =
      playlist_value.FindStringKey(kPlaylistsIDKey);
  if (!result) {
    NotifyPlaylistChanged(
        {PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ABORTED, *playlist_id});
    return;
  }

  NotifyPlaylistChanged(
      {PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ADDED, *playlist_id});

  const base::FilePath playlist_dir =
      base_dir_.Append(GetPlaylistIDDirName(*playlist_id));
  base::PostTaskAndReplyWithResult(
      io_task_runner(), FROM_HERE,
      base::BindOnce(&base::CreateDirectory, playlist_dir),
      base::BindOnce(&PlaylistsController::OnPlaylistDirCreated,
                     weak_factory_.GetWeakPtr(), std::move(playlist_value)));
}

void PlaylistsController::OnPlaylistDirCreated(base::Value&& playlist_value,
                                               bool directory_ready) {
  VLOG(2) << __func__;
  if (!directory_ready) {
    const std::string* playlist_id =
        playlist_value.FindStringKey(kPlaylistsIDKey);
    NotifyPlaylistChanged(
        {PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ABORTED, *playlist_id});
    return;
  }

  const std::string* thumbnail_url =
      playlist_value.FindStringPath(kPlaylistsCreateParamsThumbnailUrlPathKey);
  if (thumbnail_url && !thumbnail_url->empty()) {
    DownloadThumbnail(std::move(playlist_value));
  } else {
    VLOG(2) << __func__ << ": "
            << "thumbnail url is not available."
            << " goes to media file generation step";
    MoveToMediaFileGenerationStep(std::move(playlist_value));
  }
}

bool PlaylistsController::GetAllPlaylists(
    base::OnceCallback<void(base::Value)> callback) {
  DCHECK(initialized_);
  return base::PostTaskAndReplyWithResult(
      io_task_runner(), FROM_HERE,
      base::BindOnce(&PlaylistsDBController::GetAll,
                     base::Unretained(db_controller_.get())),
      base::BindOnce(&PlaylistsController::OnGetAllPlaylists,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void PlaylistsController::OnGetAllPlaylists(
    base::OnceCallback<void(base::Value)> callback,
    const std::vector<std::string>& playlist_info_jsons) {
  if (playlist_info_jsons.empty())
    return std::move(callback).Run(base::Value());

  base::Value playlists(base::Value::Type::LIST);
  for (const std::string& playlist_info_json : playlist_info_jsons) {
    LOG(ERROR) << playlist_info_json;
    playlists.GetList().push_back(
        GetPlaylistValueFromPlaylistInfoJSON(playlist_info_json));
  }
  std::move(callback).Run(std::move(playlists));
}

bool PlaylistsController::GetPlaylist(
    const std::string& id,
    base::OnceCallback<void(base::Value)> callback) {
  DCHECK(initialized_);

  return base::PostTaskAndReplyWithResult(
      io_task_runner(), FROM_HERE,
      base::BindOnce(&PlaylistsDBController::Get,
                     base::Unretained(db_controller_.get()), id),
      base::BindOnce(&PlaylistsController::OnGetPlaylist,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void PlaylistsController::DoRecoverPlaylist(
    const std::string& id,
    const std::string& playlist_info_json) {
  base::Optional<base::Value> playlist_info =
      base::JSONReader::Read(playlist_info_json);
  if (!playlist_info) {
    VLOG(1) << __func__ << ": "
            << "Invalid playlist id for recover: " << id;
    return;
  }

  const std::string* thumbnail_path_str =
      playlist_info->FindStringPath(kPlaylistsThumbnailPathKey);
  const bool has_thumbnail =
      !!thumbnail_path_str && !thumbnail_path_str->empty();
  if (has_thumbnail) {
    VLOG(2) << __func__ << ": "
            << "Already has thumbnail."
            << " This is in recovering";
    base::Optional<bool> partial_ready =
        playlist_info->FindBoolPath(kPlaylistsPartialReadyKey);
    const std::string* video_media_file_path =
        playlist_info->FindStringPath(kPlaylistsVideoMediaFilePathKey);
    const std::string* audio_media_file_path =
        playlist_info->FindStringPath(kPlaylistsAudioMediaFilePathKey);
    // Only try to regenerate if partial ready or there is no media file.
    if (!video_media_file_path || video_media_file_path->empty() ||
        !audio_media_file_path || audio_media_file_path->empty() ||
        *partial_ready) {
      VLOG(2) << __func__ << ": "
              << "Regenerate media file";
      MoveToMediaFileGenerationStep(std::move(*playlist_info));
    }
    return;
  }

  VLOG(2) << __func__ << ": "
          << "Try to download thumbnail";
  DownloadThumbnail(std::move(*playlist_info));
}

bool PlaylistsController::RecoverPlaylist(const std::string& id) {
  DCHECK(initialized_);
  return base::PostTaskAndReplyWithResult(
      io_task_runner(), FROM_HERE,
      base::BindOnce(&PlaylistsDBController::Get,
                     base::Unretained(db_controller_.get()), id),
      base::BindOnce(&PlaylistsController::DoRecoverPlaylist,
                     weak_factory_.GetWeakPtr(), id));
}

void PlaylistsController::OnGetPlaylist(
    base::OnceCallback<void(base::Value)> callback,
    const std::string& playlist_info_json) {
  std::move(callback).Run(
      GetPlaylistValueFromPlaylistInfoJSON(playlist_info_json));
}

bool PlaylistsController::DeletePlaylist(const std::string& id) {
  DCHECK(initialized_);

  if (video_media_file_controller_->current_playlist_id() == id) {
    video_media_file_controller_->RequestCancelCurrentPlaylistGeneration();
    audio_media_file_controller_->RequestCancelCurrentPlaylistGeneration();
  }

  return base::PostTaskAndReplyWithResult(
      io_task_runner(), FROM_HERE,
      base::BindOnce(&PlaylistsDBController::Del,
                     base::Unretained(db_controller_.get()), id),
      base::BindOnce(&PlaylistsController::OnDeletePlaylist,
                     weak_factory_.GetWeakPtr(), id));
}

bool PlaylistsController::RequestDownload(const std::string& url) {
  DCHECK(initialized_);

  // This is handled by third-party code (in JavaScript) so all we do here is
  // tell observers that a download was requested and trust that someone is
  // listening who will handle it.
  for (PlaylistsControllerObserver& obs : observers_)
    obs.OnPlaylistsDownloadRequested(url);
  return true;
}

void PlaylistsController::OnDeletePlaylist(const std::string& playlist_id,
                                           bool success) {
  if (!success)
    return;

  NotifyPlaylistChanged(
      {PlaylistsChangeParams::ChangeType::CHANGE_TYPE_DELETED, playlist_id});

  // Delete assets from filesystem after updating db.
  video_media_file_controller_->DeletePlaylist(
      base_dir_.Append(GetPlaylistIDDirName(playlist_id)));
  audio_media_file_controller_->DeletePlaylist(
      base_dir_.Append(GetPlaylistIDDirName(playlist_id)));
}

bool PlaylistsController::DeleteAllPlaylists(
    base::OnceCallback<void(bool)> callback) {
  DCHECK(initialized_);

  // Cancel currently generated playlist if needed and pending thumbnail
  // download jobs.
  video_media_file_controller_->RequestCancelCurrentPlaylistGeneration();
  audio_media_file_controller_->RequestCancelCurrentPlaylistGeneration();
  url_loaders_.clear();
  base::queue<base::Value> empty_queue;
  std::swap(pending_media_file_creation_jobs_, empty_queue);

  // During the delete, state is non-initialized state.
  initialized_ = false;
  initialization_in_progress_ = true;

  return base::PostTaskAndReplyWithResult(
      io_task_runner(), FROM_HERE,
      base::BindOnce(&PlaylistsDBController::DeleteAll,
                     base::Unretained(db_controller_.get())),
      base::BindOnce(&PlaylistsController::OnDeleteAllPlaylists,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void PlaylistsController::OnDeleteAllPlaylists(
    base::OnceCallback<void(bool)> callback,
    bool deleted) {
  VLOG(2) << __func__ << ": all deleted: " << deleted;
  std::move(callback).Run(deleted);
  initialized_ = true;
  initialization_in_progress_ = false;
  if (deleted) {
    CleanUp();
    NotifyPlaylistChanged(
        {PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ALL_DELETED, ""});
  }
}

void PlaylistsController::AddObserver(PlaylistsControllerObserver* observer) {
  observers_.AddObserver(observer);
}

void PlaylistsController::RemoveObserver(
    PlaylistsControllerObserver* observer) {
  observers_.RemoveObserver(observer);
}

void PlaylistsController::OnMediaFileReady(base::Value&& playlist_value,
                                           bool partial) {
  if (video_media_file_controller_->in_progress() ||
      audio_media_file_controller_->in_progress())
    partial = true;
  VLOG(2) << __func__ << ": "
          << *playlist_value.FindStringKey(kPlaylistsPlaylistNameKey) << " "
          << partial;

  std::string output;
  base::JSONWriter::Write(playlist_value, &output);
  const std::string* playlist_id =
      playlist_value.FindStringKey(kPlaylistsIDKey);
  DCHECK(playlist_id);
  PutPlayReadyPlaylistToDB(*playlist_id, output, partial,
                           std::move(playlist_value));

  if (partial)
    return;

  if (!pending_media_file_creation_jobs_.empty())
    GenerateMediaFiles();
}

void PlaylistsController::OnMediaFileGenerationFailed(
    base::Value&& playlist_value) {
  VLOG(2) << __func__ << ": "
          << *playlist_value.FindStringKey(kPlaylistsPlaylistNameKey);

  video_media_file_controller_->RequestCancelCurrentPlaylistGeneration();
  audio_media_file_controller_->RequestCancelCurrentPlaylistGeneration();
  const std::string* playlist_id =
      playlist_value.FindStringKey(kPlaylistsIDKey);
  DCHECK(playlist_id);
  NotifyPlaylistChanged(
      {PlaylistsChangeParams::ChangeType::CHANGE_TYPE_ABORTED, *playlist_id});

  if (!pending_media_file_creation_jobs_.empty())
    GenerateMediaFiles();
}

void PlaylistsController::PutPlayReadyPlaylistToDB(
    const std::string& key,
    const std::string& json_value,
    bool partial,
    base::Value&& playlist_value) {
  base::PostTaskAndReplyWithResult(
      io_task_runner(), FROM_HERE,
      base::BindOnce(&PlaylistsDBController::Put,
                     base::Unretained(db_controller_.get()), key, json_value),
      base::BindOnce(&PlaylistsController::OnPutPlayReadyPlaylist,
                     weak_factory_.GetWeakPtr(), std::move(playlist_value),
                     partial));
}

void PlaylistsController::OnPutPlayReadyPlaylist(base::Value&& playlist_value,
                                                 bool partial,
                                                 bool result) {
  if (!result)
    return;

  const std::string* playlist_id =
      playlist_value.FindStringKey(kPlaylistsIDKey);
  DCHECK(playlist_id);
  NotifyPlaylistChanged(
      {partial
           ? PlaylistsChangeParams::ChangeType::CHANGE_TYPE_PLAY_READY_PARTIAL
           : PlaylistsChangeParams::ChangeType::CHANGE_TYPE_PLAY_READY,
       *playlist_id});
}

void PlaylistsController::OnGetAllPlaylistsForCleanUp(base::Value playlists) {
  base::flat_set<std::string> ids;
  if (playlists.is_none()) {
    VLOG(2) << __func__ << ": "
            << "Empty playlists";
    return;
  }

  for (const auto& item : playlists.GetList()) {
    const std::string* id = item.FindStringKey(kPlaylistsIDKey);
    if (id)
      ids.insert(*id);
  }

  base::PostTaskAndReplyWithResult(
      io_task_runner(), FROM_HERE,
      base::BindOnce(&GetOrphanedPaths, base_dir_, ids),
      base::BindOnce(&PlaylistsController::OnGetOrphanedPaths,
                     weak_factory_.GetWeakPtr()));
}

void PlaylistsController::OnGetOrphanedPaths(
    const std::vector<base::FilePath> orphaned_paths) {
  if (orphaned_paths.empty()) {
    VLOG(2) << __func__ << ": "
            << "No orphaned playlist";
    return;
  }

  for (const auto& path : orphaned_paths) {
    VLOG(2) << __func__ << ": " << path << " is orphaned";
    video_media_file_controller_->DeletePlaylist(path);
    audio_media_file_controller_->DeletePlaylist(path);
  }
}

void PlaylistsController::CleanUp() {
  GetAllPlaylists(
      base::BindOnce(&PlaylistsController::OnGetAllPlaylistsForCleanUp,
                     weak_factory_.GetWeakPtr()));
}

base::SequencedTaskRunner* PlaylistsController::io_task_runner() {
  if (!io_task_runner_) {
    io_task_runner_ = base::CreateSequencedTaskRunnerWithTraits(
        {base::ThreadPool(), base::MayBlock(), base::TaskPriority::BEST_EFFORT,
         base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
  }
  return io_task_runner_.get();
}
